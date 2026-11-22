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
#include "esp_vga_vga4_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


static inline __attribute__((always_inline)) void vga4_set_pixel_in_row(uint8_t *row, int x,
                                                                        int value)
{
    int brow = x >> 2;
    int shift = 6 - (x & 3) * 2;
    row[brow] ^= ((value << shift) ^ row[brow]) & (3 << shift);
}

static inline __attribute__((always_inline)) int vga4_get_pixel_in_row(uint8_t *row, int x)
{
    int brow = x >> 2;
    int shift = 6 - (x & 3) * 2;
    return (row[brow] >> shift) & 3;
}

#define VGA4_INVERTPIXELINROW(row, x) (row)[(x) >> 2] ^= 3 << (6 - ((x) & 3) * 2)

static inline __attribute__((always_inline)) void vga4_set_pixel(int x, int y, int value)
{
    auto row = (uint8_t *)vga4_controller::sget_scanline(y);
    int brow = x >> 2;
    int shift = 6 - (x & 3) * 2;
    row[brow] ^= ((value << shift) ^ row[brow]) & (3 << shift);
}

#define VGA4_GETPIXEL(x, y) \
    vga4_get_pixel_in_row((uint8_t *)vga4_controller::_isr_view_port[(y)], (x))

#define VGA4_INVERT_PIXEL(x, y) \
    VGA4_INVERTPIXELINROW((uint8_t *)vga4_controller::_isr_view_port[(y)], (x))


#define VGA4_COLUMNSQUANTUM 16


/*************************************************************************************/
/* VGA4Controller definitions */


vga4_controller *vga4_controller::_instance = nullptr;


vga4_controller::vga4_controller()
    : vga_paletted_controller(VGA4_LinesCount, VGA4_COLUMNSQUANTUM,
                              esp::gfx::native_pixel_format::palette4, 4, 1, isr_handler)
{
    _instance = this;
    _packed_palette_index_quad_to_signals =
        (uint32_t *)heap_caps_malloc(256 * sizeof(uint32_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


vga4_controller::~vga4_controller()
{
    heap_caps_free((void *)_packed_palette_index_quad_to_signals);
}


void vga4_controller::setup_default_palette()
{
    set_palette_item(0, esp::gfx::rgb888(0, 0, 0)); // 0: black
    set_palette_item(1, esp::gfx::rgb888(0, 0, 255)); // 1: blue
    set_palette_item(2, esp::gfx::rgb888(0, 255, 0)); // 2: green
    set_palette_item(3, esp::gfx::rgb888(255, 255, 255)); // 3: white
}


void vga4_controller::set_palette_item(int index, esp::gfx::rgb888 const &color)
{
    index %= 4;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 256; ++i)
    {
        auto b = (uint8_t *)(_packed_palette_index_quad_to_signals + i);
        for (int j = 0; j < 4; ++j)
        {
            auto aj = 6 - j * 2;
            if (((i >> aj) & 3) == index)
            {
                b[j ^ 2] = _hv_sync | packed222;
            }
        }
    }
}


void vga4_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                   esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga4_set_pixel);
}


// coordinates are absolute values (not relative to origin)
// line clipped on current absolute clipping rectangle
void vga4_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, uint8_t color_index) { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, vga4_set_pixel,
        [&](int x, int y) { VGA4_INVERT_PIXEL(x, y); });
}


// parameters not checked
void vga4_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, rgb888_to_palette_index(color));
}


// parameters not checked
void vga4_controller::raw_fill_row(int y, int x1, int x2, uint8_t color_index)
{
    uint8_t *row = (uint8_t *)_view_port[y];
    // fill first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        vga4_set_pixel_in_row(row, x, color_index);
    }
    // fill whole 8 bits words (4 pixels)
    if (x <= x2)
    {
        int sz = (x2 & ~3) - x;
        memset((void *)(row + x / 4),
               color_index | (color_index << 2) | (color_index << 4) | (color_index << 6), sz / 4);
        x += sz;
    }
    // fill last unaligned pixels
    for (; x <= x2; ++x)
    {
        vga4_set_pixel_in_row(row, x, color_index);
    }
}


// parameters not checked
void vga4_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        VGA4_INVERTPIXELINROW(row, x);
}


void vga4_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = (uint8_t *)_view_port[src_y];
    auto dst_row = (uint8_t *)_view_port[dst_y];
    // copy first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        vga4_set_pixel_in_row(dst_row, x, vga4_get_pixel_in_row(src_row, x));
    }
    // copy whole 8 bits words (4 pixels)
    auto src = (uint8_t *)(src_row + x / 4);
    auto dst = (uint8_t *)(dst_row + x / 4);
    for (int right = (x2 & ~3); x < right; x += 4)
        *dst++ = *src++;
    // copy last unaligned pixels
    for (x = (x2 & ~3); x <= x2; ++x)
    {
        vga4_set_pixel_in_row(dst_row, x, vga4_get_pixel_in_row(src_row, x));
    }
}


void vga4_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = (uint8_t *)_view_port[y_a];
    auto row_b = (uint8_t *)_view_port[y_b];
    // swap first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 3) != 0; ++x)
    {
        uint8_t a = vga4_get_pixel_in_row(row_a, x);
        uint8_t b = vga4_get_pixel_in_row(row_b, x);
        vga4_set_pixel_in_row(row_a, x, b);
        vga4_set_pixel_in_row(row_b, x, a);
    }
    // swap whole 8 bits words (4 pixels)
    auto a = (uint8_t *)(row_a + x / 4);
    auto b = (uint8_t *)(row_b + x / 4);
    for (int right = (x2 & ~3); x < right; x += 4)
        esp::tswap(*a++, *b++);
    // swap last unaligned pixels
    for (x = (x2 & ~3); x <= x2; ++x)
    {
        uint8_t a = vga4_get_pixel_in_row(row_a, x);
        uint8_t b = vga4_get_pixel_in_row(row_b, x);
        vga4_set_pixel_in_row(row_a, x, b);
        vga4_set_pixel_in_row(row_b, x, a);
    }
}


void vga4_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga4_set_pixel);
}


void vga4_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    uint8_t pattern4 =
        palette_index | (palette_index << 2) | (palette_index << 4) | (palette_index << 6);
    for (int y = 0; y < _view_port_height; ++y)
        memset((uint8_t *)_view_port[y], pattern4, _view_port_width / 4);
}


// scroll < 0 -> scroll UP
// scroll > 0 -> scroll DOWN
void vga4_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swapRowsCopying
        [&](int y_a, int y_b) { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swapRowsPointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 color)
        { raw_fill_row(y, x1, x2, color); } // rawFillRow
    );
}


void vga4_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t back = rgb888_to_palette_index(actual_brush_color());
    uint8_t back4 = back | (back << 2) | (back << 4) | (back << 6);

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
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 4;
                for (int s = -scroll; s > 0;)
                {
                    if (s < 4)
                    {
                        // scroll left by 1..3
                        int sz = width / 4;
                        uint8_t prev = back4;
                        for (int i = sz - 1; i >= 0; --i)
                        {
                            uint8_t lowbits = prev >> (8 - s * 2);
                            prev = row[i];
                            row[i] = (row[i] << (s * 2)) | lowbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // scroll left by multiplies of 4
                        auto sc = s & ~3;
                        auto sz = width & ~3;
                        memmove(row, row + sc / 4, (sz - sc) / 4);
                        raw_fill_row(y, x2 - sc + 1, x2, back);
                        s -= sc;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                auto row = (uint8_t *)_view_port[y];
                for (int x = x1; x <= x2 + scroll; ++x)
                    vga4_set_pixel_in_row(row, x, vga4_get_pixel_in_row(row, x - scroll));
                // fill right area with brush color
                raw_fill_row(y, x2 + 1 + scroll, x2, back);
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
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 4;
                for (int s = scroll; s > 0;)
                {
                    if (s < 4)
                    {
                        // scroll right by 1..3
                        int sz = width / 4;
                        uint8_t prev = back4;
                        for (int i = 0; i < sz; ++i)
                        {
                            uint8_t highbits = prev << (8 - s * 2);
                            prev = row[i];
                            row[i] = (row[i] >> (s * 2)) | highbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // scroll right by multiplies of 4
                        auto sc = s & ~3;
                        auto sz = width & ~3;
                        memmove(row + sc / 4, row, (sz - sc) / 4);
                        raw_fill_row(y, x1, x1 + sc - 1, back);
                        s -= sc;
                    }
                }
            }
            else
            {
                // unaligned horizontal scrolling region, fallback to slow version
                auto row = (uint8_t *)_view_port[y];
                for (int x = x2 - scroll; x >= x1; --x)
                    vga4_set_pixel_in_row(row, x + scroll, vga4_get_pixel_in_row(row, x));
                // fill left area with brush color
                raw_fill_row(y, x1, x1 + scroll - 1, back);
            }
        }
    }
}


void vga4_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                 esp::gfx::glyph_options glyph_options, esp::gfx::rgb888 pen_color,
                                 esp::gfx::rgb888 brush_color, esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; }, vga4_set_pixel_in_row);
}


void vga4_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}


void vga4_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, [&](int y) { return (uint8_t *)_view_port[y]; },
        vga4_get_pixel_in_row, vga4_set_pixel_in_row);
}


// Slow operation!
// supports overlapping of source and dest rectangles
void vga4_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return (uint8_t *)_view_port[y]; }, vga4_get_pixel_in_row,
        vga4_set_pixel_in_row);
}


// no bounds check is done!
void vga4_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[vga4_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 x 3 = 255
        }
    }
}


void vga4_controller::raw_draw_bitmap_native(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                             int x1, int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, (uint8_t *)bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga4_set_pixel_in_row);
}


void vga4_controller::raw_draw_bitmap_mask(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                           void *save_background, int x1, int y1, int x_count,
                                           int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga4_get_pixel_in_row, [&](uint8_t *row, int x)
        { vga4_set_pixel_in_row(row, x, foreground_color_index); } // rawSetPixelInRow
    );
}


void vga4_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga4_get_pixel_in_row, [&](uint8_t *row, int x, uint8_t src)
        { vga4_set_pixel_in_row(row, x, rgb2222_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void vga4_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return vga4_get_pixel_in_row(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        { vga4_set_pixel_in_row(row, x, rgb8888_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga4_controller::isr_handler(void *arg)
{
#if ESP_VGA_PERFORMANCE_CHECK
    auto s1 = cycle_count();
#endif

    auto ctrl = (vga4_controller *)arg;

    if (I2S1.int_st.out_eof)
    {

        auto const desc = (lldesc_t *)I2S1.out_eof_des_addr;

        if (desc == _frame_reset_desc)
            _scan_line = 0;

        auto const width = ctrl->_view_port_width;
        auto const height = ctrl->_view_port_height;
        auto const packedPaletteIndexQuad_to_signals =
            (uint32_t const *)ctrl->_packed_palette_index_quad_to_signals;
        auto const lines = ctrl->_lines;

        int scan_line = (_scan_line + VGA4_LinesCount / 2) % height;

        auto line_index = scan_line & (VGA4_LinesCount - 1);

        for (int i = 0; i < VGA4_LinesCount / 2; ++i)
        {

            auto src = (uint8_t const *)_isr_view_port_visible[scan_line];
            auto dest = (uint32_t *)lines[line_index];

            // optimization warn: horizontal resolution must be a multiple of 16!
            for (int col = 0; col < width; col += 16)
            {

                auto src1 = *(src + 0);
                auto src2 = *(src + 1);
                auto src3 = *(src + 2);
                auto src4 = *(src + 3);

                PSRAM_HACK;

                auto v1 = packedPaletteIndexQuad_to_signals[src1];
                auto v2 = packedPaletteIndexQuad_to_signals[src2];
                auto v3 = packedPaletteIndexQuad_to_signals[src3];
                auto v4 = packedPaletteIndexQuad_to_signals[src4];

                *(dest + 0) = v1;
                *(dest + 1) = v2;
                *(dest + 2) = v3;
                *(dest + 3) = v4;

                dest += 4;
                src += 4;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += VGA4_LinesCount / 2;

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
