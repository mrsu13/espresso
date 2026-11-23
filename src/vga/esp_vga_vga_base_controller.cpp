// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga_base_controller.h
 * @brief Объявление основы контроллеров VGA esp::vga::vga_base_controller.
 */

/**
 * @file esp_vga_vga_base_controller.cpp
 * @brief Основа контроллеров VGA: развёртка, буферы DMA и вывод через I2S1.
 */

#include "esp_vga_vga_base_controller.h"

#include "esp_current_video_mode.h"
#include "esp_vga_config.h"
#include "esp_vga_utils_p.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>

#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_rom_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_sig_map.h>


#pragma GCC optimize("O2")


/**
 * @class esp::vga::vga_base_controller
 * @brief Основа контроллеров VGA: развёртка, буферы DMA и вывод через I2S1.
 *
 * Кадр выводит модуль I2S1 в режиме LCD (esp::vga::gpio_stream): каждый
 * байт — пиксель с цветом (по 1 или 2 разряда на канал, см. RED_BIT и
 * соседние константы) и разрядами синхроимпульсов. Кольцо дескрипторов
 * DMA описывает весь кадр: видимые строки ссылаются на строки области
 * вывода, гасящие интервалы — на заготовленные пустые строки. Производные
 * контроллеры решают, как хранить пиксели (байт на пиксель, номера цветов
 * палитры, строки, которые рисует приложение), и дополняют дескрипторы
 * (on_setup_dma_buffer()).
 *
 * begin() задаёт выводы, set_resolution() — развёртку:
 * @code
 * esp::vga::vga16_controller display;
 * display.begin();
 * display.set_resolution(esp::vga::VGA_640X480_60HZ);
 * @endcode
 */


// Открытые

/**
 * @brief Создаёт контроллер; выводы и развёртку задают begin() и set_resolution().
 */
esp::vga::vga_base_controller::vga_base_controller() {}

/**
 * @brief Запускает контроллер на 8 цветов: по выводу на канал цвета.
 * @param red_gpio Вывод красного канала.
 * @param green_gpio Вывод зелёного канала.
 * @param blue_gpio Вывод синего канала.
 * @param h_sync_gpio Вывод строчного синхроимпульса.
 * @param v_sync_gpio Вывод кадрового синхроимпульса.
 */
void esp::vga::vga_base_controller::begin(gpio_num_t red_gpio, gpio_num_t green_gpio,
                                          gpio_num_t blue_gpio, gpio_num_t h_sync_gpio,
                                          gpio_num_t v_sync_gpio)
{
    init();

    // Младшие разряды каналов цвета.
    setup_gpio(red_gpio, RED_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(green_gpio, GREEN_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(blue_gpio, BLUE_BIT, GPIO_MODE_OUTPUT);

    // Синхроимпульсы.
    setup_gpio(h_sync_gpio, HSYNC_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(v_sync_gpio, VSYNC_BIT, GPIO_MODE_OUTPUT);

    esp::gfx::rgb222::low_bit_only = true;
    _bits_per_channel = 1;
}

/**
 * @brief Запускает контроллер на 64 цвета: по два вывода на канал цвета.
 * @param red_1gpio Вывод старшего разряда красного канала.
 * @param red_0gpio Вывод младшего разряда красного канала.
 * @param green_1gpio Вывод старшего разряда зелёного канала.
 * @param green_0gpio Вывод младшего разряда зелёного канала.
 * @param blue_1gpio Вывод старшего разряда синего канала.
 * @param blue_0gpio Вывод младшего разряда синего канала.
 * @param h_sync_gpio Вывод строчного синхроимпульса.
 * @param v_sync_gpio Вывод кадрового синхроимпульса.
 */
void esp::vga::vga_base_controller::begin(gpio_num_t red_1gpio, gpio_num_t red_0gpio,
                                          gpio_num_t green_1gpio, gpio_num_t green_0gpio,
                                          gpio_num_t blue_1gpio, gpio_num_t blue_0gpio,
                                          gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio)
{
    begin(red_0gpio, green_0gpio, blue_0gpio, h_sync_gpio, v_sync_gpio);

    // Старшие разряды каналов цвета.
    setup_gpio(red_1gpio, RED_BIT + 1, GPIO_MODE_OUTPUT);
    setup_gpio(green_1gpio, GREEN_BIT + 1, GPIO_MODE_OUTPUT);
    setup_gpio(blue_1gpio, BLUE_BIT + 1, GPIO_MODE_OUTPUT);

    esp::gfx::rgb222::low_bit_only = false;
    _bits_per_channel = 2;
}

/**
 * @brief Запускает контроллер на 64 цвета на выводах TTGO VGA32.
 *
 * Красный канал — GPIO22 и GPIO21, зелёный — GPIO19 и GPIO18, синий —
 * GPIO5 и GPIO4, строчный синхроимпульс — GPIO23, кадровый — GPIO15.
 */
void esp::vga::vga_base_controller::begin()
{
    begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23,
          GPIO_NUM_15);
}

/**
 * @brief Останавливает вывод и освобождает буферы.
 *
 * Приостанавливает выполнение примитивов, останавливает поток DMA и
 * снимает обработчик прерывания.
 */
void esp::vga::vga_base_controller::end()
{
    if (_dma_buffers)
    {
        suspend_background_primitive_execution();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        _gpio_stream.stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (_isr_handle)
        {
            esp_intr_free(_isr_handle);
            _isr_handle = nullptr;
        }
        free_buffers();
    }
}

/**
 * @brief Разбирает модлайн.
 *
 * Формат: «"имя" частота_МГц hdisp hsyncstart hsyncend htotal vdisp
 * vsyncstart vsyncend vtotal», затем необязательные слова:
 * (+HSync | -HSync) (+VSync | -VSync), DoubleScan | QuadScan,
 * FrontPorchBegins | SyncBegins | BackPorchBegins | VisibleBegins,
 * MultiScanBlank. Регистр букв не важен; слова различаются по первой
 * букве. По умолчанию полярность отрицательная, каждая строка выводится
 * один раз, строка начинается с видимой части. Имя режима — не длиннее
 * 21 знака.
 *
 * @param modeline Модлайн.
 * @param timings Куда записать развёртку.
 * @return @c true, если модлайн разобран.
 */
bool esp::vga::vga_base_controller::convert_modeline_to_timings(const char *modeline,
                                                                vga_timings *timings)
{
    float freq;
    int hdisp, hsyncstart, hsyncend, htotal, vdisp, vsyncstart, vsyncend, vtotal;
    char h_sync_pol = 0, v_sync_pol = 0;
    int pos = 0;

    const int count = std::sscanf(modeline, "\"%21[^\"]\" %g %d %d %d %d %d %d %d %d %n",
                                  timings->label, &freq, &hdisp, &hsyncstart, &hsyncend, &htotal,
                                  &vdisp, &vsyncstart, &vsyncend, &vtotal, &pos);

    if (count == 10 && pos > 0)
    {
        timings->frequency = freq * 1000000;
        timings->h_visible_area = hdisp;
        timings->h_front_porch = hsyncstart - hdisp;
        timings->h_sync_pulse = hsyncend - hsyncstart;
        timings->h_back_porch = htotal - hsyncend;
        timings->v_visible_area = vdisp;
        timings->v_front_porch = vsyncstart - vdisp;
        timings->v_sync_pulse = vsyncend - vsyncstart;
        timings->v_back_porch = vtotal - vsyncend;
        timings->h_sync_logic = '-';
        timings->v_sync_logic = '-';
        timings->scan_count = 1;
        timings->multi_scan_black = 0;
        timings->h_starting_block = vga_scan_start::visible_area;

        for (const char *pc = modeline + pos; *pc;)
        {
            if (*pc == ' ')
            {
                ++pc;
                continue;
            }
            switch (*pc)
            {
                // [(+HSync | -HSync) (+VSync | -VSync)]
                case '+':
                case '-':
                    if (!h_sync_pol)
                        timings->h_sync_logic = h_sync_pol = *pc;
                    else if (!v_sync_pol)
                        timings->v_sync_logic = v_sync_pol = *pc;
                    break;
                // DoubleScan
                case 'D':
                case 'd':
                    timings->scan_count = 2;
                    break;
                // QuadScan
                case 'Q':
                case 'q':
                    timings->scan_count = 4;
                    break;
                // FrontPorchBegins
                case 'F':
                case 'f':
                    timings->h_starting_block = vga_scan_start::front_porch;
                    break;
                // SyncBegins
                case 'S':
                case 's':
                    timings->h_starting_block = vga_scan_start::sync;
                    break;
                // BackPorchBegins
                case 'B':
                case 'b':
                    timings->h_starting_block = vga_scan_start::back_porch;
                    break;
                // VisibleBegins
                case 'V':
                case 'v':
                    timings->h_starting_block = vga_scan_start::visible_area;
                    break;
                // MultiScanBlank
                case 'M':
                case 'm':
                    timings->multi_scan_black = 1;
                    break;
                default:
                    return false;
            }
            // Слова различаются по первой букве: остаток слова пропускаем.
            while (*pc && *pc != ' ')
                ++pc;
        }

        return true;
    }
    return false;
}

/**
 * @brief Приостанавливает выполнение примитивов в обработчике прерывания.
 *
 * Вызовы можно вкладывать. Пока выполнение приостановлено, добавление
 * примитива может зависнуть, если часто не вызывать process_primitives().
 */
void esp::vga::vga_base_controller::suspend_background_primitive_execution()
{
    _primitive_processing_suspended = _primitive_processing_suspended + 1;
}

/**
 * @brief Возобновляет выполнение примитивов после suspend_background_primitive_execution().
 */
void esp::vga::vga_base_controller::resume_background_primitive_execution()
{
    _primitive_processing_suspended = std::max(0, _primitive_processing_suspended - 1);
}

/**
 * @brief Задаёт развёртку модлайном.
 *
 * Готовые модлайны — в esp_vga_modelines.h, формат — в
 * convert_modeline_to_timings(). Неверный модлайн ничего не меняет.
 * @code
 * // 640×382, но область вывода — 640×350.
 * display.set_resolution(esp::vga::VGA_640X382_60HZ, 640, 350);
 * @endcode
 *
 * @param modeline Модлайн.
 * @param view_port_width Ширина области вывода, пиксели; −1 — вся видимая ширина.
 * @param view_port_height Высота области вывода, строки; −1 — вся видимая
 *        высота, если хватит памяти.
 * @param double_buffered @c true — второй буфер того же размера для рисования.
 */
void esp::vga::vga_base_controller::set_resolution(const char *modeline, int view_port_width,
                                                   int view_port_height, bool double_buffered)
{
    vga_timings timings;
    if (convert_modeline_to_timings(modeline, &timings))
        set_resolution(timings, view_port_width, view_port_height, double_buffered);
}

/**
 * @brief Задаёт развёртку.
 *
 * Останавливает вывод (end()), выделяет пустые строки, буферы кадра и
 * дескрипторы DMA и заполняет их. Область вывода выравнивается по 4
 * пикселя и ставится в середину экрана; если памяти не хватает, её высота
 * уменьшается.
 *
 * @param timings Развёртка.
 * @param view_port_width Ширина области вывода, пиксели; −1 — вся видимая ширина.
 * @param view_port_height Высота области вывода, строки; −1 — вся видимая
 *        высота, если хватит памяти.
 * @param double_buffered @c true — второй буфер того же размера для рисования.
 */
void esp::vga::vga_base_controller::set_resolution(const vga_timings &timings, int view_port_width,
                                                   int view_port_height, bool double_buffered)
{
    // На случай, если развёртка уже задана.
    end();

    _timings = timings;

    set_screen_size(_timings.h_visible_area, _timings.v_visible_area);

    set_double_buffered(double_buffered);

    _hv_sync = pack_hv_sync(false, false);

    _h_line_size = _timings.h_front_porch + _timings.h_sync_pulse + _timings.h_back_porch
        + _timings.h_visible_area;

    _h_blank_line_with_v_sync =
        static_cast<std::uint8_t *>(heap_caps_malloc(_h_line_size, MALLOC_CAP_DMA));
    _h_blank_line = static_cast<std::uint8_t *>(heap_caps_malloc(_h_line_size, MALLOC_CAP_DMA));

    // Ширина области вывода кратна 4 пикселям: строка — целые 32-разрядные слова.
    _view_port_width = ~3
        & (view_port_width <= 0 || view_port_width >= _timings.h_visible_area
               ? _timings.h_visible_area
               : view_port_width);
    _view_port_height = view_port_height <= 0 || view_port_height >= _timings.v_visible_area
        ? _timings.v_visible_area
        : view_port_height;

    check_view_port_size();

    // Область вывода — в середине экрана, с выравниванием по 4 пикселя.
    _view_port_col = (_timings.h_visible_area - _view_port_width) / 2;
    _view_port_row = (_timings.v_visible_area - _view_port_height) / 2;
    _view_port_col = _view_port_col & ~3;
    _view_port_row = _view_port_row & ~3;

    _raw_frame_height = _timings.v_visible_area + _timings.v_front_porch + _timings.v_sync_pulse
        + _timings.v_back_porch;

    set_dma_buffers_count(calc_required_dma_buffers_count(_view_port_height));

    allocate_view_port();

    // Если памяти не хватило, область вывода стала ниже: подгоняем её
    // и освобождаем лишние дескрипторы.
    check_view_port_size();
    set_dma_buffers_count(calc_required_dma_buffers_count(_view_port_height));

    fill_vert_buffers(0);
    fill_horiz_buffers(0);

    reset_paint_state();

    if (_double_buffer_over_dma)
        _dma_buffers_head->qe.stqe_next = const_cast<lldesc_t *>(&_dma_buffers_visible[0]);
}

/**
 * @brief Сдвигает изображение, перенося пиксели и строки между гасящими интервалами.
 * @param offset_x Сдвиг по горизонтали, пиксели: < 0 — влево, > 0 — вправо.
 * @param offset_y Сдвиг по вертикали, строки: < 0 — вверх, > 0 — вниз.
 */
void esp::vga::vga_base_controller::move_screen(int offset_x, int offset_y)
{
    suspend_background_primitive_execution();
    fill_vert_buffers(offset_y);
    fill_horiz_buffers(offset_x);
    resume_background_primitive_execution();
}

/**
 * @brief Сжимает или растягивает изображение, удлиняя или укорачивая гасящие интервалы.
 *
 * Развёртка задаётся заново. Например, сжать на 8 пикселей с каждой
 * стороны и сдвинуть влево:
 * @code
 * display.shrink_screen(2, 0);
 * display.move_screen(-8, 0);
 * @endcode
 *
 * @param shrink_x По горизонтали, шаги по 4 пикселя с каждой стороны: > 0 —
 *        сжать, < 0 — растянуть.
 * @param shrink_y По вертикали, строки с каждой стороны: > 0 — сжать, < 0 —
 *        растянуть.
 */
void esp::vga::vga_base_controller::shrink_screen(int shrink_x, int shrink_y)
{
    vga_timings *const curr_timings = resolution_timings();

    curr_timings->h_back_porch = std::max(curr_timings->h_back_porch + 4 * shrink_x, 4);
    curr_timings->h_front_porch = std::max(curr_timings->h_front_porch + 4 * shrink_x, 4);

    curr_timings->v_back_porch = std::max(curr_timings->v_back_porch + shrink_y, 1);
    curr_timings->v_front_porch = std::max(curr_timings->v_front_porch + shrink_y, 1);

    set_resolution(*curr_timings, _view_port_width, _view_port_height, is_double_buffered());
}

/**
 * @brief Приостанавливает или возобновляет двойную буферизацию.
 *
 * Пока она приостановлена, рисование идёт прямо в видимый буфер.
 *
 * @param value @c true — приостановить, @c false — возобновить.
 * @return Прежнее значение.
 */
bool esp::vga::vga_base_controller::suspend_double_buffering(bool value)
{
    const bool prev_value = esp::gfx::bitmapped_display_controller::suspend_double_buffering(value);
    if (prev_value != value && is_double_buffered())
    {
        if (value)
        {
            _view_ports[0] = _view_port;
            _view_ports[1] = _view_port_visible;
            _view_port = _view_port_visible;
        }
        else
        {
            _view_port = _view_ports[0];
            _view_port_visible = _view_ports[1];
        }
    }
    return prev_value;
}

// Защищённые

/**
 * @brief Подключает вывод к разряду выхода I2S1.
 * @param gpio Вывод.
 * @param bit Разряд байта пикселя.
 * @param mode Режим вывода.
 */
void esp::vga::vga_base_controller::setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode)
{
    esp::vga::p::configure_gpio(gpio, mode);
    esp_rom_gpio_connect_out_signal(gpio, I2S1O_DATA_OUT0_IDX + bit, false, false);
}

/**
 * @brief Запускает вывод кадра: поток DMA с частотой пикселей развёртки.
 */
void esp::vga::vga_base_controller::start_gpio_stream()
{
    _gpio_stream.play(_timings.frequency, _dma_buffers);
}

/**
 * @brief Освобождает пустые строки, буферы кадра и дескрипторы DMA.
 */
void esp::vga::vga_base_controller::free_buffers()
{
    if (_dma_buffers_count > 0)
    {
        heap_caps_free(const_cast<std::uint8_t *>(_h_blank_line_with_v_sync));
        heap_caps_free(const_cast<std::uint8_t *>(_h_blank_line));

        free_view_port();

        set_dma_buffers_count(0);
    }
}

/**
 * @brief Освобождает буферы кадра.
 */
void esp::vga::vga_base_controller::free_view_port()
{
    if (_view_port_memory_pool)
    {
        for (std::uint8_t **pool_ptr = _view_port_memory_pool; *pool_ptr; ++pool_ptr)
            heap_caps_free(*pool_ptr);
        heap_caps_free(_view_port_memory_pool);
        _view_port_memory_pool = nullptr;
    }
    for (int i = 0; i < 2; ++i)
    {
        if (_view_ports[i])
        {
            heap_caps_free(_view_ports[i]);
            _view_ports[i] = nullptr;
        }
    }
    _view_port = _view_port_visible = nullptr;
}

/**
 * @brief Сбрасывает состояние контроллера и готовит поток DMA; его вызывает begin().
 */
void esp::vga::vga_base_controller::init()
{
    esp::current_video_mode::set(esp::video_mode::vga);

    _dma_buffers = nullptr;
    _dma_buffers_count = 0;
    _dma_buffers_head = nullptr;
    _dma_buffers_visible = nullptr;
    _primitive_processing_suspended = 1; // Приостановлено.
    _isr_handle = nullptr;
    _double_buffer_over_dma = false;
    _view_ports[0] = _view_ports[1] = nullptr;
    _view_port = nullptr;
    _view_port_visible = nullptr;
    _view_port_memory_pool = nullptr;

    _gpio_stream.begin();
}

/**
 * @brief Меняет число дескрипторов DMA, сохраняя уже заданные указатели.
 *
 * Дескрипторы замыкаются в кольцо. При двойной буферизации через DMA так
 * же ведётся вторая цепочка, а кольцо замыкается через головной
 * дескриптор.
 *
 * @param buffers_count Число дескрипторов; 0 — освободить их.
 * @return @c false, если не хватило памяти.
 */
bool esp::vga::vga_base_controller::set_dma_buffers_count(int buffers_count)
{
    if (buffers_count == 0)
    {
        if (_dma_buffers_visible && _dma_buffers_visible != _dma_buffers)
            heap_caps_free(const_cast<lldesc_t *>(_dma_buffers_visible));
        heap_caps_free(const_cast<lldesc_t *>(_dma_buffers));
        _dma_buffers = nullptr;
        _dma_buffers_visible = nullptr;
        _dma_buffers_count = 0;
        return true;
    }

    if (buffers_count != _dma_buffers_count)
    {
        // Головной дескриптор кольца при двойной буферизации через DMA.
        if (_double_buffer_over_dma && !_dma_buffers_head)
        {
            _dma_buffers_head =
                static_cast<lldesc_t *>(heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA));
            _dma_buffers_head->eof = 0;
            _dma_buffers_head->sosf = 0;
            _dma_buffers_head->offset = 0;
            _dma_buffers_head->owner = 1;
            _dma_buffers_head->size = 0;
            _dma_buffers_head->length = 0;
            // Любой правильный адрес: с nullptr DMA останавливается.
            _dma_buffers_head->buf = _h_blank_line;
            // Задаётся перед первым кадром.
            _dma_buffers_head->qe.stqe_next = nullptr;
        }

        // Выделяем или перевыделяем дескрипторы.
        _dma_buffers = static_cast<lldesc_t *>(
            heap_caps_realloc(const_cast<lldesc_t *>(_dma_buffers),
                              buffers_count * sizeof(lldesc_t), MALLOC_CAP_DMA));
        if (_double_buffer_over_dma && is_double_buffered())
        {
            _dma_buffers_visible = static_cast<lldesc_t *>(
                heap_caps_realloc(const_cast<lldesc_t *>(_dma_buffers_visible),
                                  buffers_count * sizeof(lldesc_t), MALLOC_CAP_DMA));
        }
        else
            _dma_buffers_visible = _dma_buffers;
        if (!_dma_buffers || !_dma_buffers_visible)
            return false;

        lldesc_t volatile *const buffers_head =
            _dma_buffers_head ? _dma_buffers_head : &_dma_buffers[0];

        for (int i = 0; i < buffers_count; ++i)
        {
            _dma_buffers[i].eof = 0;
            _dma_buffers[i].sosf = 0;
            _dma_buffers[i].offset = 0;
            _dma_buffers[i].owner = 1;
            _dma_buffers[i].qe.stqe_next = const_cast<lldesc_t *>(
                i == buffers_count - 1 ? buffers_head : &_dma_buffers[i + 1]);
            if (_double_buffer_over_dma && is_double_buffered())
            {
                _dma_buffers_visible[i].eof = 0;
                _dma_buffers_visible[i].sosf = 0;
                _dma_buffers_visible[i].offset = 0;
                _dma_buffers_visible[i].owner = 1;
                _dma_buffers_visible[i].qe.stqe_next = const_cast<lldesc_t *>(
                    i == buffers_count - 1 ? buffers_head : &_dma_buffers_visible[i + 1]);
            }
        }

        _dma_buffers_count = buffers_count;
    }

    return true;
}

/**
 * @brief Собирает разряды синхроимпульсов с учётом их полярности.
 * @param h_sync @c true — строчный синхроимпульс.
 * @param v_sync @c true — кадровый синхроимпульс.
 * @return Разряды HSYNC_BIT и VSYNC_BIT байта пикселя.
 */
std::uint8_t IRAM_ATTR esp::vga::vga_base_controller::pack_hv_sync(bool h_sync, bool v_sync)
{
    const std::uint8_t hsync_value =
        (_timings.h_sync_logic == '+' ? (h_sync ? 1 : 0) : (h_sync ? 0 : 1));
    const std::uint8_t vsync_value =
        (_timings.v_sync_logic == '+' ? (v_sync ? 1 : 0) : (v_sync ? 0 : 1));
    return (vsync_value << VSYNC_BIT) | (hsync_value << HSYNC_BIT);
}

/**
 * @brief Собирает байт пикселя из цвета и заданных синхроимпульсов.
 * @param rgb Цвет, по 2 разряда на канал.
 * @param h_sync @c true — строчный синхроимпульс.
 * @param v_sync @c true — кадровый синхроимпульс.
 * @return Байт пикселя.
 */
std::uint8_t IRAM_ATTR esp::vga::vga_base_controller::prepare_pixel_with_sync(esp::gfx::rgb222 rgb,
                                                                              bool h_sync,
                                                                              bool v_sync)
{
    return pack_hv_sync(h_sync, v_sync) | (rgb.b << BLUE_BIT) | (rgb.g << GREEN_BIT)
        | (rgb.r << RED_BIT);
}

/**
 * @brief Перестраивает дескрипторы DMA всего кадра.
 *
 * По вертикали кадр — видимая часть, передний гасящий интервал,
 * синхроимпульс и задний гасящий интервал; каждая строка повторяется
 * @c vga_timings::scan_count раз. Строка области вывода собирается из
 * пустых участков и строки буфера в порядке @c vga_timings::h_starting_block.
 *
 * @param offset_y Сдвиг кадра по вертикали, строки: переносит строки из
 *        одного гасящего интервала в другой (в каждом остаётся хотя бы одна).
 */
void esp::vga::vga_base_controller::fill_vert_buffers(int offset_y)
{
    const std::int16_t porch_sum = _timings.v_front_porch + _timings.v_back_porch;
    _timings.v_front_porch = std::max(1, _timings.v_front_porch - offset_y);
    _timings.v_back_porch = std::max(1, porch_sum - _timings.v_front_porch);
    _timings.v_front_porch = porch_sum - _timings.v_back_porch;

    // По вертикали: видимая часть, передний гасящий интервал, синхроимпульс,
    // задний гасящий интервал.
    const int v_visible_area_pos = 0;
    const int v_front_porch_pos = v_visible_area_pos + _timings.v_visible_area;
    const int v_sync_pos = v_front_porch_pos + _timings.v_front_porch;
    const int v_back_porch_pos = v_sync_pos + _timings.v_sync_pulse;

    const int right_pad_size = _timings.h_visible_area - _view_port_width - _view_port_col;

    for (int line = 0, dma_buf_idx = 0; line < _raw_frame_height; ++line)
    {
        const bool is_v_visible_area = (line < v_front_porch_pos);
        const bool is_v_front_porch = (line >= v_front_porch_pos && line < v_sync_pos);
        const bool is_v_sync = (line >= v_sync_pos && line < v_back_porch_pos);
        const bool is_v_back_porch = (line >= v_back_porch_pos);

        for (int scan = 0; scan < _timings.scan_count; ++scan)
        {
            const bool is_start_of_vert_front_porch = (line == v_front_porch_pos && scan == 0);

            if (is_v_sync)
            {
                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line_with_v_sync, _h_line_size, scan,
                                     is_start_of_vert_front_porch);
            }
            else if (is_v_front_porch || is_v_back_porch)
            {
                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _h_line_size, scan,
                                     is_start_of_vert_front_porch);
            }
            else if (is_v_visible_area)
            {
                const int visible_area_line = line - v_visible_area_pos;
                const bool is_viewport = visible_area_line >= _view_port_row
                    && visible_area_line < _view_port_row + _view_port_height;
                const int h_invisible_area_size = _h_line_size - _timings.h_visible_area;

                if (is_viewport)
                {
                    // Строка области вывода.
                    switch (_timings.h_starting_block)
                    {
                        case vga_scan_start::front_porch:
                            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 h_invisible_area_size + _view_port_col, scan,
                                                 is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            if (right_pad_size > 0)
                                set_dma_buffer_blank(
                                    dma_buf_idx++, _h_blank_line + h_invisible_area_size,
                                    right_pad_size, scan, is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::sync:
                            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 _timings.h_sync_pulse + _timings.h_back_porch
                                                     + _view_port_col,
                                                 scan, is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(dma_buf_idx++,
                                                 _h_blank_line + _h_line_size
                                                     - _timings.h_front_porch - right_pad_size,
                                                 _timings.h_front_porch + right_pad_size, scan,
                                                 is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::back_porch:
                            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 _timings.h_back_porch + _view_port_col, scan,
                                                 is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(
                                dma_buf_idx++,
                                _h_blank_line + _h_line_size - _timings.h_front_porch
                                    - _timings.h_sync_pulse - right_pad_size,
                                _timings.h_front_porch + _timings.h_sync_pulse + right_pad_size,
                                scan, is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::visible_area:
                            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
                            if (_view_port_col > 0)
                                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _view_port_col,
                                                     scan, is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(dma_buf_idx++,
                                                 _h_blank_line + _timings.h_visible_area
                                                     - right_pad_size,
                                                 h_invisible_area_size + right_pad_size, scan,
                                                 is_start_of_vert_front_porch);
                            break;
                    }
                }
                else
                {
                    // Строка вне области вывода.
                    set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _h_line_size, scan,
                                         is_start_of_vert_front_porch);
                }
            }
        }
    }
}

/**
 * @brief Заполняет пустые строки и ставит в них строчный синхроимпульс.
 * @param offset_x Сдвиг по горизонтали, пиксели: < 0 — влево, > 0 — вправо;
 *        переносит пиксели из одного гасящего интервала в другой (в каждом
 *        остаётся хотя бы 8).
 */
void esp::vga::vga_base_controller::fill_horiz_buffers(int offset_x)
{
    // Сначала вся строка без строчного синхроимпульса.
    fill(_h_blank_line, 0, _h_line_size, 0, 0, 0, false, false);
    fill(_h_blank_line_with_v_sync, 0, _h_line_size, 0, 0, 0, false, true);

    // Затем синхроимпульс на своём месте.
    const std::int16_t porch_sum = _timings.h_front_porch + _timings.h_back_porch;
    _timings.h_front_porch = std::max(8, _timings.h_front_porch - offset_x);
    _timings.h_back_porch = std::max(8, porch_sum - _timings.h_front_porch);
    _timings.h_front_porch = porch_sum - _timings.h_back_porch;

    int sync_pos = 0;

    switch (_timings.h_starting_block)
    {
        case vga_scan_start::front_porch:
            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
            sync_pos = _timings.h_front_porch;
            break;
        case vga_scan_start::sync:
            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
            sync_pos = 0;
            break;
        case vga_scan_start::back_porch:
            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
            sync_pos = _timings.h_back_porch + _timings.h_visible_area + _timings.h_front_porch;
            break;
        case vga_scan_start::visible_area:
            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
            sync_pos = _timings.h_visible_area + _timings.h_front_porch;
            break;
    }

    fill(_h_blank_line, sync_pos, _timings.h_sync_pulse, 0, 0, 0, true, false);
    fill(_h_blank_line_with_v_sync, sync_pos, _timings.h_sync_pulse, 0, 0, 0, true, true);
}

/**
 * @brief Заполняет участок строки одним цветом с заданными синхроимпульсами.
 * @param buffer Строка; её размер кратен 4 байтам.
 * @param start_pos Первый пиксель.
 * @param length Число пикселей.
 * @param red Красный канал: 0…3.
 * @param green Зелёный канал: 0…3.
 * @param blue Синий канал: 0…3.
 * @param h_sync @c true — строчный синхроимпульс.
 * @param v_sync @c true — кадровый синхроимпульс.
 * @return Пиксель после участка: @p start_pos + @p length.
 */
int esp::vga::vga_base_controller::fill(std::uint8_t volatile *buffer, int start_pos, int length,
                                        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                                        bool h_sync, bool v_sync)
{
    const std::uint8_t pattern =
        prepare_pixel_with_sync(esp::gfx::rgb222{red, green, blue}, h_sync, v_sync);
    for (int i = 0; i < length; ++i, ++start_pos)
        pixel_in_row(buffer, start_pos) = pattern;
    return start_pos;
}

/**
 * @brief Считает, сколько дескрипторов DMA нужно кадру.
 *
 * Строка области вывода занимает два или три дескриптора: пустой
 * участок, строку буфера и, если нужно, ещё один пустой участок.
 *
 * @param view_port_height Высота области вывода, строки.
 * @return Число дескрипторов.
 */
int esp::vga::vga_base_controller::calc_required_dma_buffers_count(int view_port_height)
{
    const int right_pad_size = _timings.h_visible_area - _view_port_width - _view_port_col;
    int buffers_count = _timings.scan_count * (_raw_frame_height + view_port_height);

    switch (_timings.h_starting_block)
    {
        case vga_scan_start::front_porch:
            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
            buffers_count += _timings.scan_count * (right_pad_size > 0 ? view_port_height : 0);
            break;
        case vga_scan_start::sync:
            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
            buffers_count += _timings.scan_count * view_port_height;
            break;
        case vga_scan_start::back_porch:
            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
            buffers_count += _timings.scan_count * view_port_height;
            break;
        case vga_scan_start::visible_area:
            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
            buffers_count += _timings.scan_count * (_view_port_col > 0 ? view_port_height : 0);
            break;
    }

    return buffers_count;
}

/**
 * @brief Сообщает, выводится ли повтор строки чёрным (MultiScanBlank).
 *
 * Работает, только если строка начинается с переднего гасящего интервала.
 *
 * @param scan Номер повтора строки, 0 — сама строка.
 * @return @c true, если повтор чёрный.
 */
bool esp::vga::vga_base_controller::is_multi_scan_black_line(int scan)
{
    return (scan > 0 && _timings.multi_scan_black == 1
            && _timings.h_starting_block == vga_scan_start::front_porch);
}

/**
 * @brief Направляет дескриптор DMA на пустой участок строки.
 * @param index Дескриптор.
 * @param address Участок: память из MALLOC_CAP_DMA или часть другого такого буфера.
 * @param length Длина, байты; не больше 4092.
 * @param scan Номер повтора строки.
 * @param is_start_of_vert_front_porch Первый ли это дескриптор переднего
 *        гасящего интервала кадра.
 */
void esp::vga::vga_base_controller::set_dma_buffer_blank(int index, void volatile *address,
                                                         int length, int scan,
                                                         bool is_start_of_vert_front_porch)
{
    const int size = (length + 3) & (~3);
    _dma_buffers[index].eof = 0;
    _dma_buffers[index].size = size;
    _dma_buffers[index].length = length;
    _dma_buffers[index].buf = static_cast<std::uint8_t *>(const_cast<void *>(address));
    on_setup_dma_buffer(&_dma_buffers[index], is_start_of_vert_front_porch, scan, false, 0);
    if (_double_buffer_over_dma && is_double_buffered())
    {
        _dma_buffers_visible[index].eof = 0;
        _dma_buffers_visible[index].size = size;
        _dma_buffers_visible[index].length = length;
        _dma_buffers_visible[index].buf = static_cast<std::uint8_t *>(const_cast<void *>(address));
        on_setup_dma_buffer(&_dma_buffers_visible[index], is_start_of_vert_front_porch, scan, false,
                            0);
    }
}

/**
 * @brief Направляет дескриптор DMA одной цепочки на строку области вывода.
 *
 * Повтор строки при MultiScanBlank направляется на пустую видимую часть.
 *
 * @param index Дескриптор.
 * @param row Строка области вывода.
 * @param scan Номер повтора строки.
 * @param view_port Строки буфера кадра или @c nullptr.
 * @param on_visible_dma @c true — дескриптор видимой цепочки.
 */
void esp::vga::vga_base_controller::set_dma_buffer_view(int index, int row, int scan,
                                                        volatile std::uint8_t **view_port,
                                                        bool on_visible_dma)
{
    std::uint8_t *buffer_ptr = nullptr;
    if (is_multi_scan_black_line(scan))
    {
        // Пустая видимая часть — в конце пустой строки, если строка
        // начинается с гасящих интервалов и синхроимпульса.
        buffer_ptr =
            const_cast<std::uint8_t *>(_h_blank_line + _h_line_size - _timings.h_visible_area);
    }
    else if (view_port)
        buffer_ptr = const_cast<std::uint8_t *>(view_port[row]);
    lldesc_t volatile *dma_buffers = on_visible_dma ? _dma_buffers_visible : _dma_buffers;
    dma_buffers[index].size = (_view_port_width + 3) & (~3);
    dma_buffers[index].length = _view_port_width;
    dma_buffers[index].buf = buffer_ptr;
}

/**
 * @brief Направляет дескриптор DMA на строку области вывода в обеих цепочках.
 *
 * О каждом дескрипторе сообщается производному контроллеру
 * (on_setup_dma_buffer()), кроме чёрных повторов строк.
 *
 * @param index Дескриптор.
 * @param row Строка области вывода.
 * @param scan Номер повтора строки.
 * @param is_start_of_vert_front_porch Первый ли это дескриптор переднего
 *        гасящего интервала кадра.
 */
void esp::vga::vga_base_controller::set_dma_buffer_view(int index, int row, int scan,
                                                        bool is_start_of_vert_front_porch)
{
    set_dma_buffer_view(index, row, scan, _view_port, false);
    if (!is_multi_scan_black_line(scan))
        on_setup_dma_buffer(&_dma_buffers[index], is_start_of_vert_front_porch, scan, true, row);
    if (is_double_buffered())
    {
        set_dma_buffer_view(index, row, scan, _view_port_visible, true);
        if (!is_multi_scan_black_line(scan))
            on_setup_dma_buffer(&_dma_buffers_visible[index], is_start_of_vert_front_porch, scan,
                                true, row);
    }
}

/**
 * @brief Возвращает данные дескриптора DMA.
 * @param index Дескриптор.
 * @param length Куда записать длину данных, байты.
 * @return Адрес данных.
 */
void volatile *esp::vga::vga_base_controller::get_dma_buffer(int index, int *length)
{
    *length = _dma_buffers[index].length;
    return const_cast<volatile std::uint8_t *>(_dma_buffers[index].buf);
}

/**
 * @brief Выделяет буферы кадра как можно меньшим числом блоков памяти.
 *
 * В куче остаётся блок не меньше esp::vga::MIN_FREE_LARGEST_BLOCK байтов;
 * если памяти не хватает, высота области вывода уменьшается до
 * выделенной.
 *
 * @param alloc_caps Вид памяти для heap_caps_malloc().
 * @param rowlen Длина строки буфера, байты.
 *
 * @bug Не проверяет, выделены ли массивы указателей на строки и блоки.
 */
void esp::vga::vga_base_controller::allocate_view_port(std::uint32_t alloc_caps, int rowlen)
{
    int lines_count[esp::vga::VIEW_PORT_MEMORY_POOL_COUNT]; // Строк в каждом блоке.
    int pools_count = 0;
    int remaining_lines = _view_port_height;
    _view_port_height = 0; // Пересчитывается по выделенному.

    if (is_double_buffered())
        remaining_lines *= 2;

    // Блоки памяти — как можно больше, но с запасом в куче.
    _view_port_memory_pool = static_cast<std::uint8_t **>(heap_caps_malloc(
        sizeof(std::uint8_t *) * (esp::vga::VIEW_PORT_MEMORY_POOL_COUNT + 1), MALLOC_CAP_32BIT));
    while (remaining_lines > 0 && pools_count < esp::vga::VIEW_PORT_MEMORY_POOL_COUNT)
    {
        const int largest_block = heap_caps_get_largest_free_block(alloc_caps);
        if (largest_block < esp::vga::MIN_FREE_LARGEST_BLOCK)
            break;
        lines_count[pools_count] = std::max(
            1,
            std::min(remaining_lines, (largest_block - esp::vga::MIN_FREE_LARGEST_BLOCK) / rowlen));
        _view_port_memory_pool[pools_count] = static_cast<std::uint8_t *>(
            heap_caps_malloc(lines_count[pools_count] * rowlen, alloc_caps));
        if (!_view_port_memory_pool[pools_count])
            break;
        remaining_lines -= lines_count[pools_count];
        _view_port_height += lines_count[pools_count];
        ++pools_count;
    }
    _view_port_memory_pool[pools_count] = nullptr;

    // Указатели на строки буферов кадра.
    if (is_double_buffered())
    {
        _view_port_height /= 2;
        _view_port_visible = _view_ports[1] = static_cast<volatile std::uint8_t **>(
            heap_caps_malloc(sizeof(std::uint8_t *) * _view_port_height,
                             MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL));
    }
    _view_port = _view_ports[0] = static_cast<volatile std::uint8_t **>(heap_caps_malloc(
        sizeof(std::uint8_t *) * _view_port_height, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL));
    if (!is_double_buffered())
        _view_port_visible = _view_port;
    for (int p = 0, l = 0; p < pools_count; ++p)
    {
        std::uint8_t *pool = _view_port_memory_pool[p];
        for (int i = 0; i < lines_count[p]; ++i)
        {
            if (l + i < _view_port_height)
                _view_port[l + i] = pool;
            else
                _view_port_visible[l + i - _view_port_height] = pool; // Только при двойной.
            pool += rowlen;
        }
        l += lines_count[p];
    }
}

/**
 * @brief Меняет местами буфер рисования и видимый.
 *
 * При двойной буферизации через DMA переключает и цепочку дескрипторов.
 */
void IRAM_ATTR esp::vga::vga_base_controller::swap_buffers()
{
    std::swap(_view_port, _view_port_visible);
    if (_double_buffer_over_dma)
    {
        std::swap(_dma_buffers, _dma_buffers_visible);
        _dma_buffers_head->qe.stqe_next = const_cast<lldesc_t *>(&_dma_buffers_visible[0]);
    }
}

/**
 * @fn void esp::vga::vga_base_controller::on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan, bool is_visible, int visible_row)
 * @brief Дополняет дескриптор DMA: производный контроллер, например, отмечает
 *        в нём прерывание.
 * @param buffer Дескриптор.
 * @param is_start_of_vert_front_porch Первый ли это дескриптор переднего
 *        гасящего интервала кадра.
 * @param scan Номер повтора строки.
 * @param is_visible @c true — дескриптор строки области вывода.
 * @param visible_row Строка области вывода.
 */

/**
 * @fn void esp::vga::vga_base_controller::allocate_view_port()
 * @brief Выделяет буферы кадра в формате производного контроллера.
 */
