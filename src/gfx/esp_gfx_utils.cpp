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

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace esp::gfx
{


////////////////////////////////////////////////////////////////////////////////////////////
// Sutherland-Cohen line clipping algorithm

static int clipLine_code(int x, int y, Rect const &clipRect)
{
    int code = 0;
    if (x < clipRect.X1)
        code = 1;
    else if (x > clipRect.X2)
        code = 2;
    if (y < clipRect.Y1)
        code |= 4;
    else if (y > clipRect.Y2)
        code |= 8;
    return code;
}

// false = line is out of clipping rect
// true = line intersects or is inside the clipping rect (x1, y1, x2, y2 are changed if checkOnly=false)
bool clipLine(int &x1, int &y1, int &x2, int &y2, Rect const &clipRect, bool checkOnly)
{
    int newX1 = x1;
    int newY1 = y1;
    int newX2 = x2;
    int newY2 = y2;
    int topLeftCode = clipLine_code(newX1, newY1, clipRect);
    int bottomRightCode = clipLine_code(newX2, newY2, clipRect);
    while (true)
    {
        if ((topLeftCode == 0) && (bottomRightCode == 0))
        {
            if (!checkOnly)
            {
                x1 = newX1;
                y1 = newY1;
                x2 = newX2;
                y2 = newY2;
            }
            return true;
        }
        else if (topLeftCode & bottomRightCode)
        {
            break;
        }
        else
        {
            int x = 0, y = 0;
            int ncode = topLeftCode != 0 ? topLeftCode : bottomRightCode;
            if (ncode & 8)
            {
                x = newX1 + (newX2 - newX1) * (clipRect.Y2 - newY1) / (newY2 - newY1);
                y = clipRect.Y2;
            }
            else if (ncode & 4)
            {
                x = newX1 + (newX2 - newX1) * (clipRect.Y1 - newY1) / (newY2 - newY1);
                y = clipRect.Y1;
            }
            else if (ncode & 2)
            {
                y = newY1 + (newY2 - newY1) * (clipRect.X2 - newX1) / (newX2 - newX1);
                x = clipRect.X2;
            }
            else if (ncode & 1)
            {
                y = newY1 + (newY2 - newY1) * (clipRect.X1 - newX1) / (newX2 - newX1);
                x = clipRect.X1;
            }
            if (ncode == topLeftCode)
            {
                newX1 = x;
                newY1 = y;
                topLeftCode = clipLine_code(newX1, newY1, clipRect);
            }
            else
            {
                newX2 = x;
                newY2 = y;
                bottomRightCode = clipLine_code(newX2, newY2, clipRect);
            }
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Rect

Rect IRAM_ATTR Rect::merge(Rect const &rect) const
{
    return Rect(esp::imin(rect.X1, X1), esp::imin(rect.Y1, Y1), esp::imax(rect.X2, X2),
                esp::imax(rect.Y2, Y2));
}


Rect IRAM_ATTR Rect::intersection(Rect const &rect) const
{
    return Rect(esp::tmax(X1, rect.X1), esp::tmax(Y1, rect.Y1), esp::tmin(X2, rect.X2),
                esp::tmin(Y2, rect.Y2));
}


///////////////////////////////////////////////////////////////////////////////////
// LightMemoryPool


void LightMemoryPool::mark(int pos, int16_t size, bool allocated)
{
    m_mem[pos] = size & 0xff;
    m_mem[pos + 1] = ((size >> 8) & 0x7f) | (allocated ? 0x80 : 0);
}


int16_t LightMemoryPool::getSize(int pos)
{
    return m_mem[pos] | ((m_mem[pos + 1] & 0x7f) << 8);
}


bool LightMemoryPool::isFree(int pos)
{
    return (m_mem[pos + 1] & 0x80) == 0;
}


LightMemoryPool::LightMemoryPool(int poolSize)
{
    m_poolSize = poolSize + 2;
    m_mem = (uint8_t *)heap_caps_malloc(m_poolSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    mark(0, m_poolSize - 2, false);
}


LightMemoryPool::~LightMemoryPool()
{
    heap_caps_free(m_mem);
}


void *LightMemoryPool::alloc(int size)
{
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos))
        {
            if (blockSize == size)
            {
                // found a block having the same size
                mark(pos, size, true);
                return m_mem + pos + 2;
            }
            else if (blockSize > size)
            {
                // found a block having larger size
                int remainingSize = blockSize - size - 2;
                if (remainingSize > 0)
                    mark(pos + 2 + size, remainingSize,
                         false); // create new free block at the end of this block
                else
                    size = blockSize; // to avoid to waste last block
                mark(pos, size, true); // reduce size of this block and mark as allocated
                return m_mem + pos + 2;
            }
            else
            {
                // this block hasn't enough space
                // can merge with next block?
                int nextBlockPos = pos + 2 + blockSize;
                if (nextBlockPos < m_poolSize && isFree(nextBlockPos))
                {
                    // join blocks and stay at this pos
                    mark(pos, blockSize + getSize(nextBlockPos) + 2, false);
                }
                else
                {
                    // move to the next block
                    pos += blockSize + 2;
                }
            }
        }
        else
        {
            // move to the next block
            pos += blockSize + 2;
        }
    }
    return nullptr;
}


bool LightMemoryPool::memCheck()
{
    int pos = 0;
    while (pos < m_poolSize)
    {
        int16_t blockSize = getSize(pos);
        pos += blockSize + 2;
    }
    return pos == m_poolSize;
}


int LightMemoryPool::totFree()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos))
            r += blockSize;
        pos += blockSize + 2;
    }
    return r;
}


int LightMemoryPool::totAllocated()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (!isFree(pos))
            r += blockSize;
        pos += blockSize + 2;
    }
    return r;
}


int LightMemoryPool::largestFree()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos) && blockSize > r)
            r = blockSize;
        pos += blockSize + 2;
    }
    return r;
}


// LightMemoryPool
///////////////////////////////////////////////////////////////////////////////////


} // end of namespace
