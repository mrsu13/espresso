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
#include "esp_gfx_rgb888.h"
#include "esp_gfx_size.h"
#include "esp_vga_vga_base_controller.h"
#include "esp_vga_vga_timings.h"

#include <cstdint>


namespace esp::vga
{

/**
 * @brief Функция, которая рисует строки для esp::vga::vga_direct_controller.
 *
 * Её вызывает обработчик прерывания, поэтому она должна быть в IRAM.
 *
 * @param arg Аргумент, переданный в esp::vga::vga_direct_controller::set_draw_scanline_callback().
 * @param dest Строка DMA, которую надо заполнить байтами пикселей.
 * @param scan_line Строка экрана: от 0 до высоты − 1.
 */
using draw_scanline_callback = void (*)(void *arg, std::uint8_t *dest, int scan_line);

class vga_direct_controller : public vga_base_controller
{

public:

    explicit vga_direct_controller(bool auto_run = true);

    inline esp::gfx::native_pixel_format native_pixel_format() override;

    using vga_base_controller::set_resolution;
    void set_resolution(const vga_timings &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false) override;

    void read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf) override;

    inline void set_draw_scanline_callback(draw_scanline_callback draw_scanline_callback,
                                           void *arg = nullptr);
    void run();
    inline void set_scanlines_per_call_back(int value);

    void set_scanline_buffer(int scanline, std::uint8_t volatile *line_buffer);
    std::uint8_t volatile *scanline_buffer(int scanline);
    std::uint8_t volatile *default_scanline_buffer(int scanline);

    inline static bool v_sync();

private:

    void init() override;

    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row) override;
    void allocate_view_port() override;
    void free_view_port() override;

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

    /** @brief Строка экрана, с которой обработчик прерывания продолжит. */
    static volatile int _scan_line;

    /** @brief Дескриптор DMA первой строки области вывода: по нему обработчик узнаёт начало кадра. */
    static lldesc_t volatile *_frame_reset_desc;

    /** @brief Выведена ли последняя строка кадра: идёт обратный ход луча. */
    static bool _v_sync;

    /** @brief Дескриптор DMA каждой строки области вывода. */
    static lldesc_t volatile **_dma_lines;

    /** @brief Число строк DMA: вдвое больше строк за вызов функции рисования. */
    std::int32_t _lines_count;

    /** @brief Строки DMA, которые заполняет функция рисования. */
    std::uint8_t **_lines;

    /**
     * @brief Функция, которая рисует строки.
     *
     * Функция, а не виртуальный метод: таблицы виртуальных методов лежат во
     * флеш-памяти, и обработчик прерывания не мог бы работать из IRAM.
     */
    draw_scanline_callback _draw_scanline_callback;

    /** @brief Аргумент функции рисования. */
    void *_draw_scanline_arg;

    /** @brief Начинать ли вывод сразу после set_resolution(). */
    bool _auto_run;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает формат пикселя строк.
 * @return Байт на пиксель: синхроимпульсы и по 2 разряда на канал цвета.
 */
esp::gfx::native_pixel_format esp::vga::vga_direct_controller::native_pixel_format()
{
    return esp::gfx::native_pixel_format::sbgr2222;
}

/**
 * @brief Задаёт функцию, которая рисует строки; без неё set_resolution() ничего не делает.
 * @param draw_scanline_callback Функция; должна быть в IRAM.
 * @param arg Аргумент функции.
 */
void esp::vga::vga_direct_controller::set_draw_scanline_callback(
    esp::vga::draw_scanline_callback draw_scanline_callback, void *arg)
{
    _draw_scanline_callback = draw_scanline_callback;
    _draw_scanline_arg = arg;
}

/**
 * @brief Задаёт, сколько строк функция рисования заполняет за вызов; по умолчанию одну.
 *
 * Менять до set_resolution().
 *
 * @param value Строк за вызов.
 */
void esp::vga::vga_direct_controller::set_scanlines_per_call_back(int value)
{
    _lines_count = value * 2;
}

/**
 * @brief Сообщает, выведена ли последняя строка кадра: идёт обратный ход луча.
 * @return @c true во время обратного хода луча.
 */
bool esp::vga::vga_direct_controller::v_sync()
{
    return _v_sync;
}

// Закрытые

/**
 * @brief Возвращает размер пикселя сохранённого под спрайтом фона.
 * @return 1 байт.
 */
int esp::vga::vga_direct_controller::bitmap_save_pixel_size()
{
    return 1;
}
