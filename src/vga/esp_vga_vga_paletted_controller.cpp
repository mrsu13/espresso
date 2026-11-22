/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <alloca.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/rtc.h"
#if __has_include("esp_private/cache_utils.h")
#   include "esp_private/cache_utils.h"
#else
#   include "esp_spi_flash.h"
#endif
#include "esp_heap_caps.h"

#include "esp_gfx_utils.h"
#include "esp_tools.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_utils.h"
#include "esp_vga_vga_paletted_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


/*************************************************************************************/
/* VGAPalettedController definitions */


volatile uint8_t **vga_paletted_controller::_isr_view_port;
volatile uint8_t **vga_paletted_controller::_isr_view_port_visible;
lldesc_t volatile *vga_paletted_controller::_frame_reset_desc;
volatile int vga_paletted_controller::_scan_line;


vga_paletted_controller::vga_paletted_controller(int lines_count, int columns_quantum,
                                                 esp::gfx::native_pixel_format native_pixel_format,
                                                 int view_port_ratio_div, int view_port_ratio_mul,
                                                 intr_handler_t isr_handler)
    : _lines_count(lines_count)
    , _columns_quantum(columns_quantum)
    , _native_pixel_format(native_pixel_format)
    , _view_port_ratio_div(view_port_ratio_div)
    , _view_port_ratio_mul(view_port_ratio_mul)
    , _isr_handler(isr_handler)
{
    _lines = (volatile uint8_t **)heap_caps_malloc(sizeof(uint8_t *) * _lines_count,
                                                   MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    _palette = (esp::gfx::rgb222 *)heap_caps_malloc(sizeof(esp::gfx::rgb222) * palette_size(),
                                                    MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


vga_paletted_controller::~vga_paletted_controller()
{
    heap_caps_free(_palette);
    heap_caps_free(_lines);
}


void vga_paletted_controller::init()
{
    vga_base_controller::init();

    _double_buffer_over_dma = false;
    _task_processing_primitives = false;
    _process_primitives_on_blank = false;
    _primitive_exec_task = nullptr;
}


void vga_paletted_controller::end()
{
    if (_primitive_exec_task)
    {
        vTaskDelete(_primitive_exec_task);
        _primitive_exec_task = nullptr;
        _task_processing_primitives = false;
    }
    vga_base_controller::end();
}


void vga_paletted_controller::suspend_background_primitive_execution()
{
    vga_base_controller::suspend_background_primitive_execution();
    while (_task_processing_primitives)
        ;
}

// make sure view port height is divisible by m_linesCount, view port width is divisible by m_columnsQuantum
void vga_paletted_controller::check_view_port_size()
{
    _view_port_height &= ~(_lines_count - 1);
    _view_port_width &= ~(_columns_quantum - 1);
}


void vga_paletted_controller::allocate_view_port()
{
    vga_base_controller::allocate_view_port(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL,
                                            _view_port_width / _view_port_ratio_div
                                                * _view_port_ratio_mul);

    for (int i = 0; i < _lines_count; ++i)
        _lines[i] = (uint8_t *)heap_caps_malloc(_view_port_width, MALLOC_CAP_DMA);
}


void vga_paletted_controller::free_view_port()
{
    vga_base_controller::free_view_port();

    for (int i = 0; i < _lines_count; ++i)
    {
        heap_caps_free((void *)_lines[i]);
        _lines[i] = nullptr;
    }
}


void vga_paletted_controller::set_resolution(vga_timings const &timings, int view_port_width,
                                             int view_port_height, bool double_buffered)
{
    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    _isr_view_port = _view_port;
    _isr_view_port_visible = _view_port_visible;

    // fill view port
    for (int i = 0; i < _view_port_height; ++i)
        memset((void *)(_view_port[i]), 0,
               _view_port_width / _view_port_ratio_div * _view_port_ratio_mul);

    setup_default_palette();
    update_rgb2_palette_lut();

    calculate_available_cycles_for_drawings();

    // must be started before interrupt alloc
    start_gpio_stream();

    // ESP_INTR_FLAG_LEVEL1: should be less than PS2Controller interrupt level, necessary when running on the same core
    if (_isr_handle == nullptr)
    {
        esp::core_usage::set_busiest_core(FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE);
        esp::esp_intr_alloc_pinned_to_core(
            ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, _isr_handler, this,
            &_isr_handle, FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }

    if (_primitive_exec_task == nullptr)
    {
        xTaskCreatePinnedToCore(primitive_exec_task, "",
                                FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_STACK_SIZE, this,
                                FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_PRIORITY,
                                &_primitive_exec_task, esp::core_usage::quiet_core());
    }

    resume_background_primitive_execution();
}


void vga_paletted_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                                  bool is_start_of_vert_front_porch, int scan,
                                                  bool is_visible, int visible_row)
{
    if (is_visible)
    {
        buffer->buf = (uint8_t *)_lines[visible_row % _lines_count];

        // generate interrupt every half m_linesCount
        if ((scan == 0 && (visible_row % (_lines_count / 2)) == 0))
        {
            if (visible_row == 0)
                _frame_reset_desc = buffer;
            buffer->eof = 1;
        }
    }
}


int vga_paletted_controller::palette_size()
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


// rebuild m_packedRGB222_to_PaletteIndex
void vga_paletted_controller::update_rgb2_palette_lut()
{
    auto palette_size = this->palette_size();
    for (int r = 0; r < 4; ++r)
        for (int g = 0; g < 4; ++g)
            for (int b = 0; b < 4; ++b)
            {
                double h1, s1, v1;
                rgb222_to_hsv(r, g, b, &h1, &s1, &v1);
                int best_idx = 0;
                int best_dst = 1000000000;
                for (int i = 0; i < palette_size; ++i)
                {
                    double h2, s2, v2;
                    rgb222_to_hsv(_palette[i].r, _palette[i].g, _palette[i].b, &h2, &s2, &v2);
                    double ah = h1 - h2;
                    double as = s1 - s2;
                    double av = v1 - v2;
                    int dst = ah * ah + as * as + av * av;
                    if (dst <= best_dst)
                    { // "<=" to prioritize higher indexes
                        best_idx = i;
                        best_dst = dst;
                        if (best_dst == 0)
                            break;
                    }
                }
                _packed_rgb222_to_palette_index[r | (g << 2) | (b << 4)] = best_idx;
            }
}


// calculates number of CPU cycles usable to draw primitives
void vga_paletted_controller::calculate_available_cycles_for_drawings()
{
    int availtime_us;

    if (_process_primitives_on_blank)
    {
        // allowed time to process primitives is limited to the vertical blank. Slow, but avoid flickering
        availtime_us = ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                            * (_lines_count / 2 + _timings.v_front_porch + _timings.v_sync_pulse
                               + _timings.v_back_porch + _view_port_row));
    }
    else
    {
        // allowed time is the half of an entire frame. Fast, but may flick
        availtime_us = ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                            * (_timings.v_visible_area + _timings.v_front_porch
                               + _timings.v_sync_pulse + _timings.v_back_porch));
        availtime_us /= 2;
    }

    _primitive_exec_timeout_cycles =
        cpu_frequency_mhz() * availtime_us; // at 240Mhz, there are 240 cycles every microsecond
}


// we can use getCycleCount here because primitiveExecTask is pinned to a specific core (so cycle counter is the same)
// getCycleCount() requires 0.07us, while esp_timer_get_time() requires 0.78us
void vga_paletted_controller::primitive_exec_task(void *arg)
{
    auto ctrl = (vga_paletted_controller *)arg;

    while (true)
    {
        if (!ctrl->_primitive_processing_suspended)
        {
            auto start_cycle = ctrl->background_primitive_timeout_enabled() ? cycle_count() : 0;
            esp::gfx::rect update_rect = esp::gfx::rect(SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN);
            ctrl->_task_processing_primitives = true;
            do
            {
                esp::gfx::primitive prim;
                if (ctrl->get_primitive(&prim, 0) == false)
                    break;
                ctrl->exec_primitive(prim, update_rect, false);
                if (ctrl->_primitive_processing_suspended)
                    break;
            } while (!ctrl->background_primitive_timeout_enabled()
                     || (start_cycle + ctrl->_primitive_exec_timeout_cycles > cycle_count()));
            ctrl->show_sprites(update_rect);
            ctrl->_task_processing_primitives = false;
        }

        // wait for vertical sync
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}


void vga_paletted_controller::swap_buffers()
{
    vga_base_controller::swap_buffers();
    _isr_view_port = _view_port;
    _isr_view_port_visible = _view_port_visible;
}


bool vga_paletted_controller::suspend_double_buffering(bool value)
{
    auto prev_value = vga_base_controller::suspend_double_buffering(value);
    if (prev_value != value && is_double_buffered())
    {
        _isr_view_port = _view_port;
        _isr_view_port_visible = _view_port_visible;
    }
    return prev_value;
}


} // end of namespace
