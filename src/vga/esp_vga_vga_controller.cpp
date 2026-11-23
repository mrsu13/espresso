// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga_controller.h
 * @brief Объявление контроллера VGA на 64 цвета esp::vga::vga_controller.
 */

/**
 * @file esp_vga_vga_controller.cpp
 * @brief Контроллер VGA на 64 цвета: байт на пиксель, рисование в обработчике прерывания.
 */

#include "esp_vga_vga_controller.h"

#include "esp_gfx_primitive.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_intr_alloc.h>
#include <esp_timer.h>
#include <soc/i2s_struct.h>


#pragma GCC optimize("O2")


/**
 * @class esp::vga::vga_controller
 * @brief Контроллер VGA на 64 цвета: в буфере кадра байт на пиксель.
 *
 * Каждый байт буфера — пиксель с цветом (по 2 разряда на канал) и
 * разрядами синхроимпульсов, и DMA выводит его прямо из буфера; поэтому
 * буфер велик (при 640 × 480 — 300 КБ) и большие разрешения не всегда
 * помещаются в память. Примитивы рисует обработчик прерывания в начале
 * переднего гасящего интервала кадра, так что все функции рисования — в
 * IRAM. Двойная буферизация — сменой цепочек DMA.
 *
 * С 8 цветами — по выводу на канал:
 * @code
 * esp::vga::vga_controller display;
 * display.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
 * display.set_resolution(esp::vga::VGA_640X350_70HZ);
 * @endcode
 */


// Открытые

/**
 * @brief Создаёт контроллер; выводы и развёртку задают begin() и set_resolution().
 */
esp::vga::vga_controller::vga_controller() {}

/**
 * @brief Приостанавливает рисование примитивов: на первом уровне вложенности запрещает прерывание.
 */
void esp::vga::vga_controller::suspend_background_primitive_execution()
{
    vga_base_controller::suspend_background_primitive_execution();
    if (_primitive_processing_suspended == 1)
    {
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 0;
    }
}

/**
 * @brief Возобновляет рисование примитивов: когда приостановок не осталось, разрешает прерывание.
 *
 * При первом вызове ставит обработчик прерывания I2S1.
 */
void esp::vga::vga_controller::resume_background_primitive_execution()
{
    vga_base_controller::resume_background_primitive_execution();
    if (_primitive_processing_suspended == 0)
    {
        if (!_isr_handle)
            esp_intr_alloc(ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, v_sync_interrupt, this,
                           &_isr_handle);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }
}

/**
 * @brief Задаёт развёртку, очищает буфер кадра и запускает вывод.
 *
 * Обработчику прерывания даётся время всех гасящих интервалов кадра и
 * строк над областью вывода.
 *
 * @param timings Развёртка.
 * @param view_port_width Ширина области вывода, пиксели; −1 — вся видимая ширина.
 * @param view_port_height Высота области вывода, строки; −1 — вся видимая
 *        высота, если хватит памяти.
 * @param double_buffered @c true — второй буфер того же размера для рисования.
 */
void esp::vga::vga_controller::set_resolution(const vga_timings &timings, int view_port_width,
                                              int view_port_height, bool double_buffered)
{
    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    for (int i = 0; i < _view_port_height; ++i)
        fill(_view_port[i], 0, _view_port_width, 0, 0, 0, false, false);

    // Сколько микросекунд может работать обработчик кадрового синхроимпульса.
    _max_v_sync_isr_time =
        std::ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                  * (_timings.v_sync_pulse + _timings.v_back_porch + _timings.v_front_porch
                     + _view_port_row));

    start_gpio_stream();
    resume_background_primitive_execution();
}

/**
 * @brief Читает пиксели области вывода цветами RGB222.
 *
 * Границы не проверяются. Чтобы прочитать уже нарисованное, сначала
 * дождитесь примитивов (process_primitives()):
 * @code
 * esp::gfx::rgb222 buf[100 * 100];
 * const esp::gfx::rect rect{0, 0, 99, 99};
 * display.process_primitives();
 * display.read_screen(rect, buf);
 * display.write_screen(rect.translate(110, 0), buf);
 * @endcode
 *
 * @param rect Прямоугольник.
 * @param dest_buf Буфер на ширину × высоту прямоугольника цветов, построчно.
 */
void esp::vga::vga_controller::read_screen(const esp::gfx::rect &rect, esp::gfx::rgb222 *dest_buf)
{
    std::uint8_t *dbuf = reinterpret_cast<std::uint8_t *>(dest_buf);
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = rect.x1; x <= rect.x2; ++x, ++dbuf)
            *dbuf = pixel_in_row(row, x) & ~SYNC_MASK;
    }
}

/**
 * @brief Читает пиксели области вывода цветами RGB888.
 *
 * Границы не проверяются.
 *
 * @param rect Прямоугольник.
 * @param dest_buf Буфер на ширину × высоту прямоугольника цветов, построчно.
 */
void esp::vga::vga_controller::read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            std::uint8_t rawpix = pixel_in_row(row, x);
            *dest_buf = esp::gfx::rgb888((rawpix & 3) * 85, ((rawpix >> 2) & 3) * 85,
                                         ((rawpix >> 4) & 3) * 85);
        }
    }
}

/**
 * @brief Записывает пиксели в буфер рисования без очереди примитивов.
 *
 * Границы не проверяются; пример — в read_screen().
 *
 * @param rect Прямоугольник.
 * @param src_buf Цвета на ширину × высоту прямоугольника, построчно.
 */
void esp::vga::vga_controller::write_screen(const esp::gfx::rect &rect, esp::gfx::rgb222 *src_buf)
{
    std::uint8_t *sbuf = reinterpret_cast<std::uint8_t *>(src_buf);
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = rect.x1; x <= rect.x2; ++x, ++sbuf)
            pixel_in_row(row, x) = *sbuf | _hv_sync;
    }
}

// Закрытые

/**
 * @brief Сбрасывает состояние контроллера и включает двойную буферизацию через DMA; его вызывает begin().
 */
void esp::vga::vga_controller::init()
{
    vga_base_controller::init();

    _double_buffer_over_dma = true;
}

/**
 * @brief Выделяет буфер кадра: байт на пиксель в памяти DMA.
 */
void esp::vga::vga_controller::allocate_view_port()
{
    vga_base_controller::allocate_view_port(MALLOC_CAP_DMA, _view_port_width);
}

/**
 * @brief Отмечает прерыванием дескриптор начала переднего гасящего интервала кадра.
 * @param buffer Дескриптор.
 * @param is_start_of_vert_front_porch Первый ли это дескриптор переднего
 *        гасящего интервала кадра.
 * @param scan Номер повтора строки.
 * @param is_visible @c true — дескриптор строки области вывода.
 * @param visible_row Строка области вывода.
 */
void esp::vga::vga_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                                   bool is_start_of_vert_front_porch, int scan,
                                                   bool is_visible, int visible_row)
{
    // Прерывание — в начале переднего гасящего интервала кадра.
    if (is_start_of_vert_front_porch)
        buffer->eof = 1;
}

/**
 * @brief Обработчик прерывания в начале переднего гасящего интервала кадра: рисует примитивы.
 *
 * Рисует, пока не кончится очередь или отведённое время
 * (@c _max_v_sync_isr_time), затем возвращает спрайты. Работает в IRAM.
 *
 * @param arg Контроллер.
 */
void IRAM_ATTR esp::vga::vga_controller::v_sync_interrupt(void *arg)
{
    if (I2S1.int_st.out_eof)
    {
        auto *const self = static_cast<esp::vga::vga_controller *>(arg);
        const std::int64_t start_time =
            self->background_primitive_timeout_enabled() ? esp_timer_get_time() : 0;
        esp::gfx::rect update_rect{SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN};
        do
        {
            esp::gfx::primitive prim;
            if (!self->get_primitive_isr(&prim))
                break;

            self->exec_primitive(prim, update_rect, true);

            if (self->_primitive_processing_suspended)
                break;

        } while (!self->background_primitive_timeout_enabled()
                 || (start_time + self->_max_v_sync_isr_time > esp_timer_get_time()));
        self->show_sprites(update_rect);
    }
    I2S1.int_clr.val = I2S1.int_st.val;
}

/**
 * @brief Рисует точку заданного цвета.
 * @param pixel_desc Положение относительно начала координат и цвет.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::set_pixel_at(const esp::gfx::pixel_desc &pixel_desc,
                                                      esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect,
        [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int x, int y, std::uint8_t pattern) { pixel(x, y) = pattern; });
}

/**
 * @brief Рисует линию в абсолютных координатах с отсечением по текущей области отсечения.
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 * @param color Цвет.
 */
void IRAM_ATTR esp::vga::vga_controller::abs_draw_line(int x1, int y1, int x2, int y2,
                                                       esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y, int x1, int x2, std::uint8_t pattern) { raw_fill_row(y, x1, x2, pattern); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); },
        [&](int x, int y, std::uint8_t pattern) { pixel(x, y) = pattern; },
        [&](int x, int y) { invert_pixel(x, y); });
}

/**
 * @brief Заливает отрезок строки цветом; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param color Цвет.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, prepare_pixel(color));
}

/**
 * @brief Заливает отрезок строки байтом пикселя; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param pattern Байт пикселя.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_fill_row(int y, int x1, int x2, std::uint8_t pattern)
{
    auto row = _view_port[y];
    // Первые байты до целого 32-разрядного слова.
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        pixel_in_row(row, x) = pattern;
    }
    // Целые 32-разрядные слова (порядок пикселей в слове не важен).
    if (x <= x2)
    {
        int sz = (x2 & ~3) - x;
        std::memset(const_cast<std::uint8_t *>(row + x), pattern, sz);
        x += sz;
    }
    // Последние невыровненные байты.
    for (; x <= x2; ++x)
    {
        pixel_in_row(row, x) = pattern;
    }
}

/**
 * @brief Инвертирует цвета в отрезке строки, не трогая синхроимпульсы; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
    {
        std::uint8_t *const px = const_cast<std::uint8_t *>(&pixel_in_row(row, x));
        *px = _hv_sync | ~(*px);
    }
}

/**
 * @brief Меняет местами отрезки двух строк; параметры не проверяются.
 * @param y_a Первая строка.
 * @param y_b Вторая строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void IRAM_ATTR esp::vga::vga_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    std::uint8_t *row_a = const_cast<std::uint8_t *>(_view_port[y_a]);
    std::uint8_t *row_b = const_cast<std::uint8_t *>(_view_port[y_b]);
    // Первые байты до целого 32-разрядного слова.
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
        std::swap(pixel_in_row(row_a, x), pixel_in_row(row_b, x));
    // Целые 32-разрядные слова (порядок пикселей в слове не важен).
    std::uint32_t *a = reinterpret_cast<std::uint32_t *>(row_a + x);
    std::uint32_t *b = reinterpret_cast<std::uint32_t *>(row_b + x);
    for (int right = (x2 & ~3); x < right; x += 4)
        std::swap(*a++, *b++);
    // Последние невыровненные байты.
    for (x = (x2 & ~3); x <= x2; ++x)
        std::swap(pixel_in_row(row_a, x), pixel_in_row(row_b, x));
}

/**
 * @brief Рисует эллипс пером с центром в текущей позиции.
 * @param size Размеры эллипса.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::draw_ellipse(const esp::gfx::size &size,
                                                      esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int x, int y, std::uint8_t pattern) { pixel(x, y) = pattern; });
}

/**
 * @brief Заливает область вывода цветом кисти.
 * @param update_rect Изменённая область.
 */
void IRAM_ATTR esp::vga::vga_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t pattern = prepare_pixel(actual_brush_color());
    for (int y = 0; y < _view_port_height; ++y)
        std::memset(const_cast<std::uint8_t *>(_view_port[y]), pattern, _view_port_width);
}

/**
 * @brief Прокручивает область прокрутки по вертикали.
 *
 * Строки меняются местами вместе с указателями DMA; если задана область
 * прокрутки уже экрана, строки копируются, а это медленнее.
 *
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 */
void IRAM_ATTR esp::vga::vga_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swap_rows_copying
        [&](int y_a, int y_b)
        { std::swap(_view_port[y_a], _view_port[y_b]); }, // swap_rows_pointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 pattern)
        { raw_fill_row(y, x1, x2, pattern); } // raw_fill_row
    );

    if (scroll != 0)
    {
        // Переназначаем указатели DMA.
        int view_port_buffers_per_line = 0;
        int line_pos = 1;
        switch (_timings.h_starting_block)
        {
            case vga_scan_start::front_porch:
                // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
                view_port_buffers_per_line =
                    (_view_port_col + _view_port_width) < _timings.h_visible_area ? 3 : 2;
                break;
            case vga_scan_start::sync:
                // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
                view_port_buffers_per_line = 3;
                break;
            case vga_scan_start::back_porch:
                // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
                view_port_buffers_per_line = 3;
                break;
            case vga_scan_start::visible_area:
                // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
                view_port_buffers_per_line = _view_port_col > 0 ? 3 : 2;
                line_pos = _view_port_col > 0 ? 1 : 0;
                break;
        }
        const int y1 = paint_state().scrolling_region.y1;
        const int y2 = paint_state().scrolling_region.y2;
        for (int i = y1, idx = y1 * _timings.scan_count; i <= y2; ++i)
            for (int scan = 0; scan < _timings.scan_count; ++scan, ++idx)
                set_dma_buffer_view(_view_port_row * _timings.scan_count
                                        + idx * view_port_buffers_per_line + line_pos,
                                    i, scan, _view_port, false);
    }
}

/**
 * @brief Прокручивает область прокрутки по горизонтали.
 *
 * Сдвиги на 1, 2, 3 и 4 пикселя и на кратное 4 быстрые; другие
 * раскладываются на два-три быстрых (5 = 4 + 1, 6 = 4 + 2). Начало и
 * ширина области прокрутки должны быть кратны 4 пикселям, иначе работает
 * очень медленный вариант.
 *
 * @param scroll Шаг: отрицательный — влево, положительный — вправо.
 * @param update_rect Изменённая область.
 */
void IRAM_ATTR esp::vga::vga_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t pattern8 = prepare_pixel(actual_brush_color());
    std::uint16_t pattern16 = pattern8 << 8 | pattern8;
    std::uint32_t pattern32 = pattern16 << 16 | pattern16;

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    int width = x2 - x1 + 1;
    int width32 = width >> 2;
    bool h_scrollling_region_aligned = ((x1 & 3) == 0 && (width & 3) == 0);

    if (scroll < 0)
    {
        // Влево.
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // Область прокрутки выровнена: быстрый вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y] + x1);
                for (int s = -scroll; s > 0;)
                {
                    if (s >= 4)
                    {
                        // Влево на 4, 8, 12… пикселя сдвигом 32-разрядных слов.
                        std::uint8_t *w = row;
                        int sz = (s & ~3) >> 2;
                        for (int i = 0; i < width32 - sz; ++i, w += 4)
                            *reinterpret_cast<std::uint32_t *>(w) =
                                *(reinterpret_cast<std::uint32_t *>(w) + sz);
                        for (int i = std::max(0, width32 - sz); i < width32; ++i, w += 4)
                            *reinterpret_cast<std::uint32_t *>(w) = pattern32;
                        s -= (s & ~3);
                    }
                    else if ((s & 3) == 3)
                    {
                        // Влево на 3 пикселя перестановкой байтов.
                        std::uint8_t *b = row;
                        for (int i = 1; i < width32; ++i, b += 4)
                        {
                            b[2] = b[1];
                            b[1] = b[4];
                            b[0] = b[7];
                            b[3] = b[6];
                        }
                        b[2] = b[1];
                        b[1] = b[0] = b[3] = pattern8;
                        s -= 3;
                    }
                    else if (s & 2)
                    {
                        // Влево на 2 пикселя перестановкой 16-разрядных слов.
                        std::uint16_t *w = reinterpret_cast<std::uint16_t *>(row);
                        for (int i = 1; i < width32; ++i, w += 2)
                        {
                            w[1] = w[0];
                            w[0] = w[3];
                        }
                        w[1] = w[0];
                        w[0] = pattern16;
                        s -= 2;
                    }
                    else if (s & 1)
                    {
                        // Влево на 1 пиксель вращением 32-разрядных слов.
                        std::uint8_t *w = row;
                        for (int i = 1; i < width32; ++i, w += 4)
                        {
                            *reinterpret_cast<std::uint32_t *>(w) =
                                *reinterpret_cast<std::uint32_t *>(w) >> 8
                                | *reinterpret_cast<std::uint32_t *>(w) << 24;
                            w[1] = w[6];
                        }
                        *reinterpret_cast<std::uint32_t *>(w) =
                            *reinterpret_cast<std::uint32_t *>(w) >> 8
                            | *reinterpret_cast<std::uint32_t *>(w) << 24;
                        w[1] = pattern8;
                        --s;
                    }
                }
            }
            else
            {
                // Область прокрутки не выровнена: медленный вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
                for (int x = x1; x <= x2 + scroll; ++x)
                    pixel_in_row(row, x) = pixel_in_row(row, x - scroll);
                // Освободившееся место справа — цветом кисти.
                for (int x = x2 + 1 + scroll; x <= x2; ++x)
                    pixel_in_row(row, x) = pattern8;
            }
        }
    }
    else if (scroll > 0)
    {
        // Вправо.
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // Область прокрутки выровнена: быстрый вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y] + x1);
                for (int s = scroll; s > 0;)
                {
                    if (s >= 4)
                    {
                        // Вправо на 4, 8, 12… пикселя сдвигом 32-разрядных слов.
                        int sz = (s & ~3) >> 2;
                        std::uint8_t *w = row + width - 4;
                        for (int i = 0; i < width32 - sz; ++i, w -= 4)
                            *reinterpret_cast<std::uint32_t *>(w) =
                                *(reinterpret_cast<std::uint32_t *>(w) - sz);
                        for (int i = std::max(0, width32 - sz); i < width32; ++i, w -= 4)
                            *reinterpret_cast<std::uint32_t *>(w) = pattern32;
                        s -= (s & ~3);
                    }
                    else if ((s & 3) == 3)
                    {
                        // Вправо на 3 пикселя перестановкой байтов.
                        std::uint8_t *b = row + width - 4;
                        for (int i = 1; i < width32; ++i, b -= 4)
                        {
                            b[0] = b[-3];
                            b[1] = b[2];
                            b[2] = b[-1];
                            b[3] = b[-4];
                        }
                        b[1] = b[2];
                        b[0] = b[2] = b[3] = pattern8;
                        s -= 3;
                    }
                    else if (s & 2)
                    {
                        // Вправо на 2 пикселя перестановкой 16-разрядных слов.
                        std::uint16_t *w = reinterpret_cast<std::uint16_t *>(row + width - 4);
                        for (int i = 1; i < width32; ++i, w -= 2)
                        {
                            w[0] = w[1];
                            w[1] = w[-2];
                        }
                        w[0] = w[1];
                        w[1] = pattern16;
                        s -= 2;
                    }
                    else if (s & 1)
                    {
                        // Вправо на 1 пиксель вращением 32-разрядных слов.
                        std::uint8_t *w = row + width - 4;
                        for (int i = 1; i < width32; ++i, w -= 4)
                        {
                            *reinterpret_cast<std::uint32_t *>(w) =
                                *reinterpret_cast<std::uint32_t *>(w) << 8
                                | *reinterpret_cast<std::uint32_t *>(w) >> 24;
                            w[2] = w[-3];
                        }
                        *reinterpret_cast<std::uint32_t *>(w) =
                            *reinterpret_cast<std::uint32_t *>(w) << 8
                            | *reinterpret_cast<std::uint32_t *>(w) >> 24;
                        w[2] = pattern8;
                        --s;
                    }
                }
            }
            else
            {
                // Область прокрутки не выровнена: медленный вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
                for (int x = x2 - scroll; x >= x1; --x)
                    pixel_in_row(row, x + scroll) = pixel_in_row(row, x);
                // Освободившееся место слева — цветом кисти.
                for (int x = x1; x < x1 + scroll; ++x)
                    pixel_in_row(row, x) = pattern8;
            }
        }
    }
}

/**
 * @brief Рисует знак.
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::draw_glyph(const esp::gfx::glyph &glyph,
                                                    esp::gfx::glyph_options glyph_options,
                                                    esp::gfx::rgb888 pen_color,
                                                    esp::gfx::rgb888 brush_color,
                                                    esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        [&](std::uint8_t *row, int x, std::uint8_t pattern) { pixel_in_row(row, x) = pattern; });
}

/**
 * @brief Инвертирует цвета в прямоугольнике.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::invert_rect(const esp::gfx::rect &rect,
                                                     esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}

/**
 * @brief Меняет в прямоугольнике цвета пера и кисти; другие цвета не трогает.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::swap_fgbg(const esp::gfx::rect &rect,
                                                   esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        [&](std::uint8_t *row, int x) { return pixel_in_row(row, x); },
        [&](std::uint8_t *row, int x, std::uint8_t pattern) { pixel_in_row(row, x) = pattern; });
}

/**
 * @brief Копирует прямоугольник в текущую позицию; области могут перекрываться.
 *
 * Медленно: пиксели копируются по одному.
 *
 * @param source Исходный прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void IRAM_ATTR esp::vga::vga_controller::copy_rect(const esp::gfx::rect &source,
                                                   esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        [&](std::uint8_t *row, int x) { return pixel_in_row(row, x); },
        [&](std::uint8_t *row, int x, std::uint8_t pattern) { pixel_in_row(row, x) = pattern; });
}

/**
 * @brief Переносит часть растра собственного формата на экран без отсечения.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                                const esp::gfx::bitmap *bitmap,
                                                                int x1, int y1, int x_count,
                                                                int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x, std::uint8_t src)
        { pixel_in_row(row, x) = _hv_sync | src; } // raw_set_pixel_in_row
    );
}

/**
 * @brief Рисует маску растра цветом его переднего плана без отсечения.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                              const esp::gfx::bitmap *bitmap,
                                                              void *save_background, int x1, int y1,
                                                              int x_count, int y_count)
{
    auto foreground_pattern = prepare_pixel(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x) { return pixel_in_row(row, x); }, // raw_get_pixel_in_row
        [&](std::uint8_t *row, int x)
        { pixel_in_row(row, x) = foreground_pattern; } // raw_set_pixel_in_row
    );
}

/**
 * @brief Переносит часть растра RGBA2222 на экран без отсечения; прозрачные пиксели пропускаются.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                                  const esp::gfx::bitmap *bitmap,
                                                                  void *save_background, int x1,
                                                                  int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x) { return pixel_in_row(row, x); }, // raw_get_pixel_in_row
        [&](std::uint8_t *row, int x, std::uint8_t src)
        { pixel_in_row(row, x) = _hv_sync | (src & 0x3f); } // raw_set_pixel_in_row
    );
}

/**
 * @brief Переносит часть растра RGBA8888 на экран без отсечения; прозрачные пиксели пропускаются.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void IRAM_ATTR esp::vga::vga_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                                  const esp::gfx::bitmap *bitmap,
                                                                  void *save_background, int x1,
                                                                  int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x) { return pixel_in_row(row, x); }, // raw_get_pixel_in_row
        [&](std::uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        {
            pixel_in_row(row, x) = _hv_sync | (src.r >> 6) | (src.g >> 6 << 2) | (src.b >> 6 << 4);
        } // raw_set_pixel_in_row
    );
}
