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
 * @brief This file contains esp::vga::VGADirectController definition.
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
#include "esp_vga_vga_base_controller.h"


namespace esp::vga
{

/**
 * @brief Callback used when VGADirectController needs to prepare a new scanline to be sent to the VGA output.
 *
 * @param arg Parameter arg passed to VGADirectController::setDrawScanlineCallback()
 * @param dest Buffer to fill with raw pixels
 * @param scanLine Line index (0 to screen height - 1)
 */
typedef void (*draw_scanline_callback)(void *arg, uint8_t *dest, int scan_line);


/**
 * @brief Represents a base abstract class for direct draw VGA controller
 *
 * A direct draw VGA controller draws paint the screen in real time, for each scanline. Every two scanlines an interrupt is generated.
 * Put your drawing code inside a callback and call setDrawScanlineCallback() to assign it.
 *
 * See DirectVGA.ino example.
 */
class vga_direct_controller : public vga_base_controller
{

public:

    /**
   * @brief Initializes a new instance of VGADirectController
   *
   * @param autoRun If True display is active after setResolution() has been called. If False you need to call VGADirectController.run() to start the display.
   */
    vga_direct_controller(bool auto_run = true);

    // unwanted methods
    vga_direct_controller(vga_direct_controller const &) = delete;
    void operator=(vga_direct_controller const &) = delete;


    /**
   * @brief Returns the singleton instance of VGADirectController class
   *
   * @return A pointer to VGADirectController singleton object
   */
    static vga_direct_controller *instance()
    {
        return _instance;
    }

    // abstract method of BitmappedDisplayController
    esp::gfx::native_pixel_format native_pixel_format()
    {
        return esp::gfx::native_pixel_format::sbgr2222;
    }

    // import "modeline" version of setResolution
    using vga_base_controller::set_resolution;

    void set_resolution(vga_timings const &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false);

    void read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf);

    /**
   * @brief Sets the callback used when VGADirectController needs to prepare a new scanline to be sent to the VGA output.
   *
   * @param drawScanlineCallback The callback to call
   * @param arg Argument to pass to the callback
   */
    void set_draw_scanline_callback(draw_scanline_callback draw_scanline_callback,
                                    void *arg = nullptr)
    {
        _draw_scanline_callback = draw_scanline_callback;
        _draw_scanline_arg = arg;
    }

    /**
   * @brief Begins to call the callback function and to display video frames
   *
   * You need to call this only when VGADirectController constructor has been called with autoRun = false.
   */
    void run();

    /**
   * @brief Sets number of scanlines to draw in a single callback
   *
   * Default scanlines per callback is 1.
   *
   * @param value Number of scanlines to fill in DrawScanlineCallback callback
   */
    void set_scanlines_per_call_back(int value)
    {
        _lines_count = value * 2;
    }

    /**
   * @brief Sets a scanline buffer
   *
   * @param scanline Scanline index (0 up to screen height minus one)
   * @param lineBuffer Scanline buffer (must be DMA memory)
   */
    void set_scanline_buffer(int scanline, uint8_t volatile *line_buffer);

    /**
   * @brief Gets current scanline buffer
   *
   * @param scanline Scanline index (0 up to screen height minus one)
   */
    uint8_t volatile *scanline_buffer(int scanline);

    /**
   * @brief Gets default scanline buffer
   *
   * VGADirectController automatically alocates a number of scanline buffers. The number
   * of scanline buffers allocated is determined by setScanlinesPerCallBack() multipled by 2. The default is 2.
   *
   * @param scanline Scanline index (0 up to screen height minus one)
   */
    uint8_t volatile *default_scanline_buffer(int scanline);

    /**
   * @brief Determines if retracing is in progress
   *
   * @return True when retracing (vertical sync) is active
   */
    static bool v_sync()
    {
        return _v_sync;
    }

private:

    void init();

    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row);
    void allocate_view_port();
    void free_view_port();


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


    static vga_direct_controller *_instance;
    static volatile int _scan_line;
    static lldesc_t volatile *_frame_reset_desc;
    static bool _v_sync;
    static lldesc_t volatile **_dma_lines;

    int32_t _lines_count;
    uint8_t **_lines;

    // here we use callbacks in place of virtual methods because vtables are stored in Flash and
    // so it would not have been possible to put ISR into IRAM.
    draw_scanline_callback _draw_scanline_callback;
    void *_draw_scanline_arg;

    bool _auto_run;
};


} // end of namespace
