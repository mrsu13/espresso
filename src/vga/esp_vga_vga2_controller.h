// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_bitmap.h"
#include "esp_gfx_glyph.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_pixel_desc.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_size.h"
#include "esp_vga_vga_paletted_controller.h"

#include <cstdint>


namespace esp::vga
{

class vga2_controller : public vga_paletted_controller
{

public:

    vga2_controller();
    ~vga2_controller();

    void read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf) override;
    void set_palette_item(int index, const esp::gfx::rgb888 &color);

protected:

    void setup_default_palette() override;

private:

    void set_pixel_at(const esp::gfx::pixel_desc &pixel_desc, esp::gfx::rect &update_rect) override;
    void draw_ellipse(const esp::gfx::size &size, esp::gfx::rect &update_rect) override;
    void clear(esp::gfx::rect &update_rect) override;
    void v_scroll(int scroll, esp::gfx::rect &update_rect) override;
    void h_scroll(int scroll, esp::gfx::rect &update_rect) override;
    void draw_glyph(const esp::gfx::glyph &glyph, esp::gfx::glyph_options glyph_options,
                    esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                    esp::gfx::rect &update_rect) override;
    void invert_rect(const esp::gfx::rect &rect, esp::gfx::rect &update_rect) override;
    void copy_rect(const esp::gfx::rect &source, esp::gfx::rect &update_rect) override;
    void swap_fgbg(const esp::gfx::rect &rect, esp::gfx::rect &update_rect) override;
    void raw_draw_bitmap_native(int dest_x, int dest_y, const esp::gfx::bitmap *bitmap, int x1,
                                int y1, int x_count, int y_count) override;
    void raw_draw_bitmap_mask(int dest_x, int dest_y, const esp::gfx::bitmap *bitmap,
                              void *save_background, int x1, int y1, int x_count,
                              int y_count) override;
    void raw_draw_bitmap_rgba2222(int dest_x, int dest_y, const esp::gfx::bitmap *bitmap,
                                  void *save_background, int x1, int y1, int x_count,
                                  int y_count) override;
    void raw_draw_bitmap_rgba8888(int dest_x, int dest_y, const esp::gfx::bitmap *bitmap,
                                  void *save_background, int x1, int y1, int x_count,
                                  int y_count) override;
    void raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color) override;
    void raw_fill_row(int y, int x1, int x2, std::uint8_t color_index);
    void raw_invert_row(int y, int x1, int x2);
    void raw_copy_row(int x1, int x2, int src_y, int dst_y);
    void swap_rows(int y_a, int y_b, int x1, int x2);
    void abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color) override;
    inline int bitmap_save_pixel_size() override;

    static void isr_handler(void *arg);

    /** @brief Сигналы VGA для каждого байта буфера: 8 байтов пикселей. */
    volatile std::uint64_t *_packed_palette_index_octet_to_signals;
};

}


// Реализация

// Закрытые

/**
 * @brief Возвращает размер пикселя сохранённого под спрайтом фона.
 * @return 1 байт: номер цвета палитры.
 */
int esp::vga::vga2_controller::bitmap_save_pixel_size()
{
    return 1;
}
