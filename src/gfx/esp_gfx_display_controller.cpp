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


#include "esp_gfx_display_controller.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "freertos/task.h"

#include "esp_gfx_cursors.h"
#include "esp_gfx_utils.h"
#include "esp_tools.h"


#pragma GCC optimize("O2")


namespace esp::gfx
{


// Array to convert Color enum to RGB888 struct
const rgb888 COLOR2RGB888[16] = {
    {0, 0, 0}, // Black
    {128, 0, 0}, // Red
    {0, 128, 0}, // Green
    {128, 128, 0}, // Yellow
    {0, 0, 128}, // Blue
    {128, 0, 128}, // Magenta
    {0, 128, 128}, // Cyan
    {128, 128, 128}, // White
    {64, 64, 64}, // BrightBlack
    {255, 0, 0}, // BrightRed
    {0, 255, 0}, // BrightGreen
    {255, 255, 0}, // BrightYellow
    {0, 0, 255}, // BrightBlue
    {255, 0, 255}, // BrightMagenta
    {0, 255, 255}, // BrightCyan
    {255, 255, 255}, // BrightWhite
};


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RGB222 implementation

bool rgb222::low_bit_only = false;


//   0 ..  63 => 0
//  64 .. 127 => 1
// 128 .. 191 => 2
// 192 .. 255 => 3
rgb222::rgb222(rgb888 const &value)
{
    if (low_bit_only)
    {
        r = value.r ? 3 : 0;
        g = value.g ? 3 : 0;
        b = value.b ? 3 : 0;
    }
    else
    {
        r = value.r >> 6;
        g = value.g >> 6;
        b = value.b >> 6;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RGB888 implementation


rgb888::rgb888(color color)
{
    *this = COLOR2RGB888[(int)color];
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RGB888toPackedRGB222()

uint8_t rgb888_to_packed_rgb222(rgb888 const &rgb)
{
    // 64 colors
    static const int CONVR64[4] = {
        0 << 0, // 00XXXXXX (0..63)
        1 << 0, // 01XXXXXX (64..127)
        2 << 0, // 10XXXXXX (128..191)
        3 << 0,
    }; // 11XXXXXX (192..255)
    static const int CONVG64[4] = {
        0 << 2, // 00XXXXXX (0..63)
        1 << 2, // 01XXXXXX (64..127)
        2 << 2, // 10XXXXXX (128..191)
        3 << 2,
    }; // 11XXXXXX (192..255)
    static const int CONVB64[4] = {
        0 << 4, // 00XXXXXX (0..63)
        1 << 4, // 01XXXXXX (64..127)
        2 << 4, // 10XXXXXX (128..191)
        3 << 4,
    }; // 11XXXXXX (192..255)
    // 8 colors
    static const int CONVR8[4] = {
        0 << 0, // 00XXXXXX (0..63)
        3 << 0, // 01XXXXXX (64..127)
        3 << 0, // 10XXXXXX (128..191)
        3 << 0,
    }; // 11XXXXXX (192..255)
    static const int CONVG8[4] = {
        0 << 2, // 00XXXXXX (0..63)
        3 << 2, // 01XXXXXX (64..127)
        3 << 2, // 10XXXXXX (128..191)
        3 << 2,
    }; // 11XXXXXX (192..255)
    static const int CONVB8[4] = {
        0 << 4, // 00XXXXXX (0..63)
        3 << 4, // 01XXXXXX (64..127)
        3 << 4, // 10XXXXXX (128..191)
        3 << 4,
    }; // 11XXXXXX (192..255)

    if (rgb222::low_bit_only)
        return (CONVR8[rgb.r >> 6]) | (CONVG8[rgb.g >> 6]) | (CONVB8[rgb.b >> 6]);
    else
        return (CONVR64[rgb.r >> 6]) | (CONVG64[rgb.g >> 6]) | (CONVB64[rgb.b >> 6]);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// Sprite implementation


sprite::sprite()
{
    x = 0;
    y = 0;
    current_frame = 0;
    frames = nullptr;
    frames_count = 0;
    saved_background_width = 0;
    saved_background_height = 0;
    saved_background = nullptr; // allocated or reallocated when bitmaps are added
    saved_x = 0;
    saved_y = 0;
    visible = true;
    is_static = false;
    allow_draw = true;
}


sprite::~sprite()
{
    free(frames);
    free(saved_background);
}


void sprite::clear_bitmaps()
{
    free(frames);
    frames = nullptr;
    frames_count = 0;
}


sprite *sprite::add_bitmap(bitmap *bitmap)
{
    ++frames_count;
    frames = (esp::gfx::bitmap **)realloc(frames, sizeof(esp::gfx::bitmap *) * frames_count);
    frames[frames_count - 1] = bitmap;
    return this;
}


sprite *sprite::add_bitmap(bitmap *bitmap[], int count)
{
    frames =
        (esp::gfx::bitmap **)realloc(frames, sizeof(esp::gfx::bitmap *) * (frames_count + count));
    for (int i = 0; i < count; ++i)
        frames[frames_count + i] = bitmap[i];
    frames_count += count;
    return this;
}


sprite *sprite::move_by(int offset_x, int offset_y)
{
    x += offset_x;
    y += offset_y;
    return this;
}


sprite *sprite::move_by(int offset_x, int offset_y, int wrap_around_width, int wrap_around_height)
{
    x += offset_x;
    y += offset_y;
    if (x > wrap_around_width)
        x = -(int)width();
    if (x < -(int)width())
        x = wrap_around_width;
    if (y > wrap_around_height)
        y = -(int)height();
    if (y < -(int)height())
        y = wrap_around_height;
    return this;
}


sprite *sprite::move_to(int x, int y)
{
    this->x = x;
    this->y = y;
    return this;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap implementation


bitmap::bitmap(int width_, int height_, void const *data_, pixel_format format_,
               rgb888 foreground_color_, bool copy)
    : width(width_)
    , height(height_)
    , format(format_)
    , foreground_color(foreground_color_)
    , data((uint8_t *)data_)
    , data_allocated(false)
{
    if (copy)
    {
        allocate();
        copy_from(data_);
    }
}


bitmap::bitmap(int width_, int height_, void const *data_, pixel_format format_, bool copy)
    : bitmap(width_, height_, data_, format_, rgb888(255, 255, 255), copy)
{
}


void bitmap::allocate()
{
    if (data_allocated)
    {
        free((void *)data);
        data = nullptr;
    }
    data_allocated = true;
    switch (format)
    {
        case pixel_format::undefined:
        case pixel_format::native:
            break;
        case pixel_format::mask:
            data = (uint8_t *)heap_caps_malloc((width + 7) / 8 * height,
                                               MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            break;
        case pixel_format::rgba2222:
            data =
                (uint8_t *)heap_caps_malloc(width * height, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            break;
        case pixel_format::rgba8888:
            data = (uint8_t *)heap_caps_malloc(width * height * 4,
                                               MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            break;
    }
}


// data must have the same pixel format
void bitmap::copy_from(void const *src_data)
{
    switch (format)
    {
        case pixel_format::undefined:
        case pixel_format::native:
            break;
        case pixel_format::mask:
            memcpy(data, src_data, (width + 7) / 8 * height);
            break;
        case pixel_format::rgba2222:
            memcpy(data, src_data, width * height);
            break;
        case pixel_format::rgba8888:
            memcpy(data, src_data, width * height * 4);
            break;
    }
}


void bitmap::set_pixel(int x, int y, int value)
{
    int rowlen = (width + 7) / 8;
    uint8_t *rowptr = data + y * rowlen;
    if (value)
        rowptr[x >> 3] |= 0x80 >> (x & 7);
    else
        rowptr[x >> 3] &= ~(0x80 >> (x & 7));
}


void bitmap::set_pixel(int x, int y, rgba2222 value)
{
    ((rgba2222 *)data)[y * width + x] = value;
}


void bitmap::set_pixel(int x, int y, rgba8888 value)
{
    ((rgba8888 *)data)[y * width + x] = value;
}


int bitmap::alpha(int x, int y)
{
    int r = 0;
    switch (format)
    {
        case pixel_format::undefined:
            break;
        case pixel_format::native:
            r = 0xff;
            break;
        case pixel_format::mask:
        {
            int rowlen = (width + 7) / 8;
            uint8_t *rowptr = data + y * rowlen;
            r = (rowptr[x >> 3] >> (7 - (x & 7))) & 1;
            break;
        }
        case pixel_format::rgba2222:
            r = ((rgba2222 *)data)[y * height + x].a;
            break;
        case pixel_format::rgba8888:
            r = ((rgba8888 *)data)[y * height + x].a;
            break;
    }
    return r;
}


bitmap::~bitmap()
{
    if (data_allocated)
        heap_caps_free((void *)data);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// BitmappedDisplayController implementation


int bitmapped_display_controller::queue_size = FABGLIB_DEFAULT_DISPLAYCONTROLLER_QUEUE_SIZE;


bitmapped_display_controller::bitmapped_display_controller()
    : _prim_dyn_mem_pool(FABGLIB_PRIMITIVES_DYNBUFFERS_SIZE)
{
    _exec_queue = nullptr;
    _exec_queue_alt = nullptr;
    _background_primitive_execution_enabled = true;
    _sprites = nullptr;
    _sprites_count = 0;
    _double_buffered_setup = false;
    _double_buffered_enabled = false;
    _mouse_cursor.visible = false;
    _background_primitive_timeout_enabled = true;
    _sprites_hidden = true;
}


bitmapped_display_controller::~bitmapped_display_controller()
{
    vQueueDelete(_exec_queue);
    if (_exec_queue_alt)
        vQueueDelete(_exec_queue_alt);
}


void bitmapped_display_controller::set_double_buffered(bool value)
{
    _double_buffered_setup = value;
    _double_buffered_enabled = value;
    if (_exec_queue)
        vQueueDelete(_exec_queue);
    // on double buffering a queue of single element is enough and necessary (see addPrimitive() for details)
    _exec_queue =
        xQueueCreate(value ? 1 : bitmapped_display_controller::queue_size, sizeof(primitive));
}


// return previous value
bool bitmapped_display_controller::suspend_double_buffering(bool value)
{
    auto ret = !_double_buffered_enabled;
    if (_double_buffered_setup && _double_buffered_enabled == value)
    {
        process_primitives();
        if (value && !_exec_queue_alt)
        {
            // need to create alternate primitives queue
            _exec_queue_alt =
                xQueueCreate(bitmapped_display_controller::queue_size, sizeof(primitive));
        }
        esp::tswap(_exec_queue_alt, _exec_queue);
        _double_buffered_enabled = !value;
    }
    return ret;
}


void IRAM_ATTR bitmapped_display_controller::reset_paint_state()
{
    _paint_state.pen_color = rgb888(255, 255, 255);
    _paint_state.brush_color = rgb888(0, 0, 0);
    _paint_state.position = point(0, 0);
    _paint_state.glyph_options.value = 0; // all options: 0
    _paint_state.paint_options = paint_options();
    _paint_state.scrolling_region = rect(0, 0, view_port_width() - 1, view_port_height() - 1);
    _paint_state.origin = point(0, 0);
    _paint_state.clipping_rect = rect(0, 0, view_port_width() - 1, view_port_height() - 1);
    _paint_state.abs_clipping_rect = _paint_state.clipping_rect;
    _paint_state.pen_width = 1;
    _paint_state.line_ends = line_ends::none;
}


void bitmapped_display_controller::add_primitive(primitive &primitive)
{
    if ((_background_primitive_execution_enabled && _double_buffered_enabled == false)
        || primitive.cmd == primitive_cmd::swap_buffers)
    {
        primitive_replace_dynamic_buffers(primitive);
        xQueueSendToBack(_exec_queue, &primitive, portMAX_DELAY);

        if (_double_buffered_enabled)
        {
            // wait notify from PrimitiveCmd::SwapBuffers executor
            ulTaskNotifyTake(true, portMAX_DELAY);
        }
    }
    else
    {
        rect update_rect = rect(SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN);
        exec_primitive(primitive, update_rect, false);
        show_sprites(update_rect);
    }
}


// some primitives require additional buffers (like drawPath and fillPath).
// this function copies primitive data into an allocated buffer (using LightMemoryPool allocator) that
// will be released inside primitive drawing code.
void bitmapped_display_controller::primitive_replace_dynamic_buffers(primitive &primitive)
{
    switch (primitive.cmd)
    {
        case primitive_cmd::draw_path:
        case primitive_cmd::fill_path:
        {
            int sz = primitive.path.points_count * sizeof(point);
            if (sz < FABGLIB_PRIMITIVES_DYNBUFFERS_SIZE)
            {
                void *newbuf = nullptr;
                // wait until we have enough free space
                while ((newbuf = _prim_dyn_mem_pool.alloc(sz)) == nullptr)
                    taskYIELD();
                memcpy(newbuf, primitive.path.points, sz);
                primitive.path.points = (point *)newbuf;
                primitive.path.free_points = true;
            }
            break;
        }

        default:
            break;
    }
}


// call this only inside an ISR
bool IRAM_ATTR bitmapped_display_controller::get_primitive_isr(primitive *primitive)
{
    return xQueueReceiveFromISR(_exec_queue, primitive, nullptr);
}


bool bitmapped_display_controller::get_primitive(primitive *primitive, int time_out_ms)
{
    return xQueueReceive(_exec_queue, primitive, esp::ms_to_ticks(time_out_ms));
}


// cannot be called inside an ISR
void bitmapped_display_controller::wait_for_primitives()
{
    primitive p;
    xQueuePeek(_exec_queue, &p, portMAX_DELAY);
}


void bitmapped_display_controller::primitives_execution_wait()
{
    if (_background_primitive_execution_enabled)
    {
        while (uxQueueMessagesWaiting(_exec_queue) > 0)
            ;
    }
}


// When false primitives are executed immediately, otherwise they are added to the primitive queue
// When set to false the queue is emptied executing all pending primitives
// Cannot be nested
void bitmapped_display_controller::enable_background_primitive_execution(bool value)
{
    if (value != _background_primitive_execution_enabled)
    {
        if (value)
        {
            resume_background_primitive_execution();
        }
        else
        {
            suspend_background_primitive_execution();
            process_primitives();
        }
        _background_primitive_execution_enabled = value;
    }
}


// Use for fast queue processing. Warning, may generate flickering because don't care of vertical sync
// Do not call inside ISR
void IRAM_ATTR bitmapped_display_controller::process_primitives()
{
    suspend_background_primitive_execution();
    rect update_rect = rect(SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN);
    primitive prim;
    while (xQueueReceive(_exec_queue, &prim, 0) == pdTRUE)
        exec_primitive(prim, update_rect, false);
    show_sprites(update_rect);
    resume_background_primitive_execution();
    primitive p(primitive_cmd::refresh, update_rect);
    add_primitive(p);
}


void bitmapped_display_controller::set_sprites(sprite *sprites, int count, int sprite_size)
{
    process_primitives();
    primitives_execution_wait();
    _sprites = sprites;
    _sprite_size = sprite_size;
    _sprites_count = count;

    // allocates background buffer
    if (!is_double_buffered_enabled())
    {
        uint8_t *sprite_ptr = (uint8_t *)_sprites;
        for (int i = 0; i < _sprites_count; ++i, sprite_ptr += _sprite_size)
        {
            sprite *sprite = (esp::gfx::sprite *)sprite_ptr;
            int req_back_buffer_size = 0;
            for (int i = 0; i < sprite->frames_count; ++i)
                req_back_buffer_size = esp::tmax(req_back_buffer_size,
                                                 sprite->frames[i]->width * bitmap_save_pixel_size()
                                                     * sprite->frames[i]->height);
            if (req_back_buffer_size > 0)
                sprite->saved_background =
                    (uint8_t *)realloc(sprite->saved_background, req_back_buffer_size);
        }
    }
}


sprite *IRAM_ATTR bitmapped_display_controller::get_sprite(int index)
{
    return (sprite *)((uint8_t *)_sprites + index * _sprite_size);
}


void bitmapped_display_controller::refresh_sprites()
{
    primitive p(primitive_cmd::refresh_sprites);
    add_primitive(p);
}


void IRAM_ATTR bitmapped_display_controller::hide_sprites(esp::gfx::rect &update_rect)
{
    if (!_sprites_hidden)
    {
        _sprites_hidden = true;

        // normal sprites
        if (sprites_count() > 0 && !is_double_buffered_enabled())
        {
            // restore saved backgrounds
            for (int i = sprites_count() - 1; i >= 0; --i)
            {
                sprite *sprite = get_sprite(i);
                if (sprite->allow_draw && sprite->saved_background_width > 0)
                {
                    int saved_x = sprite->saved_x;
                    int saved_y = sprite->saved_y;
                    int saved_width = sprite->saved_background_width;
                    int saved_height = sprite->saved_background_height;
                    bitmap bitmap(saved_width, saved_height, sprite->saved_background,
                                  pixel_format::native);
                    abs_draw_bitmap(saved_x, saved_y, &bitmap, nullptr, true);
                    update_rect = update_rect.merge(rect(
                        saved_x, saved_y, saved_x + saved_width - 1, saved_y + saved_height - 1));
                    sprite->saved_background_width = sprite->saved_background_height = 0;
                }
            }
        }

        // mouse cursor sprite
        sprite *mouse_sprite = mouse_cursor();
        if (mouse_sprite->saved_background_width > 0)
        {
            int saved_x = mouse_sprite->saved_x;
            int saved_y = mouse_sprite->saved_y;
            int saved_width = mouse_sprite->saved_background_width;
            int saved_height = mouse_sprite->saved_background_height;
            bitmap bitmap(saved_width, saved_height, mouse_sprite->saved_background,
                          pixel_format::native);
            abs_draw_bitmap(saved_x, saved_y, &bitmap, nullptr, true);
            update_rect = update_rect.merge(
                rect(saved_x, saved_y, saved_x + saved_width - 1, saved_y + saved_height - 1));
            mouse_sprite->saved_background_width = mouse_sprite->saved_background_height = 0;
        }
    }
}


void IRAM_ATTR bitmapped_display_controller::show_sprites(esp::gfx::rect &update_rect)
{
    if (_sprites_hidden)
    {
        _sprites_hidden = false;

        // normal sprites
        // save backgrounds and draw sprites
        for (int i = 0; i < sprites_count(); ++i)
        {
            sprite *sprite = get_sprite(i);
            if (sprite->visible && sprite->allow_draw && sprite->frame())
            {
                // save sprite X and Y so other threads can change them without interferring
                int sprite_x = sprite->x;
                int sprite_y = sprite->y;
                bitmap const *bitmap = sprite->frame();
                int bitmap_width = bitmap->width;
                int bitmap_height = bitmap->height;
                abs_draw_bitmap(sprite_x, sprite_y, bitmap, sprite->saved_background, true);
                sprite->saved_x = sprite_x;
                sprite->saved_y = sprite_y;
                sprite->saved_background_width = bitmap_width;
                sprite->saved_background_height = bitmap_height;
                if (sprite->is_static)
                    sprite->allow_draw = false;
                update_rect = update_rect.merge(rect(
                    sprite_x, sprite_y, sprite_x + bitmap_width - 1, sprite_y + bitmap_height - 1));
            }
        }

        // mouse cursor sprite
        // save backgrounds and draw mouse cursor
        sprite *mouse_sprite = mouse_cursor();
        if (mouse_sprite->visible && mouse_sprite->frame())
        {
            // save sprite X and Y so other threads can change them without interferring
            int sprite_x = mouse_sprite->x;
            int sprite_y = mouse_sprite->y;
            bitmap const *bitmap = mouse_sprite->frame();
            int bitmap_width = bitmap->width;
            int bitmap_height = bitmap->height;
            abs_draw_bitmap(sprite_x, sprite_y, bitmap, mouse_sprite->saved_background, true);
            mouse_sprite->saved_x = sprite_x;
            mouse_sprite->saved_y = sprite_y;
            mouse_sprite->saved_background_width = bitmap_width;
            mouse_sprite->saved_background_height = bitmap_height;
            update_rect = update_rect.merge(rect(sprite_x, sprite_y, sprite_x + bitmap_width - 1,
                                                 sprite_y + bitmap_height - 1));
        }
    }
}


// cursor = nullptr -> disable mouse
void bitmapped_display_controller::set_mouse_cursor(cursor *cursor)
{
    if (cursor == nullptr || &cursor->bitmap != _mouse_cursor.frame())
    {
        _mouse_cursor.visible = false;
        _mouse_cursor.clear_bitmaps();

        refresh_sprites();
        process_primitives();
        primitives_execution_wait();

        if (cursor)
        {
            _mouse_cursor.move_by(+_mouse_hotspot_x, +_mouse_hotspot_y);
            _mouse_hotspot_x = cursor->hotspot_x;
            _mouse_hotspot_y = cursor->hotspot_y;
            _mouse_cursor.add_bitmap(&cursor->bitmap);
            _mouse_cursor.visible = true;
            _mouse_cursor.move_by(-_mouse_hotspot_x, -_mouse_hotspot_y);
            if (!is_double_buffered_enabled())
                _mouse_cursor.saved_background = (uint8_t *)realloc(
                    _mouse_cursor.saved_background,
                    cursor->bitmap.width * bitmap_save_pixel_size() * cursor->bitmap.height);
        }
        refresh_sprites();
    }
}


void bitmapped_display_controller::set_mouse_cursor(cursor_name cursor_name)
{
    set_mouse_cursor(&CURSORS[(int)cursor_name]);
}


void bitmapped_display_controller::set_mouse_cursor_pos(int x, int y)
{
    _mouse_cursor.move_to(x - _mouse_hotspot_x, y - _mouse_hotspot_y);
    refresh_sprites();
}


void IRAM_ATTR bitmapped_display_controller::exec_primitive(primitive const &prim,
                                                            esp::gfx::rect &update_rect,
                                                            bool inside_isr)
{
    switch (prim.cmd)
    {
        case primitive_cmd::flush:
            break;
        case primitive_cmd::refresh:
            update_rect = update_rect.merge(prim.rect);
            break;
        case primitive_cmd::reset:
            reset_paint_state();
            break;
        case primitive_cmd::set_pen_color:
            paint_state().pen_color = prim.color;
            break;
        case primitive_cmd::set_brush_color:
            paint_state().brush_color = prim.color;
            break;
        case primitive_cmd::set_pixel:
            set_pixel_at((pixel_desc){prim.position, actual_pen_color()}, update_rect);
            break;
        case primitive_cmd::set_pixel_at:
            set_pixel_at(prim.pixel_desc, update_rect);
            break;
        case primitive_cmd::move_to:
            paint_state().position = point(prim.position.x + paint_state().origin.x,
                                           prim.position.y + paint_state().origin.y);
            break;
        case primitive_cmd::line_to:
            line_to(prim.position, update_rect);
            break;
        case primitive_cmd::fill_rect:
            fill_rect(prim.rect, actual_brush_color(), update_rect);
            break;
        case primitive_cmd::draw_rect:
            draw_rect(prim.rect, update_rect);
            break;
        case primitive_cmd::fill_ellipse:
            fill_ellipse(paint_state().position.x, paint_state().position.y, prim.size,
                         actual_brush_color(), update_rect);
            break;
        case primitive_cmd::draw_ellipse:
            draw_ellipse(prim.size, update_rect);
            break;
        case primitive_cmd::clear:
            update_rect =
                update_rect.merge(rect(0, 0, view_port_width() - 1, view_port_height() - 1));
            clear(update_rect);
            break;
        case primitive_cmd::v_scroll:
            update_rect = update_rect.merge(
                rect(paint_state().scrolling_region.x1, paint_state().scrolling_region.y1,
                     paint_state().scrolling_region.x2, paint_state().scrolling_region.y2));
            v_scroll(prim.ivalue, update_rect);
            break;
        case primitive_cmd::h_scroll:
            update_rect = update_rect.merge(
                rect(paint_state().scrolling_region.x1, paint_state().scrolling_region.y1,
                     paint_state().scrolling_region.x2, paint_state().scrolling_region.y2));
            h_scroll(prim.ivalue, update_rect);
            break;
        case primitive_cmd::draw_glyph:
            draw_glyph(prim.glyph, paint_state().glyph_options, paint_state().pen_color,
                       paint_state().brush_color, update_rect);
            break;
        case primitive_cmd::set_glyph_options:
            paint_state().glyph_options = prim.glyph_options;
            break;
        case primitive_cmd::set_paint_options:
            paint_state().paint_options = prim.paint_options;
            break;
        case primitive_cmd::invert_rect:
            invert_rect(prim.rect, update_rect);
            break;
        case primitive_cmd::copy_rect:
            copy_rect(prim.rect, update_rect);
            break;
        case primitive_cmd::set_scrolling_region:
            paint_state().scrolling_region = prim.rect;
            break;
        case primitive_cmd::swap_fgbg:
            swap_fgbg(prim.rect, update_rect);
            break;
        case primitive_cmd::draw_bitmap:
            draw_bitmap(prim.bitmap_drawing_info, update_rect);
            break;
        case primitive_cmd::refresh_sprites:
            hide_sprites(update_rect);
            show_sprites(update_rect);
            break;
        case primitive_cmd::swap_buffers:
            swap_buffers();
            update_rect =
                update_rect.merge(rect(0, 0, view_port_width() - 1, view_port_height() - 1));
            if (inside_isr)
            {
                vTaskNotifyGiveFromISR(prim.notify_task, nullptr);
            }
            else
            {
                xTaskNotifyGive(prim.notify_task);
            }
            break;
        case primitive_cmd::draw_path:
            draw_path(prim.path, update_rect);
            break;
        case primitive_cmd::fill_path:
            fill_path(prim.path, actual_brush_color(), update_rect);
            break;
        case primitive_cmd::set_origin:
            paint_state().origin = prim.position;
            update_absolute_clipping_rect();
            break;
        case primitive_cmd::set_clipping_rect:
            paint_state().clipping_rect = prim.rect;
            update_absolute_clipping_rect();
            break;
        case primitive_cmd::set_pen_width:
            paint_state().pen_width = esp::imax(1, prim.ivalue);
            break;
        case primitive_cmd::set_line_ends:
            paint_state().line_ends = prim.line_ends;
            break;
    }
}


rgb888 IRAM_ATTR bitmapped_display_controller::actual_brush_color()
{
    return paint_state().paint_options.swap_fgbg ? paint_state().pen_color
                                                 : paint_state().brush_color;
}


rgb888 IRAM_ATTR bitmapped_display_controller::actual_pen_color()
{
    return paint_state().paint_options.swap_fgbg ? paint_state().brush_color
                                                 : paint_state().pen_color;
}


void IRAM_ATTR bitmapped_display_controller::line_to(point const &position,
                                                     esp::gfx::rect &update_rect)
{
    rgb888 color = actual_pen_color();

    int orig_x = paint_state().origin.x;
    int orig_y = paint_state().origin.y;
    int x1 = paint_state().position.x;
    int y1 = paint_state().position.y;
    int x2 = position.x + orig_x;
    int y2 = position.y + orig_y;

    int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(rect(esp::imin(x1, x2) - hw, esp::imin(y1, y2) - hw,
                                         esp::imax(x1, x2) + hw, esp::imax(y1, y2) + hw));
    hide_sprites(update_rect);
    abs_draw_line(x1, y1, x2, y2, color);

    paint_state().position = point(x2, y2);
}


void IRAM_ATTR bitmapped_display_controller::update_absolute_clipping_rect()
{
    int x1 = esp::iclamp(paint_state().origin.x + paint_state().clipping_rect.x1, 0,
                         view_port_width() - 1);
    int y1 = esp::iclamp(paint_state().origin.y + paint_state().clipping_rect.y1, 0,
                         view_port_height() - 1);
    int x2 = esp::iclamp(paint_state().origin.x + paint_state().clipping_rect.x2, 0,
                         view_port_width() - 1);
    int y2 = esp::iclamp(paint_state().origin.y + paint_state().clipping_rect.y2, 0,
                         view_port_height() - 1);
    paint_state().abs_clipping_rect = rect(x1, y1, x2, y2);
}


void IRAM_ATTR bitmapped_display_controller::draw_rect(rect const &rect,
                                                       esp::gfx::rect &update_rect)
{
    int x1 = (rect.x1 < rect.x2 ? rect.x1 : rect.x2) + paint_state().origin.x;
    int y1 = (rect.y1 < rect.y2 ? rect.y1 : rect.y2) + paint_state().origin.y;
    int x2 = (rect.x1 < rect.x2 ? rect.x2 : rect.x1) + paint_state().origin.x;
    int y2 = (rect.y1 < rect.y2 ? rect.y2 : rect.y1) + paint_state().origin.y;

    int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(esp::gfx::rect(x1 - hw, y1 - hw, x2 + hw, y2 + hw));
    hide_sprites(update_rect);
    rgb888 color = actual_pen_color();

    abs_draw_line(x1 + 1, y1, x2, y1, color);
    abs_draw_line(x2, y1 + 1, x2, y2, color);
    abs_draw_line(x2 - 1, y2, x1, y2, color);
    abs_draw_line(x1, y2 - 1, x1, y1, color);
}


void IRAM_ATTR bitmapped_display_controller::fill_rect(rect const &rect, rgb888 const &color,
                                                       esp::gfx::rect &update_rect)
{
    int x1 = (rect.x1 < rect.x2 ? rect.x1 : rect.x2) + paint_state().origin.x;
    int y1 = (rect.y1 < rect.y2 ? rect.y1 : rect.y2) + paint_state().origin.y;
    int x2 = (rect.x1 < rect.x2 ? rect.x2 : rect.x1) + paint_state().origin.x;
    int y2 = (rect.y1 < rect.y2 ? rect.y2 : rect.y1) + paint_state().origin.y;

    const int clipX1 = paint_state().abs_clipping_rect.x1;
    const int clipY1 = paint_state().abs_clipping_rect.y1;
    const int clipX2 = paint_state().abs_clipping_rect.x2;
    const int clipY2 = paint_state().abs_clipping_rect.y2;

    if (x1 > clipX2 || x2 < clipX1 || y1 > clipY2 || y2 < clipY1)
        return;

    x1 = esp::iclamp(x1, clipX1, clipX2);
    y1 = esp::iclamp(y1, clipY1, clipY2);
    x2 = esp::iclamp(x2, clipX1, clipX2);
    y2 = esp::iclamp(y2, clipY1, clipY2);

    update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
    hide_sprites(update_rect);

    for (int y = y1; y <= y2; ++y)
        raw_fill_row(y, x1, x2, color);
}


// McIlroy's algorithm
void IRAM_ATTR bitmapped_display_controller::fill_ellipse(int center_x, int center_y,
                                                          size const &size, rgb888 const &color,
                                                          esp::gfx::rect &update_rect)
{
    const int clipX1 = paint_state().abs_clipping_rect.x1;
    const int clipY1 = paint_state().abs_clipping_rect.y1;
    const int clipX2 = paint_state().abs_clipping_rect.x2;
    const int clipY2 = paint_state().abs_clipping_rect.y2;

    const int halfWidth = size.width / 2;
    const int halfHeight = size.height / 2;

    update_rect = update_rect.merge(rect(center_x - halfWidth, center_y - halfHeight,
                                         center_x + halfWidth, center_y + halfHeight));
    hide_sprites(update_rect);

    const int a2 = halfWidth * halfWidth;
    const int b2 = halfHeight * halfHeight;
    const int crit1 = -(a2 / 4 + halfWidth % 2 + b2);
    const int crit2 = -(b2 / 4 + halfHeight % 2 + a2);
    const int crit3 = -(b2 / 4 + halfHeight % 2);
    const int d2xt = 2 * b2;
    const int d2yt = 2 * a2;
    int x = 0; // travels from 0 up to halfWidth
    int y = halfHeight; // travels from halfHeight down to 0
    int width = 1;
    int t = -a2 * y;
    int dxt = 2 * b2 * x;
    int dyt = -2 * a2 * y;

    while (y >= 0 && x <= halfWidth)
    {
        if (t + b2 * x <= crit1 || t + a2 * y <= crit3)
        {
            x++;
            dxt += d2xt;
            t += dxt;
            width += 2;
        }
        else
        {
            int col1 = center_x - x;
            int col2 = center_x - x + width - 1;
            if (col1 <= clipX2 && col2 >= clipX1)
            {
                col1 = esp::iclamp(col1, clipX1, clipX2);
                col2 = esp::iclamp(col2, clipX1, clipX2);
                int row1 = center_y - y;
                int row2 = center_y + y;
                if (row1 >= clipY1 && row1 <= clipY2)
                    raw_fill_row(row1, col1, col2, color);
                if (y != 0 && row2 >= clipY1 && row2 <= clipY2)
                    raw_fill_row(row2, col1, col2, color);
            }
            if (t - a2 * y <= crit2)
            {
                x++;
                dxt += d2xt;
                t += dxt;
                width += 2;
            }
            y--;
            dyt += d2yt;
            t += dyt;
        }
    }
    // one line horizontal ellipse case
    if (halfHeight == 0 && center_y >= clipY1 && center_y <= clipY2)
        raw_fill_row(center_y, esp::iclamp(center_x - halfWidth, clipX1, clipX2),
                     esp::iclamp(center_x - halfWidth + 2 * halfWidth + 1, clipX1, clipX2), color);
}


void IRAM_ATTR bitmapped_display_controller::draw_path(path const &path,
                                                       esp::gfx::rect &update_rect)
{
    rgb888 color = actual_pen_color();

    const int clipX1 = paint_state().abs_clipping_rect.x1;
    const int clipY1 = paint_state().abs_clipping_rect.y1;
    const int clipX2 = paint_state().abs_clipping_rect.x2;
    const int clipY2 = paint_state().abs_clipping_rect.y2;

    int orig_x = paint_state().origin.x;
    int orig_y = paint_state().origin.y;

    int min_x = clipX1;
    int max_x = clipX2 + 1;
    int min_y = INT_MAX;
    int max_y = 0;
    for (int i = 0; i < path.points_count; ++i)
    {
        int py = path.points[i].y + orig_y;
        if (py < min_y)
            min_y = py;
        if (py > max_y)
            max_y = py;
    }
    min_y = esp::tmax(clipY1, min_y);
    max_y = esp::tmin(clipY2, max_y);

    int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(rect(min_x - hw, min_y - hw, max_x + hw, max_y + hw));
    hide_sprites(update_rect);

    int i = 0;
    for (; i < path.points_count - 1; ++i)
    {
        const int x1 = path.points[i].x + orig_x;
        const int y1 = path.points[i].y + orig_y;
        const int x2 = path.points[i + 1].x + orig_x;
        const int y2 = path.points[i + 1].y + orig_y;
        abs_draw_line(x1, y1, x2, y2, color);
    }
    const int x1 = path.points[i].x + orig_x;
    const int y1 = path.points[i].y + orig_y;
    const int x2 = path.points[0].x + orig_x;
    const int y2 = path.points[0].y + orig_y;
    abs_draw_line(x1, y1, x2, y2, color);

    if (path.free_points)
        _prim_dyn_mem_pool.free((void *)path.points);
}


void IRAM_ATTR bitmapped_display_controller::fill_path(path const &path, rgb888 const &color,
                                                       esp::gfx::rect &update_rect)
{
    const int clipX1 = paint_state().abs_clipping_rect.x1;
    const int clipY1 = paint_state().abs_clipping_rect.y1;
    const int clipX2 = paint_state().abs_clipping_rect.x2;
    const int clipY2 = paint_state().abs_clipping_rect.y2;

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    int min_x = clipX1;
    int max_x = clipX2 + 1;
    int min_y = INT_MAX;
    int max_y = 0;
    for (int i = 0; i < path.points_count; ++i)
    {
        int py = path.points[i].y + orig_y;
        if (py < min_y)
            min_y = py;
        if (py > max_y)
            max_y = py;
    }
    min_y = esp::tmax(clipY1, min_y);
    max_y = esp::tmin(clipY2, max_y);

    update_rect = update_rect.merge(rect(min_x, min_y, max_x, max_y));
    hide_sprites(update_rect);

    int16_t node_x[path.points_count];

    for (int pixel_y = min_y; pixel_y <= max_y; ++pixel_y)
    {

        int nodes = 0;
        int j = path.points_count - 1;
        for (int i = 0; i < path.points_count; ++i)
        {
            int piy = path.points[i].y + orig_y;
            int pjy = path.points[j].y + orig_y;
            if ((piy < pixel_y && pjy >= pixel_y) || (pjy < pixel_y && piy >= pixel_y))
            {
                int pjx = path.points[j].x + orig_x;
                int pix = path.points[i].x + orig_x;
                int a = (pixel_y - piy) * (pjx - pix);
                int b = (pjy - piy);
                node_x[nodes++] = pix + a / b + (((a < 0) ^ (b > 0)) && (a % b));
            }
            j = i;
        }

        int i = 0;
        while (i < nodes - 1)
        {
            if (node_x[i] > node_x[i + 1])
            {
                esp::tswap(node_x[i], node_x[i + 1]);
                if (i)
                    --i;
            }
            else
                ++i;
        }

        for (int i = 0; i < nodes; i += 2)
        {
            if (node_x[i] >= max_x)
                break;
            if (node_x[i + 1] > min_x)
            {
                if (node_x[i] < min_x)
                    node_x[i] = min_x;
                if (node_x[i + 1] > max_x)
                    node_x[i + 1] = max_x;
                raw_fill_row(pixel_y, node_x[i], node_x[i + 1] - 1, color);
            }
        }
    }

    if (path.free_points)
        _prim_dyn_mem_pool.free((void *)path.points);
}


void IRAM_ATTR bitmapped_display_controller::abs_draw_thick_line(int x1, int y1, int x2, int y2,
                                                                 int pen_width, rgb888 const &color)
{
    // just to "de-absolutize"
    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;
    x1 -= orig_x;
    y1 -= orig_y;
    x2 -= orig_x;
    y2 -= orig_y;

    point pts[4];

    const double angle = atan2(y2 - y1, x2 - x1);
    const double pw = (double)pen_width / 2.0;
    const int ofs1 = (int)lround(pw * cos(angle + M_PI_2));
    const int ofs2 = (int)lround(pw * sin(angle + M_PI_2));
    const int ofs3 = (int)lround(pw * cos(angle - M_PI_2));
    const int ofs4 = (int)lround(pw * sin(angle - M_PI_2));
    pts[0].x = x1 + ofs1;
    pts[0].y = y1 + ofs2;
    pts[1].x = x1 + ofs3;
    pts[1].y = y1 + ofs4;
    pts[2].x = x2 + ofs3;
    pts[2].y = y2 + ofs4;
    pts[3].x = x2 + ofs1;
    pts[3].y = y2 + ofs2;

    rect update_rect;
    path path = {pts, 4, false};
    fill_path(path, color, update_rect);

    switch (paint_state().line_ends)
    {
        case line_ends::circle:
            if ((pen_width & 1) == 0)
                --pen_width;
            fill_ellipse(x1, y1, size(pen_width, pen_width), color, update_rect);
            fill_ellipse(x2, y2, size(pen_width, pen_width), color, update_rect);
            break;
        default:
            break;
    }
}


void IRAM_ATTR bitmapped_display_controller::draw_bitmap(
    bitmap_drawing_info const &bitmap_drawing_info, esp::gfx::rect &update_rect)
{
    int x = bitmap_drawing_info.x + paint_state().origin.x;
    int y = bitmap_drawing_info.y + paint_state().origin.y;
    update_rect = update_rect.merge(rect(x, y, x + bitmap_drawing_info.bitmap->width - 1,
                                         y + bitmap_drawing_info.bitmap->height - 1));
    hide_sprites(update_rect);
    abs_draw_bitmap(x, y, bitmap_drawing_info.bitmap, nullptr, false);
}


void IRAM_ATTR bitmapped_display_controller::abs_draw_bitmap(int dest_x, int dest_y,
                                                             bitmap const *bitmap,
                                                             void *save_background,
                                                             bool ignore_clipping_rect)
{
    const int clipX1 = ignore_clipping_rect ? 0 : paint_state().abs_clipping_rect.x1;
    const int clipY1 = ignore_clipping_rect ? 0 : paint_state().abs_clipping_rect.y1;
    const int clipX2 =
        ignore_clipping_rect ? view_port_width() - 1 : paint_state().abs_clipping_rect.x2;
    const int clipY2 =
        ignore_clipping_rect ? view_port_height() - 1 : paint_state().abs_clipping_rect.y2;

    if (dest_x > clipX2 || dest_y > clipY2)
        return;

    int width = bitmap->width;
    int height = bitmap->height;

    int x1 = 0;
    int x_count = width;

    if (dest_x < clipX1)
    {
        x1 = clipX1 - dest_x;
        dest_x = clipX1;
    }
    if (x1 >= width)
        return;

    if (dest_x + x_count > clipX2 + 1)
        x_count = clipX2 + 1 - dest_x;
    if (x1 + x_count > width)
        x_count = width - x1;

    int y1 = 0;
    int y_count = height;

    if (dest_y < clipY1)
    {
        y1 = clipY1 - dest_y;
        dest_y = clipY1;
    }
    if (y1 >= height)
        return;

    if (dest_y + y_count > clipY2 + 1)
        y_count = clipY2 + 1 - dest_y;
    if (y1 + y_count > height)
        y_count = height - y1;

    switch (bitmap->format)
    {

        case pixel_format::undefined:
            break;

        case pixel_format::native:
            raw_draw_bitmap_native(dest_x, dest_y, bitmap, x1, y1, x_count, y_count);
            break;

        case pixel_format::mask:
            raw_draw_bitmap_mask(dest_x, dest_y, bitmap, save_background, x1, y1, x_count, y_count);
            break;

        case pixel_format::rgba2222:
            raw_draw_bitmap_rgba2222(dest_x, dest_y, bitmap, save_background, x1, y1, x_count,
                                     y_count);
            break;

        case pixel_format::rgba8888:
            raw_draw_bitmap_rgba8888(dest_x, dest_y, bitmap, save_background, x1, y1, x_count,
                                     y_count);
            break;
    }
}


} // end of namespace
