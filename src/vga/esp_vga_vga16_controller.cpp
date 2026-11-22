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
#include "esp_vga_vga16_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


// high nibble is pixel 0, low nibble is pixel 1

static inline __attribute__((always_inline)) void vga16_set_pixel_in_row(uint8_t *row, int x,
                                                                         int value)
{
    int brow = x >> 1;
    row[brow] = (x & 1) ? ((row[brow] & 0xf0) | value) : ((row[brow] & 0x0f) | (value << 4));
}

static inline __attribute__((always_inline)) int vga16_get_pixel_in_row(uint8_t *row, int x)
{
    int brow = x >> 1;
    return ((x & 1) ? (row[brow] & 0x0f) : ((row[brow] & 0xf0) >> 4));
}

#define VGA16_INVERTPIXELINROW(row, x) (row)[(x) >> 1] ^= (0xf0 >> (((x) & 1) << 2))

static inline __attribute__((always_inline)) void vga16_set_pixel(int x, int y, int value)
{
    auto row = (uint8_t *)vga16_controller::sget_scanline(y);
    int brow = x >> 1;
    row[brow] = (x & 1) ? ((row[brow] & 0xf0) | value) : ((row[brow] & 0x0f) | (value << 4));
}

#define VGA16_GETPIXEL(x, y) \
    vga16_get_pixel_in_row((uint8_t *)vga16_controller::_isr_view_port[(y)], (x))

#define VGA16_INVERT_PIXEL(x, y) \
    VGA16_INVERTPIXELINROW((uint8_t *)vga16_controller::_isr_view_port[(y)], (x))


#define VGA16_COLUMNSQUANTUM 16


/*************************************************************************************/
/* VGA16Controller definitions */


vga16_controller *vga16_controller::_instance = nullptr;


vga16_controller::vga16_controller()
    : vga_paletted_controller(VGA16_LinesCount, VGA16_COLUMNSQUANTUM,
                              esp::gfx::native_pixel_format::palette16, 2, 1, isr_handler)
{
    _instance = this;
}


void vga16_controller::setup_default_palette()
{
    for (int color_index = 0; color_index < 16; ++color_index)
    {
        esp::gfx::rgb888 rgb888((esp::gfx::color)color_index);
        set_palette_item(color_index, rgb888);
    }
}


void vga16_controller::set_palette_item(int index, esp::gfx::rgb888 const &color)
{
    index %= 16;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 16; ++i)
    {
        _packed_palette_index_pair_to_signals[(index << 4) | i] &= 0xFF00;
        _packed_palette_index_pair_to_signals[(index << 4) | i] |= (_hv_sync | packed222);
        _packed_palette_index_pair_to_signals[(i << 4) | index] &= 0x00FF;
        _packed_palette_index_pair_to_signals[(i << 4) | index] |= (_hv_sync | packed222) << 8;
    }
}


void vga16_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                    esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga16_set_pixel);
}


// coordinates are absolute values (not relative to origin)
// line clipped on current absolute clipping rectangle
void vga16_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, uint8_t color_index) { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, vga16_set_pixel,
        [&](int x, int y) { VGA16_INVERT_PIXEL(x, y); });
}


// parameters not checked
void vga16_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, rgb888_to_palette_index(color));
}


// parameters not checked
void vga16_controller::raw_fill_row(int y, int x1, int x2, uint8_t color_index)
{
    uint8_t *row = (uint8_t *)_view_port[y];
    // fill first pixels before full 16 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        vga16_set_pixel_in_row(row, x, color_index);
    }
    // fill whole 16 bits words (4 pixels)
    if (x <= x2)
    {
        int sz = (x2 & ~3) - x;
        memset((void *)(row + x / 2), color_index | (color_index << 4), sz / 2);
        x += sz;
    }
    // fill last unaligned pixels
    for (; x <= x2; ++x)
    {
        vga16_set_pixel_in_row(row, x, color_index);
    }
}


// parameters not checked
void vga16_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        VGA16_INVERTPIXELINROW(row, x);
}


void vga16_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = (uint8_t *)_view_port[src_y];
    auto dst_row = (uint8_t *)_view_port[dst_y];
    // copy first pixels before full 16 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        vga16_set_pixel_in_row(dst_row, x, vga16_get_pixel_in_row(src_row, x));
    }
    // copy whole 16 bits words (4 pixels)
    auto src = (uint16_t *)(src_row + x / 2);
    auto dst = (uint16_t *)(dst_row + x / 2);
    for (int right = (x2 & ~3); x < right; x += 4)
        *dst++ = *src++;
    // copy last unaligned pixels
    for (x = (x2 & ~3); x <= x2; ++x)
    {
        vga16_set_pixel_in_row(dst_row, x, vga16_get_pixel_in_row(src_row, x));
    }
}


void vga16_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = (uint8_t *)_view_port[y_a];
    auto row_b = (uint8_t *)_view_port[y_b];
    // swap first pixels before full 16 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        uint8_t a = vga16_get_pixel_in_row(row_a, x);
        uint8_t b = vga16_get_pixel_in_row(row_b, x);
        vga16_set_pixel_in_row(row_a, x, b);
        vga16_set_pixel_in_row(row_b, x, a);
    }
    // swap whole 16 bits words (4 pixels)
    auto a = (uint16_t *)(row_a + x / 2);
    auto b = (uint16_t *)(row_b + x / 2);
    for (int right = (x2 & ~3); x < right; x += 4)
        esp::tswap(*a++, *b++);
    // swap last unaligned pixels
    for (x = (x2 & ~3); x <= x2; ++x)
    {
        uint8_t a = vga16_get_pixel_in_row(row_a, x);
        uint8_t b = vga16_get_pixel_in_row(row_b, x);
        vga16_set_pixel_in_row(row_a, x, b);
        vga16_set_pixel_in_row(row_b, x, a);
    }
}


void vga16_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga16_set_pixel);
}


void vga16_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    uint8_t pattern = palette_index | (palette_index << 4);
    for (int y = 0; y < _view_port_height; ++y)
        memset((uint8_t *)_view_port[y], pattern, _view_port_width / 2);
}


// scroll < 0 -> scroll UP
// scroll > 0 -> scroll DOWN
void vga16_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swapRowsCopying
        [&](int y_a, int y_b) { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swapRowsPointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 color)
        { raw_fill_row(y, x1, x2, color); } // rawFillRow
    );
}


void vga16_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t back4 = rgb888_to_palette_index(actual_brush_color());

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    int width = x2 - x1 + 1;
    bool h_scrollling_region_aligned = ((x1 & 3) == 0 && (width & 3) == 0); // 4 pixels aligned

    if (scroll < 0)
    {
        // scroll left
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // aligned horizontal scrolling region, fast version
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 2;
                for (int s = -scroll; s > 0;)
                {
                    if (s > 1)
                    {
                        // scroll left by 2, 4, 6, ... moving bytes
                        auto sc = s & ~1;
                        auto sz = width & ~1;
                        memmove(row, row + sc / 2, (sz - sc) / 2);
                        raw_fill_row(y, x2 - sc + 1, x2, back4);
                        s -= sc;
                    }
                    else if (s & 1)
                    {
                        // scroll left 1 pixel (uint16_t at the time = 4 pixels)
                        // nibbles 0,1,2...  P is prev or background
                        // byte                     : 01 23 45 67 -> 12 34 56 7P
                        // word (little endian CPU) : 2301 6745  ->  3412 7P56
                        auto prev = back4;
                        auto w = (uint16_t *)(row + width / 2) - 1;
                        for (int i = 0; i < width; i += 4)
                        {
                            const uint16_t p4 = *w; // four pixels
                            *w-- = (p4 << 4 & 0xf000) | (prev << 8 & 0x0f00) | (p4 << 4 & 0x00f0)
                                | (p4 >> 12 & 0x000f);
                            prev = p4 >> 4 & 0x000f;
                        }
                        --s;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                auto row = (uint8_t *)_view_port[y];
                for (int x = x1; x <= x2 + scroll; ++x)
                    vga16_set_pixel_in_row(row, x, vga16_get_pixel_in_row(row, x - scroll));
                // fill right area with brush color
                raw_fill_row(y, x2 + 1 + scroll, x2, back4);
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
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 2;
                for (int s = scroll; s > 0;)
                {
                    if (s > 1)
                    {
                        // scroll right by 2, 4, 6, ... moving bytes
                        auto sc = s & ~1;
                        auto sz = width & ~1;
                        memmove(row + sc / 2, row, (sz - sc) / 2);
                        raw_fill_row(y, x1, x1 + sc - 1, back4);
                        s -= sc;
                    }
                    else if (s & 1)
                    {
                        // scroll right 1 pixel (uint16_t at the time = 4 pixels)
                        // nibbles 0,1,2...  P is prev or background
                        // byte                     : 01 23 45 67 -> P0 12 34 56 7...
                        // word (little endian CPU) : 2301 6745  ->  12P0 5634 ...
                        auto prev = back4;
                        auto w = (uint16_t *)row;
                        for (int i = 0; i < width; i += 4)
                        {
                            const uint16_t p4 = *w; // four pixels
                            *w++ = (p4 << 12 & 0xf000) | (p4 >> 4 & 0x0f00) | (prev << 4)
                                | (p4 >> 4 & 0x000f);
                            prev = p4 >> 8 & 0x000f;
                        }
                        --s;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                auto row = (uint8_t *)_view_port[y];
                for (int x = x2 - scroll; x >= x1; --x)
                    vga16_set_pixel_in_row(row, x + scroll, vga16_get_pixel_in_row(row, x));
                // fill left area with brush color
                raw_fill_row(y, x1, x1 + scroll - 1, back4);
            }
        }
    }
}


void vga16_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                  esp::gfx::glyph_options glyph_options, esp::gfx::rgb888 pen_color,
                                  esp::gfx::rgb888 brush_color, esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; }, vga16_set_pixel_in_row);
}


void vga16_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}


void vga16_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, [&](int y) { return (uint8_t *)_view_port[y]; },
        vga16_get_pixel_in_row, vga16_set_pixel_in_row);
}


// Slow operation!
// supports overlapping of source and dest rectangles
void vga16_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return (uint8_t *)_view_port[y]; },
        vga16_get_pixel_in_row, vga16_set_pixel_in_row);
}


// no bounds check is done!
void vga16_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[vga16_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 x 3 = 255
        }
    }
}


void vga16_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                              esp::gfx::bitmap const *bitmap, int x1, int y1,
                                              int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, (uint8_t *)bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga16_set_pixel_in_row);
}


void vga16_controller::raw_draw_bitmap_mask(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                            void *save_background, int x1, int y1, int x_count,
                                            int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga16_get_pixel_in_row, [&](uint8_t *row, int x)
        { vga16_set_pixel_in_row(row, x, foreground_color_index); } // rawSetPixelInRow
    );
}


void vga16_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                esp::gfx::bitmap const *bitmap,
                                                void *save_background, int x1, int y1, int x_count,
                                                int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga16_get_pixel_in_row, [&](uint8_t *row, int x, uint8_t src)
        { vga16_set_pixel_in_row(row, x, rgb2222_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void vga16_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                esp::gfx::bitmap const *bitmap,
                                                void *save_background, int x1, int y1, int x_count,
                                                int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return vga16_get_pixel_in_row(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        { vga16_set_pixel_in_row(row, x, rgb8888_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga16_controller::isr_handler(void *arg)
{
#if ESP_VGA_PERFORMANCE_CHECK
    auto s1 = cycle_count();
#endif

    auto ctrl = (vga16_controller *)arg;

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

        int scan_line = (_scan_line + VGA16_LinesCount / 2) % height;

        auto line_index = scan_line & (VGA16_LinesCount - 1);

        for (int i = 0; i < VGA16_LinesCount / 2; ++i)
        {

            auto src = (uint8_t const *)_isr_view_port_visible[scan_line];
            auto dest = (uint16_t *)lines[line_index];

            // optimization warn: horizontal resolution must be a multiple of 16!
            for (int col = 0; col < width; col += 16)
            {

                auto src1 = *(src + 0);
                auto src2 = *(src + 1);
                auto src3 = *(src + 2);
                auto src4 = *(src + 3);
                auto src5 = *(src + 4);
                auto src6 = *(src + 5);
                auto src7 = *(src + 6);
                auto src8 = *(src + 7);

                PSRAM_HACK;

                auto v1 = packedPaletteIndexPair_to_signals[src1];
                auto v2 = packedPaletteIndexPair_to_signals[src2];
                auto v3 = packedPaletteIndexPair_to_signals[src3];
                auto v4 = packedPaletteIndexPair_to_signals[src4];
                auto v5 = packedPaletteIndexPair_to_signals[src5];
                auto v6 = packedPaletteIndexPair_to_signals[src6];
                auto v7 = packedPaletteIndexPair_to_signals[src7];
                auto v8 = packedPaletteIndexPair_to_signals[src8];

                *(dest + 1) = v1;
                *(dest) = v2;
                *(dest + 3) = v3;
                *(dest + 2) = v4;
                *(dest + 5) = v5;
                *(dest + 4) = v6;
                *(dest + 7) = v7;
                *(dest + 6) = v8;

                dest += 8;
                src += 8;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += VGA16_LinesCount / 2;

        if (scan_line >= height && !ctrl->_primitive_processing_suspended
            && spi_flash_cache_enabled() && ctrl->_primitive_exec_task)
        {
            // vertical sync, unlock primitive execution task
            // warn: don't use vTaskSuspendAll() in primitive drawing, otherwise vTaskNotifyGiveFromISR may be blocked and screen will flick!
            vTaskNotifyGiveFromISR(ctrl->_primitive_exec_task, NULL);
        }
    }

#if ESP_VGA_PERFORMANCE_CHECK
    s_vgapalctrlcycles += cycle_count() - s1;
#endif

    I2S1.int_clr.val = I2S1.int_st.val;
}


} // end of namespace
