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


#include <stdarg.h>

#include "esp_gfx_canvas.h"


#pragma GCC optimize("O2")


namespace esp::gfx
{


#define INVALIDRECT esp::gfx::rect(-32768, -32768, -32768, -32768)


canvas::canvas(bitmapped_display_controller *display_controller)
    : _display_controller(display_controller)
    , _font_info(nullptr)
    , _text_horiz_rate(1)
    , _origin(point(0, 0))
    , _clipping_rect(INVALIDRECT)
{
}


void canvas::set_origin(int x, int y)
{
    set_origin(point(x, y));
}


void canvas::set_origin(point const &origin)
{
    primitive p;
    p.cmd = primitive_cmd::set_origin;
    p.position = _origin = origin;
    _display_controller->add_primitive(p);
}


void canvas::set_clipping_rect(rect const &rect)
{
    primitive p;
    p.cmd = primitive_cmd::set_clipping_rect;
    p.rect = _clipping_rect = rect;
    _display_controller->add_primitive(p);
}


rect canvas::clipping_rect()
{
    if (_clipping_rect == INVALIDRECT)
        _clipping_rect = rect(0, 0, width() - 1, height() - 1);
    return _clipping_rect;
}


void canvas::wait_completion(bool wait_v_sync)
{
    if (wait_v_sync)
        _display_controller->primitives_execution_wait(); // wait on VSync normal processing
    else
        _display_controller->process_primitives(); // process right now!
}


// Warning: beginUpdate() disables vertical sync interrupts. This means that
// the BitmappedDisplayController primitives queue is not processed, and adding primitives may
// cause a deadlock. To avoid this a call to "Canvas.waitCompletion(false)"
// should be performed very often.
void canvas::begin_update()
{
    _display_controller->suspend_background_primitive_execution();
}


void canvas::end_update()
{
    _display_controller->resume_background_primitive_execution();
}


void canvas::clear()
{
    primitive p;
    p.cmd = primitive_cmd::clear;
    p.ivalue = 0;
    _display_controller->add_primitive(p);
}


void canvas::reset()
{
    primitive p;
    p.cmd = primitive_cmd::reset;
    _display_controller->add_primitive(p);
    _origin = point(0, 0);
    _clipping_rect = INVALIDRECT;
    _text_horiz_rate = 1;
}


void canvas::scroll(int offset_x, int offset_y)
{
    primitive p;
    if (offset_y != 0)
    {
        p.cmd = primitive_cmd::v_scroll;
        p.ivalue = offset_y;
        _display_controller->add_primitive(p);
    }
    if (offset_x != 0)
    {
        p.cmd = primitive_cmd::h_scroll;
        p.ivalue = offset_x;
        _display_controller->add_primitive(p);
    }
}


void canvas::set_scrolling_region(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::set_scrolling_region;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}


void canvas::set_pixel(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::set_pixel;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}


void canvas::set_pixel(int x, int y, rgb888 const &color)
{
    set_pixel(point(x, y), color);
}


void canvas::set_pixel(point const &pos, rgb888 const &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_pixel_at;
    p.pixel_desc = {pos, color};
    _display_controller->add_primitive(p);
}


void canvas::move_to(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::move_to;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}


void canvas::set_pen_color(color color)
{
    set_pen_color(rgb888(color));
}


void canvas::set_pen_color(uint8_t red, uint8_t green, uint8_t blue)
{
    set_pen_color(rgb888(red, green, blue));
}


void canvas::set_pen_color(rgb888 const &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_pen_color;
    p.color = color;
    _display_controller->add_primitive(p);
}


void canvas::set_brush_color(color color)
{
    set_brush_color(rgb888(color));
}


void canvas::set_brush_color(uint8_t red, uint8_t green, uint8_t blue)
{
    set_brush_color(rgb888(red, green, blue));
}


void canvas::set_pen_width(int value)
{
    primitive p;
    p.cmd = primitive_cmd::set_pen_width;
    p.ivalue = value;
    _display_controller->add_primitive(p);
}


void canvas::set_line_ends(line_ends value)
{
    primitive p;
    p.cmd = primitive_cmd::set_line_ends;
    p.line_ends = value;
    _display_controller->add_primitive(p);
}


void canvas::set_brush_color(rgb888 const &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_brush_color;
    p.color = color;
    _display_controller->add_primitive(p);
}


void canvas::line_to(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::line_to;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}


void canvas::draw_line(int x1, int y1, int x2, int y2)
{
    move_to(x1, y1);
    line_to(x2, y2);
}


void canvas::draw_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::draw_rect;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}


void canvas::draw_rectangle(rect const &rect)
{
    draw_rectangle(rect.x1, rect.y1, rect.x2, rect.y2);
}


void canvas::fill_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::fill_rect;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}


void canvas::fill_rectangle(rect const &rect)
{
    primitive p;
    p.cmd = primitive_cmd::fill_rect;
    p.rect = rect;
    _display_controller->add_primitive(p);
}


void canvas::invert_rectangle(int x1, int y1, int x2, int y2)
{
    invert_rectangle(rect(x1, y1, x2, y2));
}


void canvas::invert_rectangle(rect const &rect)
{
    primitive p;
    p.cmd = primitive_cmd::invert_rect;
    p.rect = rect;
    _display_controller->add_primitive(p);
}


void canvas::swap_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::swap_fgbg;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}


void canvas::fill_ellipse(int x, int y, int width, int height)
{
    move_to(x, y);
    primitive p;
    p.cmd = primitive_cmd::fill_ellipse;
    p.size = size(width, height);
    _display_controller->add_primitive(p);
}


void canvas::draw_ellipse(int x, int y, int width, int height)
{
    move_to(x, y);
    primitive p;
    p.cmd = primitive_cmd::draw_ellipse;
    p.size = size(width, height);
    _display_controller->add_primitive(p);
}


void canvas::draw_glyph(int x, int y, int width, int height, uint8_t const *data, int index)
{
    primitive p;
    p.cmd = primitive_cmd::draw_glyph;
    p.glyph = glyph(x, y, width, height, data + index * height * ((width + 7) / 8));
    _display_controller->add_primitive(p);
}


void canvas::set_glyph_options(glyph_options options)
{
    primitive p;
    p.cmd = primitive_cmd::set_glyph_options;
    p.glyph_options = options;
    _display_controller->add_primitive(p);
    _text_horiz_rate = options.double_width > 0 ? 2 : 1;
}


void canvas::reset_glyph_options()
{
    set_glyph_options(glyph_options());
}


void canvas::set_paint_options(paint_options options)
{
    primitive p;
    p.cmd = primitive_cmd::set_paint_options;
    p.paint_options = options;
    _display_controller->add_primitive(p);
}


void canvas::reset_paint_options()
{
    set_paint_options(paint_options());
}


void canvas::select_font(font_info const *font_info)
{
    _font_info = font_info;
}


void canvas::draw_char(int x, int y, char c)
{
    draw_glyph(x, y, _font_info->width, _font_info->height, _font_info->data, c);
}


void canvas::draw_text(int x, int y, char const *text, bool wrap)
{
    // no built-in fonts: without selectFont() there is nothing to draw with
    if (_font_info == nullptr)
        return;
    draw_text(_font_info, x, y, text, wrap);
}


void canvas::draw_text(font_info const *font_info, int x, int y, char const *text, bool wrap)
{
    int font_width = font_info->width;
    for (; *text; ++text, x += font_width * _text_horiz_rate)
    {
        if (wrap && x >= width())
        { // TODO: clipX2 instead of getWidth()?
            x = 0;
            y += font_info->height;
        }
        if (font_info->chptr)
        {
            // variable width
            uint8_t const *chptr = font_info->data + font_info->chptr[(int)(*text)];
            font_width = *chptr++;
            draw_glyph(x, y, font_width, font_info->height, chptr, 0);
        }
        else
        {
            // fixed width
            draw_glyph(x, y, font_info->width, font_info->height, font_info->data, *text);
        }
    }
}


void canvas::draw_text_with_ellipsis(font_info const *font_info, int x, int y, char const *text,
                                     int max_x)
{
    int font_width = font_info->width;
    int font_height = font_info->height;
    for (; *text; ++text, x += font_width)
    {
        if (x >= max_x - font_height)
        {
            // draw ellipsis and exit
            draw_text(font_info, x, y, "...");
            break;
        }
        if (font_info->chptr)
        {
            // variable width
            uint8_t const *chptr = font_info->data + font_info->chptr[(int)(*text)];
            font_width = *chptr++;
            draw_glyph(x, y, font_width, font_info->height, chptr, 0);
        }
        else
        {
            // fixed width
            draw_glyph(x, y, font_info->width, font_info->height, font_info->data, *text);
        }
    }
}


int canvas::text_extent(font_info const *font_info, char const *text)
{
    int font_width = font_info->width;
    int extent = 0;
    for (; *text; ++text, extent += font_width)
    {
        if (font_info->chptr)
        {
            // variable width
            uint8_t const *chptr = font_info->data + font_info->chptr[(int)(*text)];
            font_width = *chptr;
        }
    }
    return extent;
}


int canvas::text_extent(char const *text)
{
    return text_extent(_font_info, text);
}


void canvas::draw_text_fmt(int x, int y, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int size = vsnprintf(nullptr, 0, format, ap) + 1;
    if (size > 0)
    {
        va_end(ap);
        va_start(ap, format);
        char buf[size + 1];
        vsnprintf(buf, size, format, ap);
        draw_text(x, y, buf, false);
    }
    va_end(ap);
}


void canvas::copy_rect(int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
    move_to(dest_x, dest_y);
    int source_x2 = source_x + width - 1;
    int source_y2 = source_y + height - 1;
    primitive p;
    p.cmd = primitive_cmd::copy_rect;
    p.rect = rect(source_x, source_y, source_x2, source_y2);
    _display_controller->add_primitive(p);
}


void canvas::draw_bitmap(int x, int y, bitmap const *bitmap)
{
    primitive p;
    p.cmd = primitive_cmd::draw_bitmap;
    p.bitmap_drawing_info = bitmap_drawing_info(x, y, bitmap);
    _display_controller->add_primitive(p);
}


void canvas::swap_buffers()
{
    primitive p;
    p.cmd = primitive_cmd::swap_buffers;
    p.notify_task = xTaskGetCurrentTaskHandle();
    _display_controller->add_primitive(p);
    _display_controller->primitives_execution_wait();
}


void canvas::draw_path(point const *points, int points_count)
{
    primitive p;
    p.cmd = primitive_cmd::draw_path;
    p.path.points = points;
    p.path.points_count = points_count;
    p.path.free_points = false;
    _display_controller->add_primitive(p);
}


void canvas::fill_path(point const *points, int points_count)
{
    primitive p;
    p.cmd = primitive_cmd::fill_path;
    p.path.points = points;
    p.path.points_count = points_count;
    p.path.free_points = false;
    _display_controller->add_primitive(p);
}


rgb888 canvas::pixel(int x, int y)
{
    rgb888 rgb;
    _display_controller->read_screen(rect(x, y, x, y), &rgb);
    return rgb;
}


} // end of namespace
