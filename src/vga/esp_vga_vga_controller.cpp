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


#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "soc/i2s_struct.h"

#include "esp_gfx_utils.h"
#include "esp_tools.h"
#include "esp_vga_vga_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


/*************************************************************************************/
/* VGAController definitions */


vga_controller *vga_controller::_instance = nullptr;


vga_controller::vga_controller()
{
    _instance = this;
}


void vga_controller::init()
{
    vga_base_controller::init();

    _double_buffer_over_dma = true;
}


void vga_controller::suspend_background_primitive_execution()
{
    vga_base_controller::suspend_background_primitive_execution();
    if (_primitive_processing_suspended == 1)
    {
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 0;
    }
}


void vga_controller::resume_background_primitive_execution()
{
    vga_base_controller::resume_background_primitive_execution();
    if (_primitive_processing_suspended == 0)
    {
        if (_isr_handle == nullptr)
            esp_intr_alloc(ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, v_sync_interrupt, this,
                           &_isr_handle);
        I2S1.int_clr.val = 0xFFFFFFFF;
        I2S1.int_ena.out_eof = 1;
    }
}


void vga_controller::allocate_view_port()
{
    vga_base_controller::allocate_view_port(MALLOC_CAP_DMA, _view_port_width);
}


void vga_controller::set_resolution(vga_timings const &timings, int view_port_width,
                                    int view_port_height, bool double_buffered)
{
    vga_base_controller::set_resolution(timings, view_port_width, view_port_height,
                                        double_buffered);

    // fill view port
    for (int i = 0; i < _view_port_height; ++i)
        fill(_view_port[i], 0, _view_port_width, 0, 0, 0, false, false);

    // number of microseconds usable in VSynch ISR
    _max_v_sync_isr_time = ceil(1000000.0 / _timings.frequency * _timings.scan_count * _h_line_size
                                * (_timings.v_sync_pulse + _timings.v_back_porch
                                   + _timings.v_front_porch + _view_port_row));

    start_gpio_stream();
    resume_background_primitive_execution();
}


void vga_controller::on_setup_dma_buffer(lldesc_t volatile *buffer,
                                         bool is_start_of_vert_front_porch, int scan,
                                         bool is_visible, int visible_row)
{
    // generate interrupt at the beginning of vertical front porch
    if (is_start_of_vert_front_porch)
        buffer->eof = 1;
}


void IRAM_ATTR vga_controller::v_sync_interrupt(void *arg)
{
    if (I2S1.int_st.out_eof)
    {
        auto vga_ctrl = (vga_controller *)arg;
        int64_t start_time =
            vga_ctrl->background_primitive_timeout_enabled() ? esp_timer_get_time() : 0;
        esp::gfx::rect update_rect = esp::gfx::rect(SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN);
        do
        {
            esp::gfx::primitive prim;
            if (vga_ctrl->get_primitive_isr(&prim) == false)
                break;

            vga_ctrl->exec_primitive(prim, update_rect, true);

            if (vga_ctrl->_primitive_processing_suspended)
                break;

        } while (!vga_ctrl->background_primitive_timeout_enabled()
                 || (start_time + vga_ctrl->_max_v_sync_isr_time > esp_timer_get_time()));
        vga_ctrl->show_sprites(update_rect);
    }
    I2S1.int_clr.val = I2S1.int_st.val;
}


void IRAM_ATTR vga_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                            esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect,
        [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int x, int y, uint8_t pattern) { VGA_PIXEL(x, y) = pattern; });
}


// coordinates are absolute values (not relative to origin)
// line clipped on current absolute clipping rectangle
void IRAM_ATTR vga_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y, int x1, int x2, uint8_t pattern) { raw_fill_row(y, x1, x2, pattern); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); },
        [&](int x, int y, uint8_t pattern) { VGA_PIXEL(x, y) = pattern; },
        [&](int x, int y) { VGA_INVERT_PIXEL(x, y); });
}


// parameters not checked
void IRAM_ATTR vga_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, prepare_pixel(color));
}


// parameters not checked
void IRAM_ATTR vga_controller::raw_fill_row(int y, int x1, int x2, uint8_t pattern)
{
    auto row = _view_port[y];
    // fill first bytes before full 32 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        VGA_PIXELINROW(row, x) = pattern;
    }
    // fill whole 32 bits words (don't care about VGA_PIXELINROW adjusted alignment)
    if (x <= x2)
    {
        int sz = (x2 & ~3) - x;
        memset((void *)(row + x), pattern, sz);
        x += sz;
    }
    // fill last unaligned bytes
    for (; x <= x2; ++x)
    {
        VGA_PIXELINROW(row, x) = pattern;
    }
}


// parameters not checked
void IRAM_ATTR vga_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
    {
        uint8_t *px = (uint8_t *)&VGA_PIXELINROW(row, x);
        *px = _hv_sync | ~(*px);
    }
}


// swaps all pixels inside the range x1...x2 of yA and yB
// parameters not checked
void IRAM_ATTR vga_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    uint8_t *row_a = (uint8_t *)_view_port[y_a];
    uint8_t *row_b = (uint8_t *)_view_port[y_b];
    // swap first bytes before full 32 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
        esp::tswap(VGA_PIXELINROW(row_a, x), VGA_PIXELINROW(row_b, x));
    // swap whole 32 bits words (don't care about VGA_PIXELINROW adjusted alignment)
    uint32_t *a = (uint32_t *)(row_a + x);
    uint32_t *b = (uint32_t *)(row_b + x);
    for (int right = (x2 & ~3); x < right; x += 4)
        esp::tswap(*a++, *b++);
    // swap last unaligned bytes
    for (x = (x2 & ~3); x <= x2; ++x)
        esp::tswap(VGA_PIXELINROW(row_a, x), VGA_PIXELINROW(row_b, x));
}


void IRAM_ATTR vga_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int x, int y, uint8_t pattern) { VGA_PIXEL(x, y) = pattern; });
}


void IRAM_ATTR vga_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t pattern = prepare_pixel(actual_brush_color());
    for (int y = 0; y < _view_port_height; ++y)
        memset((uint8_t *)_view_port[y], pattern, _view_port_width);
}


// scroll < 0 -> scroll UP
// scroll > 0 -> scroll DOWN
// Speciying horizontal scrolling region slow-down scrolling!
void IRAM_ATTR vga_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swapRowsCopying
        [&](int y_a, int y_b) { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swapRowsPointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 pattern)
        { raw_fill_row(y, x1, x2, pattern); } // rawFillRow
    );

    if (scroll != 0)
    {
        // reassign DMA pointers
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


// Scrolling by 1, 2, 3 and 4 pixels is optimized. Also scrolling multiples of 4 (8, 16, 24...) is optimized.
// Scrolling by other values requires up to three steps (scopose scrolling by 1, 2, 3 or 4): for example scrolling by 5 is scomposed to 4 and 1, scrolling
// by 6 is 4 + 2, etc.
// Horizontal scrolling region start and size (X2-X1+1) must be aligned to 32 bits, otherwise the unoptimized (very slow) version is used.
void IRAM_ATTR vga_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t pattern8 = prepare_pixel(actual_brush_color());
    uint16_t pattern16 = pattern8 << 8 | pattern8;
    uint32_t pattern32 = pattern16 << 16 | pattern16;

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    int width = x2 - x1 + 1;
    int width32 = width >> 2;
    bool h_scrollling_region_aligned = ((x1 & 3) == 0 && (width & 3) == 0);

    if (scroll < 0)
    {
        // scroll left
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // aligned horizontal scrolling region, fast version
                uint8_t *row = (uint8_t *)(_view_port[y] + x1);
                for (int s = -scroll; s > 0;)
                {
                    if (s >= 4)
                    {
                        // scroll left 4, 8 12, etc.. pixels moving 32 bit words
                        uint8_t *w = row;
                        int sz = (s & ~3) >> 2;
                        for (int i = 0; i < width32 - sz; ++i, w += 4)
                            *((uint32_t *)w) = *((uint32_t *)w + sz);
                        for (int i = esp::tmax(0, width32 - sz); i < width32; ++i, w += 4)
                            *((uint32_t *)w) = pattern32;
                        s -= (s & ~3);
                    }
                    else if ((s & 3) == 3)
                    {
                        // scroll left 3 pixels swapping 8 bit words
                        uint8_t *b = row;
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
                        // scroll left 2 pixels swapping 16 bit words
                        uint16_t *w = (uint16_t *)row;
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
                        // scroll left 1 pixel by rotating 32 bit words
                        uint8_t *w = row;
                        for (int i = 1; i < width32; ++i, w += 4)
                        {
                            *((uint32_t *)w) = *((uint32_t *)w) >> 8 | *((uint32_t *)w) << 24;
                            w[1] = w[6];
                        }
                        *((uint32_t *)w) = *((uint32_t *)w) >> 8 | *((uint32_t *)w) << 24;
                        w[1] = pattern8;
                        --s;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                uint8_t *row = (uint8_t *)_view_port[y];
                for (int x = x1; x <= x2 + scroll; ++x)
                    VGA_PIXELINROW(row, x) = VGA_PIXELINROW(row, x - scroll);
                // fill right area with brush color
                for (int x = x2 + 1 + scroll; x <= x2; ++x)
                    VGA_PIXELINROW(row, x) = pattern8;
            }
        }
    }
    else if (scroll > 0)
    {
        // scroll right
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // aligned horizontal scrolling region, fast version
                uint8_t *row = (uint8_t *)(_view_port[y] + x1);
                for (int s = scroll; s > 0;)
                {
                    if (s >= 4)
                    {
                        // scroll right 4, 8 12, etc.. pixels moving 32 bit words
                        int sz = (s & ~3) >> 2;
                        uint8_t *w = row + width - 4;
                        for (int i = 0; i < width32 - sz; ++i, w -= 4)
                            *((uint32_t *)w) = *((uint32_t *)w - sz);
                        for (int i = esp::tmax(0, width32 - sz); i < width32; ++i, w -= 4)
                            *((uint32_t *)w) = pattern32;
                        s -= (s & ~3);
                    }
                    else if ((s & 3) == 3)
                    {
                        // scroll right 3 pixels swapping 8 bit words
                        uint8_t *b = row + width - 4;
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
                        // scroll right 2 pixels swapping 16 bit words
                        uint16_t *w = (uint16_t *)(row + width - 4);
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
                        // scroll right 1 pixel by rotating 32 bit words
                        uint8_t *w = row + width - 4;
                        for (int i = 1; i < width32; ++i, w -= 4)
                        {
                            *((uint32_t *)w) = *((uint32_t *)w) << 8 | *((uint32_t *)w) >> 24;
                            w[2] = w[-3];
                        }
                        *((uint32_t *)w) = *((uint32_t *)w) << 8 | *((uint32_t *)w) >> 24;
                        w[2] = pattern8;
                        --s;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                uint8_t *row = (uint8_t *)_view_port[y];
                for (int x = x2 - scroll; x >= x1; --x)
                    VGA_PIXELINROW(row, x + scroll) = VGA_PIXELINROW(row, x);
                // fill left area with brush color
                for (int x = x1; x < x1 + scroll; ++x)
                    VGA_PIXELINROW(row, x) = pattern8;
            }
        }
    }
}


void IRAM_ATTR vga_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                          esp::gfx::glyph_options glyph_options,
                                          esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                                          esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; },
        [&](uint8_t *row, int x, uint8_t pattern) { VGA_PIXELINROW(row, x) = pattern; });
}


void IRAM_ATTR vga_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}


void IRAM_ATTR vga_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color) { return prepare_pixel(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; },
        [&](uint8_t *row, int x) { return VGA_PIXELINROW(row, x); },
        [&](uint8_t *row, int x, uint8_t pattern) { VGA_PIXELINROW(row, x) = pattern; });
}


// Slow operation!
// supports overlapping of source and dest rectangles
void IRAM_ATTR vga_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return (uint8_t *)_view_port[y]; },
        [&](uint8_t *row, int x) { return VGA_PIXELINROW(row, x); },
        [&](uint8_t *row, int x, uint8_t pattern) { VGA_PIXELINROW(row, x) = pattern; });
}


// no bounds check is done!
void vga_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        uint8_t *row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            uint8_t rawpix = VGA_PIXELINROW(row, x);
            *dest_buf = esp::gfx::rgb888((rawpix & 3) * 85, ((rawpix >> 2) & 3) * 85,
                                         ((rawpix >> 4) & 3) * 85);
        }
    }
}


// no bounds check is done!
void vga_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb222 *dest_buf)
{
    uint8_t *dbuf = (uint8_t *)dest_buf;
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        uint8_t *row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dbuf)
            *dbuf = VGA_PIXELINROW(row, x) & ~VGA_SYNC_MASK;
    }
}


// no bounds check is done!
void vga_controller::write_screen(esp::gfx::rect const &rect, esp::gfx::rgb222 *src_buf)
{
    uint8_t *sbuf = (uint8_t *)src_buf;
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        uint8_t *row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++sbuf)
            VGA_PIXELINROW(row, x) = *sbuf | _hv_sync;
    }
}


void IRAM_ATTR vga_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                      esp::gfx::bitmap const *bitmap, int x1,
                                                      int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, (uint8_t *)bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x, uint8_t src)
        { VGA_PIXELINROW(row, x) = _hv_sync | src; } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                    esp::gfx::bitmap const *bitmap,
                                                    void *save_background, int x1, int y1,
                                                    int x_count, int y_count)
{
    auto foreground_pattern = prepare_pixel(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return VGA_PIXELINROW(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x)
        { VGA_PIXELINROW(row, x) = foreground_pattern; } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                        esp::gfx::bitmap const *bitmap,
                                                        void *save_background, int x1, int y1,
                                                        int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return VGA_PIXELINROW(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, uint8_t src)
        { VGA_PIXELINROW(row, x) = _hv_sync | (src & 0x3f); } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                        esp::gfx::bitmap const *bitmap,
                                                        void *save_background, int x1, int y1,
                                                        int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return VGA_PIXELINROW(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        {
            VGA_PIXELINROW(row, x) =
                _hv_sync | (src.r >> 6) | (src.g >> 6 << 2) | (src.b >> 6 << 4);
        } // rawSetPixelInRow
    );
}


#ifdef FABGL_EMULATED
void vga_controller::updateFrameBuffer(uint32_t *frameBuffer)
{
    I2S1.int_st.out_eof = 1;
    v_sync_interrupt(this);

    auto dst = frameBuffer;
    for (int row = 0; row < _view_port_height; ++row)
    {
        auto src = _view_port_visible[row];
        for (int col = 0; col < _view_port_width; ++col, ++dst)
        {
            auto srcpx = src[col ^ 2];
            *dst = ((((srcpx >> 0) & 3) * 85) << 16) | ((((srcpx >> 2) & 3) * 85) << 8)
                | ((((srcpx >> 4) & 3) * 85) << 0);
        }
    }
}
#endif


} // end of namespace
