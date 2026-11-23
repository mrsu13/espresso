// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga_paletted_controller.h
 * @brief Объявление основы контроллеров VGA с палитрой esp::vga::vga_paletted_controller.
 */

/**
 * @file esp_vga_vga_paletted_controller.cpp
 * @brief Основа контроллеров VGA с палитрой: строки DMA, задача примитивов, подбор цветов.
 */

#include "esp_vga_vga_paletted_controller.h"

#include "esp_gfx_primitive.h"
#include "esp_gfx_rect.h"
#include "esp_tools.h"
#include "esp_vga_config.h"
#include "esp_vga_utils_p.h"

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <esp_heap_caps.h>
#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/i2s_struct.h>


#pragma GCC optimize("O2")


/**
 * @class esp::vga::vga_paletted_controller
 * @brief Основа контроллеров VGA с палитрой на 2, 4, 8 и 16 цветов.
 *
 * В буфере кадра хранятся номера цветов палитры — от 1 до 4 разрядов на
 * пиксель, поэтому буфер в 2–8 раз меньше, чем у esp::vga::vga_controller.
 * DMA выводит несколько небольших строк (@c _lines_count): обработчик
 * прерывания производного контроллера каждые полстроки DMA переводит
 * номера цветов следующих строк в байты пикселей. Примитивы рисует
 * отдельная задача на другом ядре, пока идёт обратный ход луча или
 * полкадра (set_process_primitives_on_blank()).
 */


volatile std::uint8_t **esp::vga::vga_paletted_controller::_isr_view_port;
volatile std::uint8_t **esp::vga::vga_paletted_controller::_isr_view_port_visible;
lldesc_t volatile *esp::vga::vga_paletted_controller::_frame_reset_desc;
volatile int esp::vga::vga_paletted_controller::_scan_line;


// Открытые

/**
 * @brief Создаёт контроллер с палитрой и выделяет указатели строк DMA и палитру.
 * @param lines_count Число строк DMA, степень двойки; высота области вывода кратна ему.
 * @param columns_quantum Ширина области вывода кратна этому числу пикселей.
 * @param native_pixel_format Формат пикселя буфера кадра.
 * @param view_port_ratio_div Делитель длины строки буфера: ширина / div × mul байтов.
 * @param view_port_ratio_mul Множитель длины строки буфера.
 * @param isr_handler Обработчик прерывания I2S1, который заполняет строки DMA.
 */
esp::vga::vga_paletted_controller::vga_paletted_controller(
    int lines_count, int columns_quantum, esp::gfx::native_pixel_format native_pixel_format,
    int view_port_ratio_div, int view_port_ratio_mul, intr_handler_t isr_handler)
    : _lines_count{lines_count}
    , _columns_quantum{columns_quantum}
    , _native_pixel_format{native_pixel_format}
    , _view_port_ratio_div{view_port_ratio_div}
    , _view_port_ratio_mul{view_port_ratio_mul}
    , _isr_handler{isr_handler}
{
    _lines = static_cast<volatile std::uint8_t **>(heap_caps_malloc(
        sizeof(std::uint8_t *) * _lines_count, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL));
    _palette = static_cast<esp::gfx::rgb222 *>(heap_caps_malloc(
        sizeof(esp::gfx::rgb222) * palette_size(), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
}

/**
 * @brief Освобождает палитру и указатели строк DMA.
 */
esp::vga::vga_paletted_controller::~vga_paletted_controller()
{
    heap_caps_free(_palette);
    heap_caps_free(_lines);
}

/**
 * @brief Останавливает вывод и удаляет задачу примитивов.
 */
void esp::vga::vga_paletted_controller::end()
{
    if (_primitive_exec_task)
    {
        vTaskDelete(_primitive_exec_task);
        _primitive_exec_task = nullptr;
        _task_processing_primitives = false;
    }
    vga_base_controller::end();
}

/**
 * @brief Приостанавливает выполнение примитивов и ждёт, пока задача закончит рисовать.
 */
void esp::vga::vga_paletted_controller::suspend_background_primitive_execution()
{
    vga_base_controller::suspend_background_primitive_execution();
    while (_task_processing_primitives)
    {
    }
}

/**
 * @brief Задаёт развёртку и запускает вывод.
 *
 * Очищает буфер кадра, задаёт палитру по умолчанию, ставит обработчик
 * прерывания I2S1 (уровень 1: ниже, чем у контроллера PS/2 на том же
 * ядре) и создаёт задачу примитивов на другом ядре.
 *
 * @param timings Развёртка.
 * @param view_port_width Ширина области вывода, пиксели; −1 — вся видимая ширина.
 * @param view_port_height Высота области вывода, строки; −1 — вся видимая
 *        высота, если хватит памяти.
 * @param double_buffered @c true — второй буфер того же размера для рисования.
 */
void esp::vga::vga_paletted_controller::set_resolution(const vga_timings &timings,
                                                       int view_port_width, int view_port_height,
                                                       bool double_buffered)
{
    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    _isr_view_port = _view_port;
    _isr_view_port_visible = _view_port_visible;

    for (int i = 0; i < _view_port_height; ++i)
    {
        std::memset(const_cast<std::uint8_t *>(_view_port[i]), 0,
                    _view_port_width / _view_port_ratio_div * _view_port_ratio_mul);
    }

    setup_default_palette();
    update_rgb2_palette_lut();

    calculate_available_cycles_for_drawings();

    // Поток запускается до установки обработчика прерывания.
    start_gpio_stream();

    // Уровень 1 — ниже, чем у контроллера PS/2: они могут работать на одном ядре.
    if (!_isr_handle)
    {
        esp::core_usage::set_busiest_core(esp::VIDEO_TASKS_CORE);
        esp::esp_intr_alloc_pinned_to_core(ETS_I2S1_INTR_SOURCE,
                                           ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, _isr_handler,
                                           this, &_isr_handle, esp::VIDEO_TASKS_CORE);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }

    if (!_primitive_exec_task)
    {
        xTaskCreatePinnedToCore(primitive_exec_task, "", esp::vga::PRIMITIVE_TASK_STACK_SIZE, this,
                                esp::vga::PRIMITIVE_TASK_PRIORITY, &_primitive_exec_task,
                                esp::core_usage::quiet_core());
    }

    resume_background_primitive_execution();
}

/**
 * @brief Возвращает размер палитры.
 * @return 2, 4, 8 или 16 цветов; 0 — формат пикселя без палитры.
 */
int esp::vga::vga_paletted_controller::palette_size()
{
    switch (native_pixel_format())
    {
        case esp::gfx::native_pixel_format::palette2:
            return 2;
        case esp::gfx::native_pixel_format::palette4:
            return 4;
        case esp::gfx::native_pixel_format::palette8:
            return 8;
        case esp::gfx::native_pixel_format::palette16:
            return 16;
        default:
            return 0;
    }
}

/**
 * @brief Приостанавливает или возобновляет двойную буферизацию и обновляет копии указателей строк.
 * @param value @c true — приостановить, @c false — возобновить.
 * @return Прежнее значение.
 */
bool esp::vga::vga_paletted_controller::suspend_double_buffering(bool value)
{
    const bool prev_value = vga_base_controller::suspend_double_buffering(value);
    if (prev_value != value && is_double_buffered())
    {
        _isr_view_port = _view_port;
        _isr_view_port_visible = _view_port_visible;
    }
    return prev_value;
}

// Защищённые

/**
 * @brief Сбрасывает состояние контроллера; его вызывает begin().
 */
void esp::vga::vga_paletted_controller::init()
{
    vga_base_controller::init();

    _double_buffer_over_dma = false;
    _task_processing_primitives = false;
    _process_primitives_on_blank = false;
    _primitive_exec_task = nullptr;
}

/**
 * @brief Для каждого цвета RGB222 находит ближайший цвет палитры.
 *
 * Цвета сравниваются в пространстве HSV; при равных расстояниях
 * выбирается больший номер.
 */
void esp::vga::vga_paletted_controller::update_rgb2_palette_lut()
{
    const int palette_size = this->palette_size();
    for (int r = 0; r < 4; ++r)
    {
        for (int g = 0; g < 4; ++g)
        {
            for (int b = 0; b < 4; ++b)
            {
                double h1, s1, v1;
                esp::vga::p::rgb222_to_hsv(r, g, b, &h1, &s1, &v1);
                int best_idx = 0;
                int best_dst = 1000000000;
                for (int i = 0; i < palette_size; ++i)
                {
                    double h2, s2, v2;
                    esp::vga::p::rgb222_to_hsv(_palette[i].r, _palette[i].g, _palette[i].b, &h2,
                                               &s2, &v2);
                    const double ah = h1 - h2;
                    const double as = s1 - s2;
                    const double av = v1 - v2;
                    const int dst = ah * ah + as * as + av * av;
                    // «<=»: при равных расстояниях — больший номер.
                    if (dst <= best_dst)
                    {
                        best_idx = i;
                        best_dst = dst;
                        if (best_dst == 0)
                            break;
                    }
                }
                _packed_rgb222_to_palette_index[r | (g << 2) | (b << 4)] = best_idx;
            }
        }
    }
}

/**
 * @brief Считает, сколько тактов процессора задача может рисовать примитивы за кадр.
 *
 * Это обратный ход луча (если задано set_process_primitives_on_blank())
 * или половина кадра.
 */
void esp::vga::vga_paletted_controller::calculate_available_cycles_for_drawings()
{
    int availtime_us;

    if (_process_primitives_on_blank)
    {
        // Только обратный ход луча: медленно, но без мерцания.
        availtime_us =
            std::ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                      * (_lines_count / 2 + _timings.v_front_porch + _timings.v_sync_pulse
                         + _timings.v_back_porch + _view_port_row));
    }
    else
    {
        // Половина кадра: быстро, но может мерцать.
        availtime_us = std::ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                                 * (_timings.v_visible_area + _timings.v_front_porch
                                    + _timings.v_sync_pulse + _timings.v_back_porch));
        availtime_us /= 2;
    }

    // На 240 МГц — 240 тактов в микросекунде.
    _primitive_exec_timeout_cycles = esp::vga::p::cpu_frequency_mhz() * availtime_us;
}

/**
 * @brief Задача, рисующая примитивы.
 *
 * Обработчик прерывания будит её в начале кадра; задача рисует примитивы,
 * пока не выйдет время, затем возвращает спрайты. Время отмеряется
 * счётчиком тактов: задача закреплена за одним ядром.
 *
 * @param arg Контроллер.
 */
void esp::vga::vga_paletted_controller::primitive_exec_task(void *arg)
{
    auto *const self = static_cast<esp::vga::vga_paletted_controller *>(arg);

    while (true)
    {
        if (!self->_primitive_processing_suspended)
        {
            const std::uint32_t start_cycle =
                self->background_primitive_timeout_enabled() ? esp::vga::p::cycle_count() : 0;
            esp::gfx::rect update_rect{SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN};
            self->_task_processing_primitives = true;
            do
            {
                esp::gfx::primitive prim;
                if (!self->get_primitive(&prim, 0))
                    break;
                self->exec_primitive(prim, update_rect, false);
                if (self->_primitive_processing_suspended)
                    break;
            } while (!self->background_primitive_timeout_enabled()
                     || (esp::vga::p::cycle_count() - start_cycle
                         < self->_primitive_exec_timeout_cycles));
            self->show_sprites(update_rect);
            self->_task_processing_primitives = false;
        }

        // Ждём начала следующего кадра: задачу будит обработчик прерывания.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

/**
 * @brief Меняет местами буфер рисования и видимый и обновляет копии указателей строк.
 */
void esp::vga::vga_paletted_controller::swap_buffers()
{
    vga_base_controller::swap_buffers();
    _isr_view_port = _view_port;
    _isr_view_port_visible = _view_port_visible;
}

/**
 * @fn void esp::vga::vga_paletted_controller::setup_default_palette()
 * @brief Задаёт палитру по умолчанию.
 */

// Закрытые

/**
 * @brief Выделяет буфер кадра для номеров цветов и строки DMA.
 */
void esp::vga::vga_paletted_controller::allocate_view_port()
{
    vga_base_controller::allocate_view_port(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL,
                                            _view_port_width / _view_port_ratio_div
                                                * _view_port_ratio_mul);

    for (int i = 0; i < _lines_count; ++i)
        _lines[i] = static_cast<std::uint8_t *>(heap_caps_malloc(_view_port_width, MALLOC_CAP_DMA));
}

/**
 * @brief Освобождает буфер кадра и строки DMA.
 */
void esp::vga::vga_paletted_controller::free_view_port()
{
    vga_base_controller::free_view_port();

    for (int i = 0; i < _lines_count; ++i)
    {
        heap_caps_free(const_cast<std::uint8_t *>(_lines[i]));
        _lines[i] = nullptr;
    }
}

/**
 * @brief Округляет высоту области вывода до кратной числу строк DMA, а ширину — до кратной @c _columns_quantum.
 */
void esp::vga::vga_paletted_controller::check_view_port_size()
{
    _view_port_height &= ~(_lines_count - 1);
    _view_port_width &= ~(_columns_quantum - 1);
}

/**
 * @brief Направляет дескриптор видимой строки на строку DMA.
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
void esp::vga::vga_paletted_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                                            bool is_start_of_vert_front_porch,
                                                            int scan, bool is_visible,
                                                            int visible_row)
{
    if (is_visible)
    {
        buffer->buf = const_cast<std::uint8_t *>(_lines[visible_row % _lines_count]);

        // Прерывание — каждые полчисла строк DMA.
        if ((scan == 0 && (visible_row % (_lines_count / 2)) == 0))
        {
            if (visible_row == 0)
                _frame_reset_desc = buffer;
            buffer->eof = 1;
        }
    }
}
