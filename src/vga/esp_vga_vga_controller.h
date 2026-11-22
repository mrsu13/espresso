// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::vga::VGAController definition.
 */


#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "esp_gfx_utils.h"
#include "esp_tools.h"
#include "esp_vga_vga_base_controller.h"


namespace esp::vga
{


/**
* @brief Represents the VGA bitmapped controller
*
* Use this class to set screen resolution and to associate VGA signals to ESP32 GPIO outputs.
*
* This example initializes VGA Controller with 64 colors at 640x350:
*
*     esp::vga::VGAController VGAController;
*     // the default assigns GPIO22 and GPIO21 to Red, GPIO19 and GPIO18 to Green, GPIO5 and GPIO4 to Blue, GPIO23 to HSync and GPIO15 to VSync
*     VGAController.begin();
*     VGAController.setResolution(VGA_640x350_70Hz);
*
* This example initializes VGA Controller with 8 colors (5 GPIOs used) and 640x350 resolution:
*
*     // Assign GPIO22 to Red, GPIO21 to Green, GPIO19 to Blue, GPIO18 to HSync and GPIO5 to VSync
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
*
*     // Set 640x350@70Hz resolution
*     VGAController.setResolution(VGA_640x350_70Hz);
*
* This example initializes VGA Controller with 64 colors (8 GPIOs used) and 640x350 resolution:
*
*     // Assign GPIO22 and GPIO21 to Red, GPIO19 and GPIO18 to Green, GPIO5 and GPIO4 to Blue, GPIO23 to HSync and GPIO15 to VSync
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);
*
*     // Set 640x350@70Hz resolution
*     VGAController.setResolution(VGA_640x350_70Hz);
*/
class vga_controller : public vga_base_controller
{

public:

    vga_controller();

    // unwanted methods
    vga_controller(vga_controller const &) = delete;
    void operator=(vga_controller const &) = delete;


    /**
   * @brief Returns the singleton instance of VGAController class
   *
   * @return A pointer to VGAController singleton object
   */
    static vga_controller *instance()
    {
        return _instance;
    }

    // abstract method of BitmappedDisplayController
    void suspend_background_primitive_execution();

    // abstract method of BitmappedDisplayController
    void resume_background_primitive_execution();

    // abstract method of BitmappedDisplayController
    esp::gfx::native_pixel_format native_pixel_format()
    {
        return esp::gfx::native_pixel_format::sbgr2222;
    }

    // import "modeline" version of setResolution
    using vga_base_controller::set_resolution;

    void set_resolution(vga_timings const &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false);

    /**
   * @brief Reads pixels inside the specified rectangle.
   *
   * Screen reading may occur while other drawings are in progress, so the result may be not updated.
   *
   * @param rect Screen rectangle to read. To improve performance rectangle is not checked.
   * @param destBuf Destination buffer. Buffer size must be at least rect.width() * rect.height.
   *
   * Example:
   *
   *     // paint a red rectangle
   *     Canvas.setBrushColor(Color::BrightRed);
   *     Rect rect = Rect(10, 10, 100, 100);
   *     Canvas.fillRectangle(rect);
   *
   *     // wait for vsync (hence actual drawing)
   *     VGAController.processPrimitives();
   *
   *     // read rectangle pixels into "buf"
   *     auto buf = new RGB222[rect.width() * rect.height()];
   *     VGAController.readScreen(rect, buf);
   *
   *     // write buf 110 pixels to the reight
   *     VGAController.writeScreen(rect.translate(110, 0), buf);
   *     delete buf;
   */
    void read_screen(esp::gfx::rect const &rect, esp::gfx::rgb222 *dest_buf);

    void read_screen(esp::gfx::rect const &rect, esp::gfx::rgb888 *dest_buf);

    /**
   * @brief Writes pixels inside the specified rectangle.
   *
   * Screen writing may occur while other drawings are in progress, so written pixels may be overlapped or mixed.
   *
   * @param rect Screen rectangle to write. To improve performance rectangle is not checked.
   * @param srcBuf Source buffer. Buffer size must be at least rect.width() * rect.height.
   *
   * Example:
   *
   *     // paint a red rectangle
   *     Canvas.setBrushColor(Color::BrightRed);
   *     Rect rect = Rect(10, 10, 100, 100);
   *     Canvas.fillRectangle(rect);
   *
   *     // wait for vsync (hence actual drawing)
   *     VGAController.processPrimitives();
   *
   *     // read rectangle pixels into "buf"
   *     auto buf = new RGB222[rect.width() * rect.height()];
   *     VGAController.readScreen(rect, buf);
   *
   *     // write buf 110 pixels to the reight
   *     VGAController.writeScreen(rect.translate(110, 0), buf);
   *     delete buf;
   */
    void write_screen(esp::gfx::rect const &rect, esp::gfx::rgb222 *src_buf);

    /**
   * @brief Sets a raw pixel prepared using VGAController.createRawPixel.
   *
   * A raw pixel (or raw color) is a byte (uint8_t) that contains color information and synchronization signals.
   *
   * @param x Horizontal pixel position
   * @param y Vertical pixel position
   * @param rgb Raw pixel color
   *
   * Example:
   *
   *     // Set color of pixel at 100, 100
   *     VGAController.setRawPixel(100, 100, VGAController.createRawPixel(RGB222(3, 0, 0));
   */
    void set_raw_pixel(int x, int y, uint8_t rgb)
    {
        VGA_PIXEL(x, y) = rgb;
    }


private:

    void init();

    void allocate_view_port();
    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row);

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

    void raw_fill_row(int y, int x1, int x2, uint8_t pattern);

    void raw_invert_row(int y, int x1, int x2);

    void swap_rows(int y_a, int y_b, int x1, int x2);

    // abstract method of BitmappedDisplayController
    void abs_draw_line(int x1, int y1, int x2, int y2, esp::gfx::rgb888 color);

    // abstract method of BitmappedDisplayController
    int bitmap_save_pixel_size()
    {
        return 1;
    }

    static void v_sync_interrupt(void *arg);


    static vga_controller *_instance;

    volatile int16_t _max_v_sync_isr_time; // Maximum us VSync interrupt routine can run
};


} // end of namespace
