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


#pragma once


/**
 * @file
 *
 * @brief This file contains some utility classes and functions
 *
 */


#include "esp_tools.h"


namespace esp::gfx
{


/**
 * @brief Represents the coordinate of a point.
 *
 * Coordinates start from 0.
 */
struct point
{
    int16_t x; /**< Horizontal coordinate */
    int16_t y; /**< Vertical coordinate */

    point()
        : x(0)
        , y(0)
    {
    }
    point(int x_, int y_)
        : x(x_)
        , y(y_)
    {
    }

    point add(point const &p) const
    {
        return point(x + p.x, y + p.y);
    }
    point sub(point const &p) const
    {
        return point(x - p.x, y - p.y);
    }
    point neg() const
    {
        return point(-x, -y);
    }
    bool operator==(point const &r)
    {
        return x == r.x && y == r.y;
    }
    bool operator!=(point const &r)
    {
        return x != r.x || y != r.y;
    }
} __attribute__((packed));


/**
 * @brief Represents a bidimensional size.
 */
struct size
{
    int16_t width; /**< Horizontal size */
    int16_t height; /**< Vertical size */

    size()
        : width(0)
        , height(0)
    {
    }
    size(int width_, int height_)
        : width(width_)
        , height(height_)
    {
    }
    bool operator==(size const &r)
    {
        return width == r.width && height == r.height;
    }
    bool operator!=(size const &r)
    {
        return width != r.width || height != r.height;
    }
} __attribute__((packed));


/**
 * @brief Represents a rectangle.
 *
 * Top and Left coordinates start from 0.
 */
struct rect
{
    int16_t x1; /**< Horizontal top-left coordinate */
    int16_t y1; /**< Vertical top-left coordinate */
    int16_t x2; /**< Horizontal bottom-right coordinate */
    int16_t y2; /**< Vertical bottom-right coordinate */

    rect()
        : x1(0)
        , y1(0)
        , x2(0)
        , y2(0)
    {
    }
    rect(int x1_, int y1_, int x2_, int y2_)
        : x1(x1_)
        , y1(y1_)
        , x2(x2_)
        , y2(y2_)
    {
    }
    rect(rect const &r)
    {
        x1 = r.x1;
        y1 = r.y1;
        x2 = r.x2;
        y2 = r.y2;
    }
    rect &operator=(rect const &r) = default;

    bool operator==(rect const &r)
    {
        return x1 == r.x1 && y1 == r.y1 && x2 == r.x2 && y2 == r.y2;
    }
    bool operator!=(rect const &r)
    {
        return x1 != r.x1 || y1 != r.y1 || x2 != r.x2 || y2 != r.y2;
    }
    point pos() const
    {
        return point(x1, y1);
    }
    esp::gfx::size size() const
    {
        return esp::gfx::size(x2 - x1 + 1, y2 - y1 + 1);
    }
    int width() const
    {
        return x2 - x1 + 1;
    }
    int height() const
    {
        return y2 - y1 + 1;
    }
    rect translate(int offset_x, int offset_y) const
    {
        return rect(x1 + offset_x, y1 + offset_y, x2 + offset_x, y2 + offset_y);
    }
    rect translate(point const &offset) const
    {
        return rect(x1 + offset.x, y1 + offset.y, x2 + offset.x, y2 + offset.y);
    }
    rect move(point const &position) const
    {
        return rect(position.x, position.y, position.x + width() - 1, position.y + height() - 1);
    }
    rect move(int x, int y) const
    {
        return rect(x, y, x + width() - 1, y + height() - 1);
    }
    rect shrink(int value) const
    {
        return rect(x1 + value, y1 + value, x2 - value, y2 - value);
    }
    rect h_shrink(int value) const
    {
        return rect(x1 + value, y1, x2 - value, y2);
    }
    rect v_shrink(int value) const
    {
        return rect(x1, y1 + value, x2, y2 - value);
    }
    rect resize(int width, int height) const
    {
        return rect(x1, y1, x1 + width - 1, y1 + height - 1);
    }
    rect resize(esp::gfx::size size) const
    {
        return rect(x1, y1, x1 + size.width - 1, y1 + size.height - 1);
    }
    rect intersection(rect const &rect) const;
    bool intersects(rect const &rect) const
    {
        return x1 <= rect.x2 && x2 >= rect.x1 && y1 <= rect.y2 && y2 >= rect.y1;
    }
    bool contains(rect const &rect) const
    {
        return (rect.x1 >= x1) && (rect.y1 >= y1) && (rect.x2 <= x2) && (rect.y2 <= y2);
    }
    bool contains(point const &point) const
    {
        return point.x >= x1 && point.y >= y1 && point.x <= x2 && point.y <= y2;
    }
    bool contains(int x, int y) const
    {
        return x >= x1 && y >= y1 && x <= x2 && y <= y2;
    }
    rect merge(rect const &rect) const;
} __attribute__((packed));


#define FONTINFOFLAGS_ITALIC 1
#define FONTINFOFLAGS_UNDERLINE 2
#define FONTINFODLAFS_STRIKEOUT 4
#define FONTINFOFLAGS_VARWIDTH 8


struct font_info
{
    uint8_t point_size;
    uint8_t width; // used only for fixed width fonts (FONTINFOFLAGS_VARWIDTH = 0)
    uint8_t height;
    uint8_t ascent;
    uint8_t inleading;
    uint8_t exleading;
    uint8_t flags;
    uint16_t weight;
    uint16_t charset;
    // when FONTINFOFLAGS_VARWIDTH = 0:
    //   data[] contains 256 items each one representing a single character
    // when FONTINFOFLAGS_VARWIDTH = 1:
    //   data[] contains 256 items each one representing a single character. First byte contains the
    //   character width. "chptr" is filled with an array of pointers to the single characters.
    uint8_t const *data;
    uint32_t const *chptr; // used only for variable width fonts (FONTINFOFLAGS_VARWIDTH = 1)
    uint16_t codepage;
};


///////////////////////////////////////////////////////////////////////////////////
// LightMemoryPool
// Each allocated block starts with a two bytes header (int16_t). Bit 15 is allocation flag (0=free, 1=allocated).
// Bits 14..0 represent the block size.
// The maximum size of a block is 32767 bytes.
// free() just marks the block header as free.

class light_memory_pool
{
public:

    light_memory_pool(int pool_size);
    ~light_memory_pool();
    void *alloc(int size);
    void free(void *mem)
    {
        if (mem)
            mark_free((uint8_t *)mem - _mem - 2);
    }

    bool mem_check();
    int tot_free(); // get total free memory
    int tot_allocated(); // get total allocated memory
    int largest_free();

private:

    void mark(int pos, int16_t size, bool allocated);
    void mark_free(int pos)
    {
        _mem[pos + 1] &= 0x7f;
    }
    int16_t get_size(int pos);
    bool is_free(int pos);

    uint8_t *_mem;
    int _pool_size;
};


bool clip_line(int &x1, int &y1, int &x2, int &y2, rect const &clip_rect, bool check_only);


} // end of namespace
