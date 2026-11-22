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
#include "esp_vga_vga2_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


static inline __attribute__((always_inline)) void vga2_set_pixel_in_row(uint8_t *row, int x,
                                                                        int value)
{
    int brow = x >> 3;
    row[brow] ^= (-value ^ row[brow]) & (0x80 >> (x & 7));
}

static inline __attribute__((always_inline)) int vga2_get_pixel_in_row(uint8_t *row, int x)
{
    int brow = x >> 3;
    return (row[brow] & (0x80 >> (x & 7))) != 0;
}

#define VGA2_INVERTPIXELINROW(row, x) (row)[(x) >> 3] ^= (0x80 >> ((x) & 7))

static inline __attribute__((always_inline)) void vga2_set_pixel(int x, int y, int value)
{
    auto row = (uint8_t *)vga2_controller::sget_scanline(y);
    int brow = x >> 3;
    row[brow] ^= (-value ^ row[brow]) & (0x80 >> (x & 7));
}

#define VGA2_GETPIXEL(x, y) \
    vga2_get_pixel_in_row((uint8_t *)vga2_controller::_isr_view_port[(y)], (x))

#define VGA2_INVERT_PIXEL(x, y) \
    VGA2_INVERTPIXELINROW((uint8_t *)vga2_controller::_isr_view_port[(y)], (x))


#define VGA2_COLUMNSQUANTUM 16


/*************************************************************************************/
/* VGA2Controller definitions */


vga2_controller *vga2_controller::_instance = nullptr;


vga2_controller::vga2_controller()
    : vga_paletted_controller(VGA2_LinesCount, VGA2_COLUMNSQUANTUM,
                              esp::gfx::native_pixel_format::palette2, 8, 1, isr_handler)
{
    _instance = this;
    _packed_palette_index_octet_to_signals =
        (uint64_t *)heap_caps_malloc(256 * sizeof(uint64_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


vga2_controller::~vga2_controller()
{
    heap_caps_free((void *)_packed_palette_index_octet_to_signals);
}


void vga2_controller::setup_default_palette()
{
    set_palette_item(0, esp::gfx::rgb888(0, 0, 0)); // 0: black
    set_palette_item(1, esp::gfx::rgb888(255, 255, 255)); // 1: white
}


void vga2_controller::set_palette_item(int index, esp::gfx::rgb888 const &color)
{
    index %= 2;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 256; ++i)
    {
        auto b = (uint8_t *)(_packed_palette_index_octet_to_signals + i);
        for (int j = 0; j < 8; ++j)
        {
            auto aj = 7 - j;
            if ((index == 0 && ((1 << aj) & i) == 0) || (index == 1 && ((1 << aj) & i) != 0))
            {
                b[j ^ 2] = _hv_sync | packed222;
            }
        }
    }
}


void vga2_controller::set_pixel_at(esp::gfx::pixel_desc const &pixel_desc,
                                   esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga2_set_pixel);
}


// coordinates are absolute values (not relative to origin)
// line clipped on current absolute clipping rectangle
void vga2_controller::abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, uint8_t color_index) { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, vga2_set_pixel,
        [&](int x, int y) { VGA2_INVERT_PIXEL(x, y); });
}


// parameters not checked
void vga2_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, rgb888_to_palette_index(color));
}


// parameters not checked
void vga2_controller::raw_fill_row(int y, int x1, int x2, uint8_t color_index)
{
    uint8_t *row = (uint8_t *)_view_port[y];
    // fill first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        vga2_set_pixel_in_row(row, x, color_index);
    }
    // fill whole 8 bits words (8 pixels)
    if (x <= x2)
    {
        int sz = (x2 & ~7) - x;
        memset((void *)(row + x / 8), color_index ? 0xFF : 0x00, sz / 8);
        x += sz;
    }
    // fill last unaligned pixels
    for (; x <= x2; ++x)
    {
        vga2_set_pixel_in_row(row, x, color_index);
    }
}


// parameters not checked
void vga2_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        VGA2_INVERTPIXELINROW(row, x);
}


void vga2_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = (uint8_t *)_view_port[src_y];
    auto dst_row = (uint8_t *)_view_port[dst_y];
    // copy first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        vga2_set_pixel_in_row(dst_row, x, vga2_get_pixel_in_row(src_row, x));
    }
    // copy whole 8 bits words (8 pixels)
    auto src = (uint8_t *)(src_row + x / 8);
    auto dst = (uint8_t *)(dst_row + x / 8);
    for (int right = (x2 & ~7); x < right; x += 8)
        *dst++ = *src++;
    // copy last unaligned pixels
    for (x = (x2 & ~7); x <= x2; ++x)
    {
        vga2_set_pixel_in_row(dst_row, x, vga2_get_pixel_in_row(src_row, x));
    }
}


void vga2_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = (uint8_t *)_view_port[y_a];
    auto row_b = (uint8_t *)_view_port[y_b];
    // swap first pixels before full 8 bits word
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        uint8_t a = vga2_get_pixel_in_row(row_a, x);
        uint8_t b = vga2_get_pixel_in_row(row_b, x);
        vga2_set_pixel_in_row(row_a, x, b);
        vga2_set_pixel_in_row(row_b, x, a);
    }
    // swap whole 8 bits words (8 pixels)
    auto a = (uint8_t *)(row_a + x / 8);
    auto b = (uint8_t *)(row_b + x / 8);
    for (int right = (x2 & ~7); x < right; x += 8)
        esp::tswap(*a++, *b++);
    // swap last unaligned pixels
    for (x = (x2 & ~7); x <= x2; ++x)
    {
        uint8_t a = vga2_get_pixel_in_row(row_a, x);
        uint8_t b = vga2_get_pixel_in_row(row_b, x);
        vga2_set_pixel_in_row(row_a, x, b);
        vga2_set_pixel_in_row(row_b, x, a);
    }
}


void vga2_controller::draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, vga2_set_pixel);
}


void vga2_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    uint8_t pattern8 = palette_index ? 0xFF : 0x00;
    for (int y = 0; y < _view_port_height; ++y)
        memset((uint8_t *)_view_port[y], pattern8, _view_port_width / 8);
}


// scroll < 0 -> scroll UP
// scroll > 0 -> scroll DOWN
void vga2_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swapRowsCopying
        [&](int y_a, int y_b) { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swapRowsPointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 color)
        { raw_fill_row(y, x1, x2, color); } // rawFillRow
    );
}


void vga2_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    uint8_t back = rgb888_to_palette_index(actual_brush_color());
    uint8_t back8 = back ? 0xFF : 0x00;

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
            if (h_scrollling_region_aligned)
            {
                // aligned horizontal scrolling region, fast version
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 8;
                for (int s = -scroll; s > 0;)
                {
                    if (s < 8)
                    {
                        // scroll left by 1..7
                        int sz = width / 8;
                        uint8_t prev = back8;
                        for (int i = sz - 1; i >= 0; --i)
                        {
                            uint8_t lowbits = prev >> (8 - s);
                            prev = row[i];
                            row[i] = (row[i] << s) | lowbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // scroll left by multiplies of 8
                        auto sc = s & ~7;
                        auto sz = width & ~7;
                        memmove(row, row + sc / 8, (sz - sc) / 8);
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
                    vga2_set_pixel_in_row(row, x, vga2_get_pixel_in_row(row, x - scroll));
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
                uint8_t *row = (uint8_t *)(_view_port[y]) + x1 / 8;
                for (int s = scroll; s > 0;)
                {
                    if (s < 8)
                    {
                        // scroll right by 1..7
                        int sz = width / 8;
                        uint8_t prev = back8;
                        for (int i = 0; i < sz; ++i)
                        {
                            uint8_t highbits = prev << (8 - s);
                            prev = row[i];
                            row[i] = (row[i] >> s) | highbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // scroll right by multiplies of 8
                        auto sc = s & ~7;
                        auto sz = width & ~7;
                        memmove(row + sc / 8, row, (sz - sc) / 8);
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
                    vga2_set_pixel_in_row(row, x + scroll, vga2_get_pixel_in_row(row, x));
                // fill left area with brush color
                raw_fill_row(y, x1, x1 + scroll - 1, back);
            }
        }
    }
}


void vga2_controller::draw_glyph(esp::gfx::glyph const &glyph,
                                 esp::gfx::glyph_options glyph_options, esp::gfx::rgb888 pen_color,
                                 esp::gfx::rgb888 brush_color, esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return (uint8_t *)_view_port[y]; }, vga2_set_pixel_in_row);
}


void vga2_controller::invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}


void vga2_controller::swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, [&](int y) { return (uint8_t *)_view_port[y]; },
        vga2_get_pixel_in_row, vga2_set_pixel_in_row);
}


// Slow operation!
// supports overlapping of source and dest rectangles
void vga2_controller::copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return (uint8_t *)_view_port[y]; }, vga2_get_pixel_in_row,
        vga2_set_pixel_in_row);
}


// no bounds check is done!
void vga2_controller::read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = (uint8_t *)_view_port[y];
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[vga2_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 x 3 = 255
        }
    }
}


void vga2_controller::raw_draw_bitmap_native(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                             int x1, int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, (uint8_t *)bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga2_set_pixel_in_row);
}


void vga2_controller::raw_draw_bitmap_mask(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                           void *save_background, int x1, int y1, int x_count,
                                           int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga2_get_pixel_in_row, [&](uint8_t *row, int x)
        { vga2_set_pixel_in_row(row, x, foreground_color_index); } // rawSetPixelInRow
    );
}


void vga2_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        vga2_get_pixel_in_row, [&](uint8_t *row, int x, uint8_t src)
        { vga2_set_pixel_in_row(row, x, rgb2222_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void vga2_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                               esp::gfx::bitmap const *bitmap,
                                               void *save_background, int x1, int y1, int x_count,
                                               int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, (uint8_t *)save_background, x1, y1, x_count, y_count,
        [&](int y) { return (uint8_t *)_view_port[y]; }, // rawGetRow
        [&](uint8_t *row, int x) { return vga2_get_pixel_in_row(row, x); }, // rawGetPixelInRow
        [&](uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        { vga2_set_pixel_in_row(row, x, rgb8888_to_palette_index(src)); } // rawSetPixelInRow
    );
}


void IRAM_ATTR vga2_controller::isr_handler(void *arg)
{
#if ESP_VGA_PERFORMANCE_CHECK
    auto s1 = cycle_count();
#endif

    auto ctrl = (vga2_controller *)arg;

    if (I2S1.int_st.out_eof)
    {

        auto const desc = (lldesc_t *)I2S1.out_eof_des_addr;

        if (desc == _frame_reset_desc)
            _scan_line = 0;

        auto const width = ctrl->_view_port_width;
        auto const height = ctrl->_view_port_height;
        auto const packedPaletteIndexOctet_to_signals =
            (uint64_t const *)ctrl->_packed_palette_index_octet_to_signals;
        auto const lines = ctrl->_lines;

        int scan_line = (_scan_line + VGA2_LinesCount / 2) % height;

        auto line_index = scan_line & (VGA2_LinesCount - 1);

        for (int i = 0; i < VGA2_LinesCount / 2; ++i)
        {

            auto src = (uint8_t const *)_isr_view_port_visible[scan_line];
            auto dest = (uint64_t *)lines[line_index];

            // optimization warn: horizontal resolution must be a multiple of 16!
            for (int col = 0; col < width; col += 16)
            {

                auto src1 = *(src + 0);
                auto src2 = *(src + 1);

                PSRAM_HACK;

                auto v1 = packedPaletteIndexOctet_to_signals[src1];
                auto v2 = packedPaletteIndexOctet_to_signals[src2];

                *(dest + 0) = v1;
                *(dest + 1) = v2;

                dest += 2;
                src += 2;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += VGA2_LinesCount / 2;

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
