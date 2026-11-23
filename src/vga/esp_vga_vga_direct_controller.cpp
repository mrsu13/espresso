// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga_direct_controller.h
 * @brief Объявление контроллера VGA с рисованием строк на лету esp::vga::vga_direct_controller.
 */

/**
 * @file esp_vga_vga_direct_controller.cpp
 * @brief Контроллер VGA, который рисует строки на лету функцией приложения.
 */

#include "esp_vga_vga_direct_controller.h"

#include "esp_tools.h"

#include <cstdint>

#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_intr_alloc.h>
#include <soc/i2s_struct.h>


#pragma GCC optimize("O2")


/**
 * @class esp::vga::vga_direct_controller
 * @brief Контроллер VGA без буфера кадра: строки рисует функция приложения на лету.
 *
 * Обработчик прерывания каждые несколько строк вызывает функцию рисования
 * (set_draw_scanline_callback()), и она заполняет следующие строки DMA
 * байтами пикселей (create_raw_pixel()). Примитивы рисования не
 * поддерживаются. Функция рисования и всё, что она вызывает, должно быть
 * в IRAM.
 * @code
 * void IRAM_ATTR draw_scanline(void *arg, std::uint8_t *dest, int scan_line)
 * {
 *     // Заполнить dest байтами пикселей строки scan_line.
 * }
 *
 * esp::vga::vga_direct_controller display;
 * display.begin();
 * display.set_draw_scanline_callback(draw_scanline);
 * display.set_resolution(esp::vga::VGA_640X480_60HZ);
 * @endcode
 */


volatile int esp::vga::vga_direct_controller::_scan_line;
lldesc_t volatile *esp::vga::vga_direct_controller::_frame_reset_desc;
bool esp::vga::vga_direct_controller::_v_sync;
lldesc_t volatile **esp::vga::vga_direct_controller::_dma_lines = nullptr;


// Открытые

/**
 * @brief Создаёт контроллер; выводы и развёртку задают begin() и set_resolution().
 * @param auto_run @c true — начинать вывод сразу после set_resolution(),
 *        @c false — после run().
 */
esp::vga::vga_direct_controller::vga_direct_controller(bool auto_run)
    : _lines_count{2}
    , _lines{nullptr}
    , _draw_scanline_callback{nullptr}
    , _auto_run{auto_run}
{
}

/**
 * @brief Задаёт развёртку и, если задано при создании, начинает вывод.
 *
 * Без функции рисования (set_draw_scanline_callback()) ничего не делает.
 *
 * @param timings Развёртка.
 * @param view_port_width Ширина области вывода, пиксели; −1 — вся видимая ширина.
 * @param view_port_height Высота области вывода, строки; −1 — вся видимая высота.
 * @param double_buffered Не используется: буфера кадра нет.
 */
void esp::vga::vga_direct_controller::set_resolution(const vga_timings &timings,
                                                     int view_port_width, int view_port_height,
                                                     bool double_buffered)
{
    // Без set_draw_scanline_callback() работать нельзя.
    if (!_draw_scanline_callback)
        return;

    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    if (_auto_run)
        run();
}

/**
 * @brief Ничего не делает: буфера кадра нет.
 * @param rect Прямоугольник.
 * @param dest_buf Буфер цветов.
 */
void esp::vga::vga_direct_controller::read_screen(const esp::gfx::rect &rect,
                                                  esp::gfx::rgb888 *dest_buf)
{
}

/**
 * @brief Начинает вывод: запускает поток DMA и ставит обработчик прерывания.
 *
 * Нужен, только если контроллер создан с @c auto_run = @c false.
 */
void esp::vga::vga_direct_controller::run()
{
    // Поток запускается до установки обработчика прерывания.
    start_gpio_stream();

    _scan_line = 0;

    // Уровень 1 — ниже, чем у контроллера PS/2: они могут работать на одном ядре.
    if (!_isr_handle)
    {
        esp::core_usage::set_busiest_core(esp::VIDEO_TASKS_CORE);
        esp::esp_intr_alloc_pinned_to_core(ETS_I2S1_INTR_SOURCE,
                                           ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, isr_handler,
                                           this, &_isr_handle, esp::VIDEO_TASKS_CORE);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }
}

/**
 * @brief Направляет строку экрана на свой буфер вместо строки DMA.
 * @param scanline Строка экрана: от 0 до высоты − 1.
 * @param line_buffer Буфер строки в памяти DMA.
 */
void esp::vga::vga_direct_controller::set_scanline_buffer(int scanline,
                                                          std::uint8_t volatile *line_buffer)
{
    _dma_lines[scanline]->buf = line_buffer;
}

/**
 * @brief Возвращает буфер, который сейчас выводится для строки экрана.
 * @param scanline Строка экрана: от 0 до высоты − 1.
 * @return Буфер строки.
 */
std::uint8_t volatile *esp::vga::vga_direct_controller::scanline_buffer(int scanline)
{
    return const_cast<volatile std::uint8_t *>(_dma_lines[scanline]->buf);
}

/**
 * @brief Возвращает строку DMA, которую контроллер выделил для строки экрана.
 *
 * Строк DMA вдвое больше, чем строк за вызов функции рисования
 * (set_scanlines_per_call_back()); по умолчанию две.
 *
 * @param scanline Строка экрана: от 0 до высоты − 1.
 * @return Строка DMA.
 */
std::uint8_t volatile *esp::vga::vga_direct_controller::default_scanline_buffer(int scanline)
{
    return _lines[scanline % _lines_count];
}

// Закрытые

/**
 * @brief Сбрасывает состояние контроллера; его вызывает begin().
 */
void esp::vga::vga_direct_controller::init()
{
    vga_base_controller::init();
    _double_buffer_over_dma = false;
}

/**
 * @brief Направляет дескриптор видимой строки на строку DMA и запоминает его.
 *
 * Каждые полчисла строк DMA дескриптор вызывает прерывание; дескриптор
 * первой строки запоминается как начало кадра.
 *
 * @param buffer Дескриптор.
 * @param is_start_of_vert_front_porch Первый ли это дескриптор переднего
 *        гасящего интервала кадра.
 * @param scan Номер повтора строки.
 * @param is_visible @c true — дескриптор строки области вывода.
 * @param visible_row Строка области вывода.
 */
void esp::vga::vga_direct_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                                          bool is_start_of_vert_front_porch,
                                                          int scan, bool is_visible,
                                                          int visible_row)
{
    if (is_visible)
    {
        _dma_lines[visible_row] = buffer;

        buffer->buf = _lines[visible_row % _lines_count];

        // Прерывание — каждые полчисла строк DMA.
        if ((scan == 0 && (visible_row % (_lines_count / 2)) == 0))
        {
            if (visible_row == 0)
                _frame_reset_desc = buffer;
            buffer->eof = 1;
        }
    }
}

/**
 * @brief Выделяет строки DMA и таблицу дескрипторов строк.
 */
void esp::vga::vga_direct_controller::allocate_view_port()
{
    _lines = static_cast<std::uint8_t **>(heap_caps_malloc(sizeof(std::uint8_t *) * _lines_count,
                                                           MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
    _lines[0] = static_cast<std::uint8_t *>(
        heap_caps_malloc(_view_port_width * _lines_count, MALLOC_CAP_DMA));
    for (int i = 1; i < _lines_count; ++i)
        _lines[i] = _lines[0] + i * _view_port_width;
    _dma_lines = new lldesc_t volatile *[_view_port_height];
}

/**
 * @brief Освобождает строки DMA и таблицу дескрипторов строк.
 */
void esp::vga::vga_direct_controller::free_view_port()
{
    vga_base_controller::free_view_port();

    heap_caps_free(_lines[0]);
    heap_caps_free(_lines);
    _lines = nullptr;
    delete[] _dma_lines;
    _dma_lines = nullptr;
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param pixel_desc Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::set_pixel_at(const esp::gfx::pixel_desc &pixel_desc,
                                                   esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param size Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::draw_ellipse(const esp::gfx::size &size,
                                                   esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::clear(esp::gfx::rect &update_rect) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param scroll Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::v_scroll(int scroll, esp::gfx::rect &update_rect) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param scroll Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::h_scroll(int scroll, esp::gfx::rect &update_rect) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param glyph Не используется.
 * @param glyph_options Не используется.
 * @param pen_color Не используется.
 * @param brush_color Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::draw_glyph(const esp::gfx::glyph &glyph,
                                                 esp::gfx::glyph_options glyph_options,
                                                 esp::gfx::rgb888 pen_color,
                                                 esp::gfx::rgb888 brush_color,
                                                 esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param rect Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::invert_rect(const esp::gfx::rect &rect,
                                                  esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param source Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::copy_rect(const esp::gfx::rect &source,
                                                esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param rect Не используется.
 * @param update_rect Не используется.
 */
void esp::vga::vga_direct_controller::swap_fgbg(const esp::gfx::rect &rect,
                                                esp::gfx::rect &update_rect)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param dest_x Не используется.
 * @param dest_y Не используется.
 * @param bitmap Не используется.
 * @param x1 Не используется.
 * @param y1 Не используется.
 * @param x_count Не используется.
 * @param y_count Не используется.
 */
void esp::vga::vga_direct_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                             const esp::gfx::bitmap *bitmap, int x1,
                                                             int y1, int x_count, int y_count)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param dest_x Не используется.
 * @param dest_y Не используется.
 * @param bitmap Не используется.
 * @param save_background Не используется.
 * @param x1 Не используется.
 * @param y1 Не используется.
 * @param x_count Не используется.
 * @param y_count Не используется.
 */
void esp::vga::vga_direct_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                           const esp::gfx::bitmap *bitmap,
                                                           void *save_background, int x1, int y1,
                                                           int x_count, int y_count)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param dest_x Не используется.
 * @param dest_y Не используется.
 * @param bitmap Не используется.
 * @param save_background Не используется.
 * @param x1 Не используется.
 * @param y1 Не используется.
 * @param x_count Не используется.
 * @param y_count Не используется.
 */
void esp::vga::vga_direct_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                               const esp::gfx::bitmap *bitmap,
                                                               void *save_background, int x1,
                                                               int y1, int x_count, int y_count)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param dest_x Не используется.
 * @param dest_y Не используется.
 * @param bitmap Не используется.
 * @param save_background Не используется.
 * @param x1 Не используется.
 * @param y1 Не используется.
 * @param x_count Не используется.
 * @param y_count Не используется.
 */
void esp::vga::vga_direct_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                               const esp::gfx::bitmap *bitmap,
                                                               void *save_background, int x1,
                                                               int y1, int x_count, int y_count)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param y Не используется.
 * @param x1 Не используется.
 * @param x2 Не используется.
 * @param color Не используется.
 */
void esp::vga::vga_direct_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param y Не используется.
 * @param x1 Не используется.
 * @param x2 Не используется.
 * @param color_index Не используется.
 */
void esp::vga::vga_direct_controller::raw_fill_row(int y, int x1, int x2, std::uint8_t color_index)
{
}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param y Не используется.
 * @param x1 Не используется.
 * @param x2 Не используется.
 */
void esp::vga::vga_direct_controller::raw_invert_row(int y, int x1, int x2) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param x1 Не используется.
 * @param x2 Не используется.
 * @param src_y Не используется.
 * @param dst_y Не используется.
 */
void esp::vga::vga_direct_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param y_a Не используется.
 * @param y_b Не используется.
 * @param x1 Не используется.
 * @param x2 Не используется.
 */
void esp::vga::vga_direct_controller::swap_rows(int y_a, int y_b, int x1, int x2) {}

/**
 * @brief Ничего не делает: изображение рисует функция рисования строк.
 * @param x1 Не используется.
 * @param y1 Не используется.
 * @param x2 Не используется.
 * @param y2 Не используется.
 * @param color Не используется.
 */
void esp::vga::vga_direct_controller::abs_draw_line(int x1, int y1, int x2, int y2,
                                                    esp::gfx::rgb888 color)
{
}

/**
 * @brief Обработчик прерывания I2S1: вызывает функцию рисования для следующих строк.
 *
 * Работает в IRAM.
 *
 * @param arg Контроллер.
 */
void IRAM_ATTR esp::vga::vga_direct_controller::isr_handler(void *arg)
{
    if (I2S1.int_st.out_eof)
    {
        auto *const self = static_cast<esp::vga::vga_direct_controller *>(arg);

        const auto view_port_height = self->_view_port_height;

        const auto desc = reinterpret_cast<volatile lldesc_t *>(I2S1.out_eof_des_addr);

        if (desc == _frame_reset_desc)
        {
            _scan_line = 0;
            _v_sync = false;
        }

        const int lines_count = self->_lines_count;
        const int scan_line = (_scan_line + lines_count / 2) % view_port_height;

        const auto line_index = scan_line & (lines_count - 1);

        self->_draw_scanline_callback(self->_draw_scanline_arg, self->_lines[line_index],
                                      scan_line);

        _scan_line += lines_count / 2;

        if (_scan_line >= view_port_height)
            _v_sync = true;
    }


    I2S1.int_clr.val = I2S1.int_st.val;
}
