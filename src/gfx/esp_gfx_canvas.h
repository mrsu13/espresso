// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_bitmap.h"
#include "esp_gfx_bitmapped_display_controller.h"
#include "esp_gfx_color.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_line_ends.h"
#include "esp_gfx_paint_options.h"
#include "esp_gfx_point.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"

#include <cstdint>


namespace esp::gfx
{

class canvas
{

public:

    explicit canvas(bitmapped_display_controller *display_controller);

    inline int width() const;
    inline int height() const;

    void wait_completion(bool wait_v_sync = true);
    void begin_update();
    void end_update();
    void swap_buffers();

    void set_origin(int x, int y);
    void set_origin(const point &origin);
    inline point origin() const;
    void set_clipping_rect(const rect &rect);
    rect clipping_rect();

    void clear();
    void reset();
    void set_scrolling_region(int x1, int y1, int x2, int y2);
    void scroll(int offset_x, int offset_y);

    void move_to(int x, int y);
    void set_pen_color(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
    void set_pen_color(color color);
    void set_pen_color(const rgb888 &color);
    void set_brush_color(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
    void set_brush_color(color color);
    void set_brush_color(const rgb888 &color);
    void set_pen_width(int value);
    void set_line_ends(line_ends value);

    void set_pixel(int x, int y);
    void set_pixel(int x, int y, const rgb888 &color);
    void set_pixel(const point &pos, const rgb888 &color);
    void line_to(int x, int y);
    void draw_line(int x1, int y1, int x2, int y2);
    void draw_rectangle(int x1, int y1, int x2, int y2);
    void draw_rectangle(const rect &rect);
    void fill_rectangle(int x1, int y1, int x2, int y2);
    void fill_rectangle(const rect &rect);
    void invert_rectangle(int x1, int y1, int x2, int y2);
    void invert_rectangle(const rect &rect);
    void swap_rectangle(int x1, int y1, int x2, int y2);
    void draw_ellipse(int x, int y, int width, int height);
    void fill_ellipse(int x, int y, int width, int height);
    void draw_glyph(int x, int y, int width, int height, const std::uint8_t *data, int index = 0);
    void set_glyph_options(glyph_options options);
    void reset_glyph_options();
    void set_paint_options(paint_options options);
    void reset_paint_options();
    void copy_rect(int source_x, int source_y, int dest_x, int dest_y, int width, int height);
    void draw_bitmap(int x, int y, const bitmap *bitmap);
    void draw_path(const point *points, int points_count);
    void fill_path(const point *points, int points_count);

    rgb888 pixel(int x, int y);

private:

    /** @brief Контроллер дисплея, в очередь которого холст ставит примитивы. */
    bitmapped_display_controller *_display_controller;

    /** @brief Последнее начало координат, заданное set_origin(). */
    point _origin;

    /**
     * @brief Последняя область отсечения, заданная set_clipping_rect().
     *
     * Пока область не задана, здесь «недействительный» прямоугольник;
     * clipping_rect() тогда возвращает всю область вывода.
     */
    rect _clipping_rect;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает ширину холста — ширину области вывода контроллера.
 * @return Ширина, пиксели.
 */
int esp::gfx::canvas::width() const
{
    return _display_controller->view_port_width();
}

/**
 * @brief Возвращает высоту холста — высоту области вывода контроллера.
 * @return Высота, пиксели.
 */
int esp::gfx::canvas::height() const
{
    return _display_controller->view_port_height();
}

/**
 * @brief Возвращает начало координат, заданное set_origin().
 * @return Начало координат.
 */
esp::gfx::point esp::gfx::canvas::origin() const
{
    return _origin;
}
