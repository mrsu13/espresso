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
 * @brief This file contains esp::vga::VGA16Controller definition.
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


#define VGA16_LinesCount 4


namespace esp::vga
{


/**
* @brief Represents the VGA 16 colors bitmapped controller
*
* This VGA controller allows just 16 colors, but requires less (1/2) RAM than VGAController, at the same resolution. Each pixel is represented
* by 4 bits, which is an index to a palette of 16 colors. Each byte of the frame buffer contains two pixels.
* For example, a frame buffer of 640x480 requires about 153K of RAM.
*
* VGA16Controller output is a CPU intensive process and consumes up to 19% of one CPU core (measured at 640x480x60Hz). Anyway this allows to have
* more RAM free for your application.
*
*
* This example initializes VGA Controller with 64 colors (16 visible at the same time) at 640x480:
*
*     esp::vga::VGA16Controller displayController;
*     // the default assigns GPIO22 and GPIO21 to Red, GPIO19 and GPIO18 to Green, GPIO5 and GPIO4 to Blue, GPIO23 to HSync and GPIO15 to VSync
*     displayController.begin();
*     displayController.setResolution(VGA_640x480_60Hz);
*/
class vga16_controller : public vga_paletted_controller
{

public:

    vga16_controller();

    // unwanted methods
    vga16_controller(vga16_controller const &) = delete;
    void operator=(vga16_controller const &) = delete;


    /**
   * @brief Returns the singleton instance of VGA16Controller class
   *
   * @return A pointer to VGA16Controller singleton object
   */
    static vga16_controller *instance()
    {
        return _instance;
    }

    void read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf);

    /**
   * @brief Determines color of specified palette item
   *
   * @param index Palette item (0..15)
   * @param color Color to assign to this item
   *
   * Example:
   *
   *     // Color item 0 is pure Red
   *     displayController.setPaletteItem(0, RGB888(255, 0, 0));
   */
    void set_palette_item(int index, esp::gfx::rgb888 const &color);


protected:

    void setup_default_palette();


private:


    // abstract method of BitmappedDisplayController
    void set_pixel_at(esp::gfx::pixel_desc const &pixel_desc, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void draw_ellipse(esp::gfx::size const &size, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void clear(esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void v_scroll(int scroll, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void h_scroll(int scroll, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void draw_glyph(esp::gfx::glyph const &glyph, esp::gfx::glyph_options glyph_options,
                    esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                    esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void invert_rect(esp::gfx::rect const &rect, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void copy_rect(esp::gfx::rect const &source, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void swap_fgbg(esp::gfx::rect const &rect, esp::gfx::rect &update_rect);

    // abstract method of BitmappedDisplayController
    void raw_draw_bitmap_native(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap, int x1,
                                int y1, int x_count, int y_count);

    // abstract method of BitmappedDisplayController
    void raw_draw_bitmap_mask(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                              void *save_background, int x1, int y1, int x_count, int y_count);

    // abstract method of BitmappedDisplayController
    void raw_draw_bitmap_rgba2222(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                  void *save_background, int x1, int y1, int x_count, int y_count);

    // abstract method of BitmappedDisplayController
    void raw_draw_bitmap_rgba8888(int dest_x, int dest_y, esp::gfx::bitmap const *bitmap,
                                  void *save_background, int x1, int y1, int x_count, int y_count);

    // abstract method of BitmappedDisplayController
    void raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color);
    void raw_fill_row(int y, int x1, int x2, uint8_t color_index);

    void raw_invert_row(int y, int x1, int x2);

    void raw_copy_row(int x1, int x2, int src_y, int dst_y);

    void swap_rows(int y_a, int y_b, int x1, int x2);

    // abstract method of BitmappedDisplayController
    void abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color);

    // abstract method of BitmappedDisplayController
    int bitmap_save_pixel_size()
    {
        return 1;
    }

    static void isr_handler(void *arg);


    static vga16_controller *_instance;

    volatile uint16_t _packed_palette_index_pair_to_signals[256];
};


} // end of namespace
