// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


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
#include "esp_vga_vga_direct_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


/*************************************************************************************/
/* VGADirectController definitions */


vga_direct_controller *vga_direct_controller::_instance = nullptr;
volatile int vga_direct_controller::_scan_line;
lldesc_t volatile *vga_direct_controller::_frame_reset_desc;
bool vga_direct_controller::_v_sync;
lldesc_t volatile **vga_direct_controller::_dma_lines = nullptr;


vga_direct_controller::vga_direct_controller(bool auto_run)
    : _lines_count(2)
    , _lines(nullptr)
    , _draw_scanline_callback(nullptr)
    , _auto_run(auto_run)
{
    _instance = this;
}


void vga_direct_controller::init()
{
    vga_base_controller::init();
    _double_buffer_over_dma = false;
}


void vga_direct_controller::allocate_view_port()
{
    _lines = (uint8_t **)heap_caps_malloc(sizeof(uint8_t *) * _lines_count,
                                          MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    _lines[0] = (uint8_t *)heap_caps_malloc(_view_port_width * _lines_count, MALLOC_CAP_DMA);
    for (int i = 1; i < _lines_count; ++i)
        _lines[i] = _lines[0] + i * _view_port_width;
    _dma_lines = new lldesc_t volatile *[_view_port_height];
}


void vga_direct_controller::free_view_port()
{
    vga_base_controller::free_view_port();

    heap_caps_free((void *)_lines[0]);
    heap_caps_free((void *)_lines);
    _lines = nullptr;
    delete _dma_lines;
}


void vga_direct_controller::set_resolution(vga_timings const &timings, int view_port_width,
                                           int view_port_height, bool double_buffered)
{
    // fail if setDrawScanlineCallback() has not been called
    if (!_draw_scanline_callback)
        return;

    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    if (_auto_run)
        run();
}


void vga_direct_controller::run()
{
    // must be started before interrupt alloc
    start_gpio_stream();

    _scan_line = 0;

    // ESP_INTR_FLAG_LEVEL1: should be less than PS2Controller interrupt level, necessary when running on the same core
    if (_isr_handle == nullptr)
    {
        esp::core_usage::set_busiest_core(esp::VIDEO_TASKS_CORE);
        esp::esp_intr_alloc_pinned_to_core(ETS_I2S1_INTR_SOURCE,
                                           ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, isr_handler,
                                           this, &_isr_handle, esp::VIDEO_TASKS_CORE);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }
}


void vga_direct_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                                bool is_start_of_vert_front_porch, int scan,
                                                bool is_visible, int visible_row)
{
    if (is_visible)
    {
        _dma_lines[visible_row] = buffer;

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


void vga_direct_controller::set_scanline_buffer(int scanline, uint8_t volatile *line_buffer)
{
    _dma_lines[scanline]->buf = line_buffer;
}


uint8_t volatile *vga_direct_controller::scanline_buffer(int scanline)
{
    return (uint8_t volatile *)_dma_lines[scanline]->buf;
}


uint8_t volatile *vga_direct_controller::default_scanline_buffer(int scanline)
{
    return _lines[scanline % _lines_count];
}


void vga_direct_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                         esp::gfx::rect &update_rect)
{
}


void vga_direct_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color) {}


void vga_direct_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color) {}


void vga_direct_controller::raw_fill_row(int y, int x1, int x2, uint8_t color_index) {}


void vga_direct_controller::raw_invert_row(int y, int x1, int x2) {}


void vga_direct_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y) {}


void vga_direct_controller::swap_rows(int y_a, int y_b, int x1, int x2) {}


void vga_direct_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect) {}


void vga_direct_controller::clear(esp::gfx::rect &update_rect) {}


void vga_direct_controller::v_scroll(int scroll, esp::gfx::rect &update_rect) {}


void vga_direct_controller::h_scroll(int scroll, esp::gfx::rect &update_rect) {}


void vga_direct_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                       esp::gfx::glyph_options glyph_options,
                                       esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                                       esp::gfx::rect &update_rect)
{
}


void vga_direct_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect) {}


void vga_direct_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect) {}


void vga_direct_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect) {}


void vga_direct_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf) {}


void vga_direct_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                   esp::gfx::bitmap const *bitmap, int x1, int y1,
                                                   int x_count, int y_count)
{
}


void vga_direct_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                 esp::gfx::bitmap const *bitmap,
                                                 void *save_background, int x1, int y1, int x_count,
                                                 int y_count)
{
}


void vga_direct_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                     esp::gfx::bitmap const *bitmap,
                                                     void *save_background, int x1, int y1,
                                                     int x_count, int y_count)
{
}


void vga_direct_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                     esp::gfx::bitmap const *bitmap,
                                                     void *save_background, int x1, int y1,
                                                     int x_count, int y_count)
{
}


void IRAM_ATTR vga_direct_controller::isr_handler(void *arg)
{
#if ESP_VGA_PERFORMANCE_CHECK
    auto s1 = cycle_count();
#endif

    if (I2S1.int_st.out_eof)
    {

        auto ctrl = (vga_direct_controller *)arg;

        auto view_port_height = ctrl->_view_port_height;

        auto desc = (volatile lldesc_t *)I2S1.out_eof_des_addr;

        if (desc == _frame_reset_desc)
        {
            _scan_line = 0;
            _v_sync = false;
        }

        int lines_count = ctrl->_lines_count;
        int scan_line = (_scan_line + lines_count / 2) % view_port_height;

        const auto line_index = scan_line & (lines_count - 1);

        ctrl->_draw_scanline_callback(ctrl->_draw_scanline_arg,
                                      (uint8_t *)(ctrl->_lines[line_index]), scan_line);

        _scan_line += lines_count / 2;

        if (_scan_line >= view_port_height)
            _v_sync = true;
    }

#if ESP_VGA_PERFORMANCE_CHECK
    s_vgapalctrlcycles += cycle_count() - s1;
#endif

    I2S1.int_clr.val = I2S1.int_st.val;
}


} // end of namespace
