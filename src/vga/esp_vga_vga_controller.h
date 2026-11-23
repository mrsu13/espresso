// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_bitmap.h"
#include "esp_gfx_glyph.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_native_pixel_format.h"
#include "esp_gfx_pixel_desc.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb222.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_size.h"
#include "esp_vga_vga_base_controller.h"
#include "esp_vga_vga_timings.h"

#include <cstdint>


namespace esp::vga
{

class vga_controller : public vga_base_controller
{

public:

    vga_controller();

    void suspend_background_primitive_execution() override;
    void resume_background_primitive_execution() override;

    inline esp::gfx::native_pixel_format native_pixel_format() override;

    using vga_base_controller::set_resolution;
    void set_resolution(const vga_timings &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false) override;

    void read_screen(const esp::gfx::rect &rect, esp::gfx::rgb222 *dest_buf);
    void read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf) override;
    void write_screen(const esp::gfx::rect &rect, esp::gfx::rgb222 *src_buf);

    inline void set_raw_pixel(int x, int y, std::uint8_t rgb);

private:

    void init() override;

    void allocate_view_port() override;
    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row) override;

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
    void raw_fill_row(int y, int x1, int x2, std::uint8_t pattern);
    void raw_invert_row(int y, int x1, int x2);
    void swap_rows(int y_a, int y_b, int x1, int x2);
    void abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color) override;
    inline int bitmap_save_pixel_size() override;

    static void v_sync_interrupt(void *arg);

    /** @brief Сколько микросекунд обработчик прерывания может рисовать примитивы за кадр. */
    volatile std::int16_t _max_v_sync_isr_time;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает формат пикселя буфера кадра.
 * @return Байт на пиксель: синхроимпульсы и по 2 разряда на канал цвета.
 */
esp::gfx::native_pixel_format esp::vga::vga_controller::native_pixel_format()
{
    return esp::gfx::native_pixel_format::sbgr2222;
}

/**
 * @brief Записывает байт пикселя (@p x, @p y) буфера рисования без очереди примитивов.
 *
 * Байт пикселя даёт create_raw_pixel():
 * @code
 * display.set_raw_pixel(100, 100, display.create_raw_pixel(esp::gfx::rgb222{3, 0, 0}));
 * @endcode
 *
 * @param x Столбец.
 * @param y Строка.
 * @param rgb Байт пикселя.
 */
void esp::vga::vga_controller::set_raw_pixel(int x, int y, std::uint8_t rgb)
{
    pixel(x, y) = rgb;
}

// Закрытые

/**
 * @brief Возвращает размер пикселя сохранённого под спрайтом фона.
 * @return 1 байт: байт пикселя.
 */
int esp::vga::vga_controller::bitmap_save_pixel_size()
{
    return 1;
}
