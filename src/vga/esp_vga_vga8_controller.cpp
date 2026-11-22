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
#include "esp_vga_vga8_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{

/*
To improve drawing and rendering speed pixels order is a bit oddy because we want to pack pixels (3 bits) into a uint32_t and ESP32 is little-endian.
8 pixels (24 bits) are packed in 3 bytes:
bytes:      0        1        2    ...
bits:   76543210 76543210 76543210 ...
pixels: 55666777 23334445 00011122 ...
bits24: 0                          1...
*/

static inline __attribute__((always_inline)) void vga8_set_pixel_in_row(uint8_t *row, int x,
                                                                        int value)
{
    uint32_t *bits24 = (uint32_t *)(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    *bits24 ^= ((value << shift) ^ *bits24) & (7 << shift);
}

static inline __attribute__((always_inline)) int vga8_get_pixel_in_row(uint8_t *row, int x)
{
    uint32_t *bits24 = (uint32_t *)(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    return (*bits24 >> shift) & 7;
}

#define VGA8_INVERTPIXELINROW(row, x) \
    *((uint32_t *)(row + ((x) >> 3) * 3)) ^= 7 << (21 - ((x) & 7) * 3)

static inline __attribute__((always_inline)) void vga8_set_pixel(int x, int y, int value)
{
    auto row = (uint8_t *)vga8_controller::sget_scanline(y);
    uint32_t *bits24 = (uint32_t *)(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    *bits24 ^= ((value << shift) ^ *bits24) & (7 << shift);
}

#define VGA8_GETPIXEL(x, y) \
    vga8_get_pixel_in_row((uint8_t *)vga8_controller::_isr_view_port[(y)], (x))

#define VGA8_INVERT_PIXEL(x, y) \
    VGA8_INVERTPIXELINROW((uint8_t *)vga8_controller::_isr_view_port[(y)], (x))


#define VGA8_COLUMNSQUANTUM 16


/*************************************************************************************/
/* VGA8Controller definitions */


vga8_controller *vga8_controller::_instance = nullptr;


vga8_controller::vga8_controller()
    : vga_paletted_controller(VGA8_LinesCount, VGA8_COLUMNSQUANTUM,
                              esp::gfx::native_pixel_format::palette8, 8, 3, isr_handler)
{
    _instance = this;
    _packed_palette_index_pair_to_signals =
        (uint16_t *)heap_caps_malloc(256 * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


vga8_controller::~vga8_controller()
{
    heap_caps_free((void *)_packed_palette_index_pair_to_signals);
}


void vga8_controller::setup_default_palette()
{
    set_palette_item(0, esp::gfx::rgb888(0, 0, 0)); // 0: black
    set_palette_item(1, esp::gfx::rgb888(128, 0, 0)); // 1: red
    set_palette_item(2, esp::gfx::rgb888(0, 128, 0)); // 2: green
    set_palette_item(3, esp::gfx::rgb888(0, 0, 128)); // 3: blue
    set_palette_item(4, esp::gfx::rgb888(255, 0, 0)); // 4: bright red
    set_palette_item(5, esp::gfx::rgb888(0, 255, 0)); // 5: bright green
    set_palette_item(6, esp::gfx::rgb888(0, 0, 255)); // 6: bright blue
    set_palette_item(7, esp::gfx::rgb888(255, 255, 255)); // 7: white
}


void vga8_controller::set_palette_item(int index, esp::gfx::rgb888 const &color)
{
    index %= 8;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 8; ++i)
    {
        _packed_palette_index_pair_to_signals[(index << 3) | i] &= 0xFF00;
        _packed_palette_index_pair_to_signals[(index << 3) | i] |= (_hv_sync | packed222);
        _packed_palette_index_pair_to_signals[(i << 3) | index] &= 0x00FF;
        _packed_palette_index_pair_to_signals[(i << 3) | index] |= (_hv_sync | packed222) << 8;
    }
}


void vga8_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                   esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga8_set_pixel);
}


// coordinates are absolute values (not relative to origin)
// line clipped on current absolute clipping rectangle
void vga8_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, uint8_t color_index) { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, vga8_set_pixel,
        [&](int x, int y) { VGA8_INVERT_PIXEL(x, y); });
}


// parameters not checked
void vga8_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, rgb888_to_palette_index(color));
}


// parameters not checked
void vga8_controller::raw_fill_row(int y, int x1, int x2, uint8_t color_index)
{
    uint8_t *row = (uint8_t *)_view_port[y];
    for (; x1 <= x2; ++x1)
        vga8_set_pixel_in_row(row, x1, color_index);
}


// parameters not checked
void vga8_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        VGA8_INVERTPIXELINROW(row, x);
}


void vga8_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = (uint8_t *)_view_port[src_y];
    auto dst_row = (uint8_t *)_view_port[dst_y];
    for (; x1 <= x2; ++x1)
        vga8_set_pixel_in_row(dst_row, x1, vga8_get_pixel_in_row(src_row, x1));
}


void vga8_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = (uint8_t *)_view_port[y_a];
    auto row_b = (uint8_t *)_view_port[y_b];
    for (; x1 <= x2; ++x1)
    {
        uint8_t a = vga8_get_pixel_in_row(row_a, x1);
        uint8_t b = vga8_get_pixel_in_row(row_b, x1);
        vga8_set_pixel_in_row(row_a, x1, b);
        vga8_set_pixel_in_row(row_b, x1, a);
    }
}


void vga8_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga8_set_pixel);
}


void vga8_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    uint32_t pattern8 = (palette_index) | (palette_index << 3) | (palette_index << 6)
        | (palette_index << 9) | (palette_index << 12) | (palette_index << 15)
        | (palette_index << 18) | (palette_index << 21);
    for (int y = 0; y < _view_port_height; ++y)
    {
        auto dest = (uint8_t *)_view_port[y];
        for (int x = 0; x < _view_port_width; x += 8, dest += 3)
            *((uint32_t *)dest) = (*((uint32_t *)dest) & 0xFF000000) | pattern8;
    }
}


// scroll < 0 -> scroll UP
// scroll > 0 -> scroll DOWN
void vga8_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swapRowsCopying
        [&](int y_a, int y_b) { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swapRowsPointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 color)
        { raw_fill_row(y, x1, x2, color); } // rawFillRow
    );
}


// todo: optimize!
void vga8_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t back = rgb888_to_palette_index(actual_brush_color());

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    int width = x2 - x1 + 1;
    bool h_scrollling_region_aligned = ((x1 & 7) == 0 && (width & 7) == 0); // 8 pixels aligned

    if (scroll < 0)
    {
        // scroll left
        for (int y = y1; y <= y2; ++y)
        {
            for (int s = -scroll; s > 0;)
            {
                if (h_scrollling_region_aligned && s >= 8)
                {
                    // scroll left by multiplies of 8
                    uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 8 * 3;
                    auto sc = s & ~7;
                    auto sz = width & ~7;
                    memmove(row, row + sc / 8 * 3, (sz - sc) / 8 * 3);
                    raw_fill_row(y, x2 - sc + 1, x2, back);
                    s -= sc;
                }
                else
                {
                    // unaligned horizontal scrolling region, fallback to slow version
                    auto row = (uint8_t *)_view_port[y];
                    for (int x = x1; x <= x2 - s; ++x)
                        vga8_set_pixel_in_row(row, x, vga8_get_pixel_in_row(row, x + s));
                    // fill right area with brush color
                    raw_fill_row(y, x2 - s + 1, x2, back);
                    s = 0;
                }
            }
        }
    }
    else if (scroll > 0)
    {
        // scroll right
        for (int y = y1; y <= y2; ++y)
        {
            for (int s = scroll; s > 0;)
            {
                if (h_scrollling_region_aligned && s >= 8)
                {
                    // scroll right by multiplies of 8
                    uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 8 * 3;
                    auto sc = s & ~7;
                    auto sz = width & ~7;
                    memmove(row + sc / 8 * 3, row, (sz - sc) / 8 * 3);
                    raw_fill_row(y, x1, x1 + sc - 1, back);
                    s -= sc;
                }
                else
                {
                    // unaligned horizontal scrolling region, fallback to slow version
                    auto row = (uint8_t *)_view_port[y];
                    for (int x = x2 - s; x >= x1; --x)
                        vga8_set_pixel_in_row(row, x + s, vga8_get_pixel_in_row(row, x));
                    // fill left area with brush color
                    raw_fill_row(y, x1, x1 + s - 1, back);
                    s = 0;
                }
            }
        }
    }
}


void vga8_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                 esp::gfx::glyph_options glyph_options, esp::gfx::rgb888 pen_color,
                                 esp::gfx::rgb888 brush_color, esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; }, vga8_set_pixel_in_row);
}


void vga8_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}


void vga8_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, [&](int y) { return (uint8_t *)_view_port[y]; },
        vga8_get_pixel_in_row, vga8_set_pixel_in_row);
}


// Slow operation!
// supports overlapping of source and dest rectangles
void vga8_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return (uint8_t *)_view_port[y]; }, vga8_get_pixel_in_row,
        vga8_set_pixel_in_row);
}


// no bounds check is done!
void vga8_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[vga8_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 x 3 = 255
        }
    }
}


void vga8_controller::raw_draw_bitmap_native(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                             int x1, int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, (uint8_t *)bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga8_set_pixel_in_row);
}


void vga8_controller::raw_draw_bitmap_mask(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                           void *save_background, int x1, int y1, int x_count,
                                           int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga8_get_pixel_in_row, [&](uint8_t *row, int x)
        { vga8_set_pixel_in_row(row, x, foreground_color_index); } // rawSetPixelInRow
    );
}


void vga8_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga8_get_pixel_in_row, [&](uint8_t *row, int x, uint8_t src)
        { vga8_set_pixel_in_row(row, x, rgb2222_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void vga8_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return vga8_get_pixel_in_row(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        { vga8_set_pixel_in_row(row, x, rgb8888_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void vga8_controller::direct_set_pixel(int x, int y, int value)
{
    vga8_set_pixel(x, y, value);
}


void IRAM_ATTR vga8_controller::isr_handler(void *arg)
{
#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
    auto s1 = cycle_count();
#endif

    auto ctrl = (vga8_controller *)arg;

    if (I2S1.int_st.out_eof)
    {

        auto const desc = (lldesc_t *)I2S1.out_eof_des_addr;

        if (desc == _frame_reset_desc)
            _scan_line = 0;

        auto const width = ctrl->_view_port_width;
        auto const height = ctrl->_view_port_height;
        auto const packedPaletteIndexPair_to_signals =
            (uint16_t const *)ctrl->_packed_palette_index_pair_to_signals;
        auto const lines = ctrl->_lines;

        int scan_line = (_scan_line + VGA8_LinesCount / 2) % height;

        auto line_index = scan_line & (VGA8_LinesCount - 1);

        for (int i = 0; i < VGA8_LinesCount / 2; ++i)
        {

            auto src = (uint8_t const *)_isr_view_port_visible[scan_line];
            auto dest = (uint16_t *)lines[line_index];

            // optimization warn: horizontal resolution must be a multiple of 16!
            for (int col = 0; col < width; col += 16)
            {

                auto w1 = *((uint16_t *)(src)); // hi A:23334445, lo A:55666777
                auto w2 = *((uint16_t *)(src + 2)); // hi B:55666777, lo A:00011122
                auto w3 = *((uint16_t *)(src + 4)); // hi B:00011122, lo B:23334445

                PSRAM_HACK;

                auto src1 = w1 | (w2 << 16);
                auto src2 = (w2 >> 8) | (w3 << 8);

                auto v1 = packedPaletteIndexPair_to_signals[(src1) & 0x3f]; // pixels 0, 1
                auto v2 = packedPaletteIndexPair_to_signals[(src1 >> 6) & 0x3f]; // pixels 2, 3
                auto v3 = packedPaletteIndexPair_to_signals[(src1 >> 12) & 0x3f]; // pixels 4, 5
                auto v4 = packedPaletteIndexPair_to_signals[(src1 >> 18) & 0x3f]; // pixels 6, 7
                auto v5 = packedPaletteIndexPair_to_signals[(src2) & 0x3f]; // pixels 8, 9
                auto v6 = packedPaletteIndexPair_to_signals[(src2 >> 6) & 0x3f]; // pixels 10, 11
                auto v7 = packedPaletteIndexPair_to_signals[(src2 >> 12) & 0x3f]; // pixels 12, 13
                auto v8 = packedPaletteIndexPair_to_signals[(src2 >> 18) & 0x3f]; // pixels 14, 15

                *(dest + 2) = v1;
                *(dest + 3) = v2;
                *(dest + 0) = v3;
                *(dest + 1) = v4;
                *(dest + 6) = v5;
                *(dest + 7) = v6;
                *(dest + 4) = v7;
                *(dest + 5) = v8;

                dest += 8;
                src += 6;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += VGA8_LinesCount / 2;

        if (scan_line >= height && !ctrl->_primitive_processing_suspended
            && spi_flash_cache_enabled() && ctrl->_primitive_exec_task)
        {
            // vertical sync, unlock primitive execution task
            // warn: don't use vTaskSuspendAll() in primitive drawing, otherwise vTaskNotifyGiveFromISR may be blocked and screen will flick!
            vTaskNotifyGiveFromISR(ctrl->_primitive_exec_task, NULL);
        }
    }

#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
    s_vgapalctrlcycles += cycle_count() - s1;
#endif

    I2S1.int_clr.val = I2S1.int_st.val;
}


} // end of namespace
