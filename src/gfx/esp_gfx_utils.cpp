// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "driver/rtc_io.h"
#include "esp_ipc.h"
#include "esp_task_wdt.h"
#include "soc/dport_reg.h"
#include "soc/efuse_reg.h"
#include "soc/rtc.h"
#include "soc/rtc_io_struct.h"
#include "soc/sens_struct.h"

#include "esp_gfx_utils.h"

#include "esp_private/periph_ctrl.h"


#pragma GCC optimize("O2")


namespace esp::gfx
{


////////////////////////////////////////////////////////////////////////////////////////////
// Sutherland-Cohen line clipping algorithm

static int clip_line_code(int x, int y, rect const &clip_rect)
{
    int code = 0;
    if (x < clip_rect.x1)
        code = 1;
    else if (x > clip_rect.x2)
        code = 2;
    if (y < clip_rect.y1)
        code |= 4;
    else if (y > clip_rect.y2)
        code |= 8;
    return code;
}

// false = line is out of clipping rect
// true = line intersects or is inside the clipping rect (x1, y1, x2, y2 are changed if checkOnly=false)
bool clip_line(int &x1, int &y1, int &x2, int &y2, rect const &clip_rect, bool check_only)
{
    int new_x1 = x1;
    int new_y1 = y1;
    int new_x2 = x2;
    int new_y2 = y2;
    int top_left_code = clip_line_code(new_x1, new_y1, clip_rect);
    int bottom_right_code = clip_line_code(new_x2, new_y2, clip_rect);
    while (true)
    {
        if ((top_left_code == 0) && (bottom_right_code == 0))
        {
            if (!check_only)
            {
                x1 = new_x1;
                y1 = new_y1;
                x2 = new_x2;
                y2 = new_y2;
            }
            return true;
        }
        else if (top_left_code & bottom_right_code)
        {
            break;
        }
        else
        {
            int x = 0, y = 0;
            int ncode = top_left_code != 0 ? top_left_code : bottom_right_code;
            if (ncode & 8)
            {
                x = new_x1 + (new_x2 - new_x1) * (clip_rect.y2 - new_y1) / (new_y2 - new_y1);
                y = clip_rect.y2;
            }
            else if (ncode & 4)
            {
                x = new_x1 + (new_x2 - new_x1) * (clip_rect.y1 - new_y1) / (new_y2 - new_y1);
                y = clip_rect.y1;
            }
            else if (ncode & 2)
            {
                y = new_y1 + (new_y2 - new_y1) * (clip_rect.x2 - new_x1) / (new_x2 - new_x1);
                x = clip_rect.x2;
            }
            else if (ncode & 1)
            {
                y = new_y1 + (new_y2 - new_y1) * (clip_rect.x1 - new_x1) / (new_x2 - new_x1);
                x = clip_rect.x1;
            }
            if (ncode == top_left_code)
            {
                new_x1 = x;
                new_y1 = y;
                top_left_code = clip_line_code(new_x1, new_y1, clip_rect);
            }
            else
            {
                new_x2 = x;
                new_y2 = y;
                bottom_right_code = clip_line_code(new_x2, new_y2, clip_rect);
            }
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Rect

rect IRAM_ATTR rect::merge(rect const &rect) const
{
    return esp::gfx::rect(esp::imin(rect.x1, x1), esp::imin(rect.y1, y1), esp::imax(rect.x2, x2),
                          esp::imax(rect.y2, y2));
}


rect IRAM_ATTR rect::intersection(rect const &rect) const
{
    return esp::gfx::rect(esp::tmax(x1, rect.x1), esp::tmax(y1, rect.y1), esp::tmin(x2, rect.x2),
                          esp::tmin(y2, rect.y2));
}


///////////////////////////////////////////////////////////////////////////////////
// LightMemoryPool


void light_memory_pool::mark(int pos, int16_t size, bool allocated)
{
    _mem[pos] = size & 0xff;
    _mem[pos + 1] = ((size >> 8) & 0x7f) | (allocated ? 0x80 : 0);
}


int16_t light_memory_pool::get_size(int pos)
{
    return _mem[pos] | ((_mem[pos + 1] & 0x7f) << 8);
}


bool light_memory_pool::is_free(int pos)
{
    return (_mem[pos + 1] & 0x80) == 0;
}


light_memory_pool::light_memory_pool(int pool_size)
{
    _pool_size = pool_size + 2;
    _mem = (uint8_t *)heap_caps_malloc(_pool_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    mark(0, _pool_size - 2, false);
}


light_memory_pool::~light_memory_pool()
{
    heap_caps_free(_mem);
}


void *light_memory_pool::alloc(int size)
{
    for (int pos = 0; pos < _pool_size;)
    {
        int16_t block_size = get_size(pos);
        if (is_free(pos))
        {
            if (block_size == size)
            {
                // found a block having the same size
                mark(pos, size, true);
                return _mem + pos + 2;
            }
            else if (block_size > size)
            {
                // found a block having larger size
                int remaining_size = block_size - size - 2;
                if (remaining_size > 0)
                    mark(pos + 2 + size, remaining_size,
                         false); // create new free block at the end of this block
                else
                    size = block_size; // to avoid to waste last block
                mark(pos, size, true); // reduce size of this block and mark as allocated
                return _mem + pos + 2;
            }
            else
            {
                // this block hasn't enough space
                // can merge with next block?
                int next_block_pos = pos + 2 + block_size;
                if (next_block_pos < _pool_size && is_free(next_block_pos))
                {
                    // join blocks and stay at this pos
                    mark(pos, block_size + get_size(next_block_pos) + 2, false);
                }
                else
                {
                    // move to the next block
                    pos += block_size + 2;
                }
            }
        }
        else
        {
            // move to the next block
            pos += block_size + 2;
        }
    }
    return nullptr;
}


bool light_memory_pool::mem_check()
{
    int pos = 0;
    while (pos < _pool_size)
    {
        int16_t block_size = get_size(pos);
        pos += block_size + 2;
    }
    return pos == _pool_size;
}


int light_memory_pool::tot_free()
{
    int r = 0;
    for (int pos = 0; pos < _pool_size;)
    {
        int16_t block_size = get_size(pos);
        if (is_free(pos))
            r += block_size;
        pos += block_size + 2;
    }
    return r;
}


int light_memory_pool::tot_allocated()
{
    int r = 0;
    for (int pos = 0; pos < _pool_size;)
    {
        int16_t block_size = get_size(pos);
        if (!is_free(pos))
            r += block_size;
        pos += block_size + 2;
    }
    return r;
}


int light_memory_pool::largest_free()
{
    int r = 0;
    for (int pos = 0; pos < _pool_size;)
    {
        int16_t block_size = get_size(pos);
        if (is_free(pos) && block_size > r)
            r = block_size;
        pos += block_size + 2;
    }
    return r;
}


// LightMemoryPool
///////////////////////////////////////////////////////////////////////////////////


} // end of namespace
