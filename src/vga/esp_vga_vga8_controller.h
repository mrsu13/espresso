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
 * @brief This file contains esp::vga::VGA8Controller definition.
 */


#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_gfx_display_controller.h"
#include "esp_gfx_utils.h"
#include "esp_tools.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_vga_paletted_controller.h"


#define VGA8_LinesCount 4


namespace esp::vga
{


/**
* @brief Represents the VGA 8 colors bitmapped controller
*
* This VGA controller allows just 8 colors, but requires less (8/3) RAM than VGAController, at the same resolution. Each pixel is represented
* by 3 bits, which is an index to a palette of 8 colors. Every 3 bytes of the frame buffer there are 8 pixels.
* For example, a frame buffer of 640x480 requires about 113K of RAM.
*
* VGA8Controller output is very CPU intensive process and consumes up to 23% of one CPU core (measured at 640x480x60Hz). Anyway this allows to have
* more RAM free for your application.
*
*
* This example initializes VGA Controller with 64 colors (16 visible at the same time) at 640x480:
*
*     esp::vga::VGA8Controller displayController;
*     // the default assigns GPIO22 and GPIO21 to Red, GPIO19 and GPIO18 to Green, GPIO5 and GPIO4 to Blue, GPIO23 to HSync and GPIO15 to VSync
*     displayController.begin();
*     displayController.setResolution(VGA_640x480_60Hz);
*/
class VGA8Controller : public VGAPalettedController
{

public:

    VGA8Controller();
    ~VGA8Controller();

    // unwanted methods
    VGA8Controller(VGA8Controller const &) = delete;
    void operator=(VGA8Controller const &) = delete;


    /**
   * @brief Returns the singleton instance of VGA8Controller class
   *
   * @return A pointer to VGA8Controller singleton object
   */
    static VGA8Controller *instance()
    {
        return s_instance;
    }

    void readScreen(esp::gfx::Rect const &rect, esp::gfx::RGB888 *destBuf);

    /**
   * @brief Determines color of specified palette item
   *
   * @param index Palette item (0..1)
   * @param color Color to assign to this item
   *
   * Example:
   *
   *     // Color item 0 is pure Red
   *     displayController.setPaletteItem(0, RGB888(255, 0, 0));
   */
    void setPaletteItem(int index, esp::gfx::RGB888 const &color);

    void directSetPixel(int x, int y, int value);


protected:

    void setupDefaultPalette();


private:

    // abstract method of BitmappedDisplayController
    void setPixelAt(esp::gfx::PixelDesc const &pixelDesc, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void drawEllipse(esp::gfx::Size const &size, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void clear(esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void VScroll(int scroll, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void HScroll(int scroll, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void drawGlyph(esp::gfx::Glyph const &glyph, esp::gfx::GlyphOptions glyphOptions,
                   esp::gfx::RGB888 penColor, esp::gfx::RGB888 brushColor,
                   esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void invertRect(esp::gfx::Rect const &rect, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void copyRect(esp::gfx::Rect const &source, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void swapFGBG(esp::gfx::Rect const &rect, esp::gfx::Rect &updateRect);

    // abstract method of BitmappedDisplayController
    void rawDrawBitmap_Native(int destX, int destY, esp::gfx::Bitmap const *bitmap, int X1, int Y1,
                              int XCount, int YCount);

    // abstract method of BitmappedDisplayController
    void rawDrawBitmap_Mask(int destX, int destY, esp::gfx::Bitmap const *bitmap,
                            void *saveBackground, int X1, int Y1, int XCount, int YCount);

    // abstract method of BitmappedDisplayController
    void rawDrawBitmap_RGBA2222(int destX, int destY, esp::gfx::Bitmap const *bitmap,
                                void *saveBackground, int X1, int Y1, int XCount, int YCount);

    // abstract method of BitmappedDisplayController
    void rawDrawBitmap_RGBA8888(int destX, int destY, esp::gfx::Bitmap const *bitmap,
                                void *saveBackground, int X1, int Y1, int XCount, int YCount);

    // abstract method of BitmappedDisplayController
    void rawFillRow(int y, int x1, int x2, esp::gfx::RGB888 color);
    void rawFillRow(int y, int x1, int x2, uint8_t colorIndex);

    void rawInvertRow(int y, int x1, int x2);

    void rawCopyRow(int x1, int x2, int srcY, int dstY);

    void swapRows(int yA, int yB, int x1, int x2);

    // abstract method of BitmappedDisplayController
    void absDrawLine(int X1, int Y1, int X2, int Y2, esp::gfx::RGB888 color);

    // abstract method of BitmappedDisplayController
    int getBitmapSavePixelSize()
    {
        return 1;
    }

    static void ISRHandler(void *arg);


    static VGA8Controller *s_instance;

    volatile uint16_t *m_packedPaletteIndexPair_to_signals;
};


} // end of namespace
