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
 * @brief This file contains esp::vga::VGABaseController definition.
 */


#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_gfx_display_controller.h"
#include "esp_tools.h"
#include "esp_vga_config.h"
#include "esp_vga_gpio_stream.h"


#define VGA_RED_BIT 0
#define VGA_GREEN_BIT 2
#define VGA_BLUE_BIT 4
#define VGA_HSYNC_BIT 6
#define VGA_VSYNC_BIT 7

#define VGA_SYNC_MASK ((1 << VGA_HSYNC_BIT) | (1 << VGA_VSYNC_BIT))


// pixel 0 = byte 2, pixel 1 = byte 3, pixel 2 = byte 0, pixel 3 = byte 1 :
// pixel : 0  1  2  3  4  5  6  7  8  9 10 11 ...etc...
// byte  : 2  3  0  1  6  7  4  5 10 11  8  9 ...etc...
// dword : 0           1           2          ...etc...
// Thanks to https://github.com/paulscottrobson for the new macro. Before was: (row[((X) & 0xFFFC) + ((2 + (X)) & 3)])
#define VGA_PIXELINROW(row, x) (row[(x) ^ 2])

// requires variables: m_viewPort
#define VGA_PIXEL(x, y) VGA_PIXELINROW(_view_port[(y)], x)
#define VGA_INVERT_PIXEL(x, y) \
    { \
        auto px = &VGA_PIXEL((x), (y)); \
        *px = ~(*px ^ VGA_SYNC_MASK); \
    }


namespace esp::vga
{


#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
extern volatile uint64_t s_vgapalctrlcycles;
#endif


/** \ingroup Enumerations
 * @brief Represents one of the four blocks of horizontal or vertical line
 */
enum vga_scan_start
{
    front_porch, /**< Horizontal line sequence is: FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA */
    sync, /**< Horizontal line sequence is: SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH */
    back_porch, /**< Horizontal line sequence is: BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC */
    visible_area /**< Horizontal line sequence is: VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH */
};


/** @brief Specifies the VGA timings. This is a modeline decoded. */
struct vga_timings
{
    char label[22]; /**< Resolution text description */
    int frequency; /**< Pixel frequency (in Hz) */
    int16_t h_visible_area; /**< Horizontal visible area length in pixels */
    int16_t h_front_porch; /**< Horizontal Front Porch duration in pixels */
    int16_t h_sync_pulse; /**< Horizontal Sync Pulse duration in pixels */
    int16_t h_back_porch; /**< Horizontal Back Porch duration in pixels */
    int16_t v_visible_area; /**< Vertical number of visible lines */
    int16_t v_front_porch; /**< Vertical Front Porch duration in lines */
    int16_t v_sync_pulse; /**< Vertical Sync Pulse duration in lines */
    int16_t v_back_porch; /**< Vertical Back Porch duration in lines */
    char h_sync_logic; /**< Horizontal Sync polarity '+' or '-' */
    char v_sync_logic; /**< Vertical Sync polarity '+' or '-' */
    uint8_t
        scan_count; /**< Scan count. 1 = single scan, 2 = double scan (allowing low resolutions like 320x240...) */
    uint8_t
        multi_scan_black; /**< 0 = Additional rows are the repetition of the first. 1 = Additional rows are blank. */
    vga_scan_start
        h_starting_block; /**< Horizontal starting block. DetermineshHorizontal order of signals */
};


class vga_base_controller : public esp::gfx::generic_bitmapped_display_controller
{

public:

    vga_base_controller();

    // unwanted methods
    vga_base_controller(vga_base_controller const &) = delete;
    void operator=(vga_base_controller const &) = delete;

    /**
   * @brief This is the 8 colors (5 GPIOs) initializer.
   *
   * One GPIO per channel, plus horizontal and vertical sync signals.
   *
   * @param redGPIO GPIO to use for red channel.
   * @param greenGPIO GPIO to use for green channel.
   * @param blueGPIO GPIO to use for blue channel.
   * @param HSyncGPIO GPIO to use for horizontal sync signal.
   * @param VSyncGPIO GPIO to use for vertical sync signal.
   *
   * Example:
   *
   *     // Use GPIO 22 for red, GPIO 19 for green, GPIO 5 for blue, GPIO 23 for HSync and GPIO 15 for VSync
   *     VGAController.begin(GPIO_NUM_22, GPIO_NUM_19, GPIO_NUM_5, GPIO_NUM_23, GPIO_NUM_15);
   */
    void begin(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
               gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio);

    /**
   * @brief This is the 64 colors (8 GPIOs) initializer.
   *
   * Two GPIOs per channel, plus horizontal and vertical sync signals.
   *
   * @param red1GPIO GPIO to use for red channel, MSB bit.
   * @param red0GPIO GPIO to use for red channel, LSB bit.
   * @param green1GPIO GPIO to use for green channel, MSB bit.
   * @param green0GPIO GPIO to use for green channel, LSB bit.
   * @param blue1GPIO GPIO to use for blue channel, MSB bit.
   * @param blue0GPIO GPIO to use for blue channel, LSB bit.
   * @param HSyncGPIO GPIO to use for horizontal sync signal.
   * @param VSyncGPIO GPIO to use for vertical sync signal.
   *
   * Example:
   *
   *     // Use GPIO 22-21 for red, GPIO 19-18 for green, GPIO 5-4 for blue, GPIO 23 for HSync and GPIO 15 for VSync
   *     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);
   */
    void begin(gpio_num_t red_1gpio, gpio_num_t red_0gpio, gpio_num_t green_1gpio,
               gpio_num_t green_0gpio, gpio_num_t blue_1gpio, gpio_num_t blue_0gpio,
               gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio);

    /**
   * @brief This is the 64 colors (8 GPIOs) initializer using default pinout.
   *
   * Two GPIOs per channel, plus horizontal and vertical sync signals.
   * Use GPIO 22-21 for red, GPIO 19-18 for green, GPIO 5-4 for blue, GPIO 23 for HSync and GPIO 15 for VSync
   *
   * Example:
   *
   *     VGAController.begin();
   */
    void begin();

    virtual void end();

    static bool convert_modeline_to_timings(char const *modeline, vga_timings *timings);

    // abstract method of BitmappedDisplayController
    virtual void suspend_background_primitive_execution();

    // abstract method of BitmappedDisplayController
    virtual void resume_background_primitive_execution();

    /**
   * @brief Sets current resolution using linux-like modeline.
   *
   * Modeline must have following syntax (non case sensitive):
   *
   *     "label" clock_mhz hdisp hsyncstart hsyncend htotal vdisp vsyncstart vsyncend vtotal (+HSync | -HSync) (+VSync | -VSync) [DoubleScan | QuadScan] [FrontPorchBegins | SyncBegins | BackPorchBegins | VisibleBegins] [MultiScanBlank]
   *
   * In esp_vga_modelines.h there are macros with some predefined modelines for common resolutions.
   * When MultiScanBlank and DoubleScan is specified then additional rows are not repeated, but just filled with blank lines.
   *
   * @param modeline Linux-like modeline as specified above.
   * @param viewPortWidth Horizontal viewport size in pixels. If less than zero (-1) it is sized to modeline visible area width.
   * @param viewPortHeight Vertical viewport size in pixels. If less then zero (-1) it is sized to maximum allocable.
   * @param doubleBuffered if True allocates another viewport of the same size to use as back buffer. Make sure there is enough free memory.
   *
   * Example:
   *
   *     // Use predefined modeline for 640x480@60Hz
   *     VGAController.setResolution(VGA_640x480_60Hz);
   *
   *     // The same of above using modeline string
   *     VGAController.setResolution("\"640x480@60Hz\" 25.175 640 656 752 800 480 490 492 525 -HSync -VSync");
   *
   *     // Set 640x382@60Hz but limit the viewport to 640x350
   *     VGAController.setResolution(VGA_640x382_60Hz, 640, 350);
   *
   */
    void set_resolution(char const *modeline, int view_port_width = -1, int view_port_height = -1,
                        bool double_buffered = false);

    virtual void set_resolution(vga_timings const &timings, int view_port_width = -1,
                                int view_port_height = -1, bool double_buffered = false);

    /**
   * @brief Determines horizontal position of the viewport.
   *
   * @return Horizontal position of the viewport (in case viewport width is less than screen width).
   */
    int view_port_col()
    {
        return _view_port_col;
    }

    /**
   * @brief Determines vertical position of the viewport.
   *
   * @return Vertical position of the viewport (in case viewport height is less than screen height).
   */
    int view_port_row()
    {
        return _view_port_row;
    }

    /**
   * @brief Moves screen by specified horizontal and vertical offset.
   *
   * Screen moving is performed moving horizontal and vertical Front and Back porchs.
   *
   * @param offsetX Horizontal offset in pixels. < 0 goes left, > 0 goes right.
   * @param offsetY Vertical offset in pixels. < 0 goes up, > 0 goes down.
   *
   * Example:
   *
   *     // Move screen 4 pixels right, 1 pixel left
   *     VGAController.moveScreen(4, -1);
   */
    void move_screen(int offset_x, int offset_y);

    /**
   * @brief Reduces or expands screen size by the specified horizontal and vertical offset.
   *
   * Screen shrinking is performed changing horizontal and vertical Front and Back porchs.
   *
   * @param shrinkX Horizontal offset in pixels. > 0 shrinks, < 0 expands.
   * @param shrinkY Vertical offset in pixels. > 0 shrinks, < 0 expands.
   *
   * Example:
   *
   *     // Shrink screen by 8 pixels and move by 8 pixels to the left
   *     VGAController.shrinkScreen(8, 0);
   *     VGAController.moveScreen(8, 0);
   */
    void shrink_screen(int shrink_x, int shrink_y);

    vga_timings *resolution_timings()
    {
        return &_timings;
    }

    /**
   * @brief Gets number of bits allocated for each channel.
   *
   * Number of bits depends by which begin() initializer has been called.
   *
   * @return 1 (8 colors) or 2 (64 colors).
   */
    uint8_t bits_per_channel()
    {
        return _bits_per_channel;
    }

    virtual int colors_count()
    {
        return 1 << (3 * _bits_per_channel);
    }

    /**
   * @brief Gets a raw scanline pointer.
   *
   * A raw scanline must be filled with raw pixel colors. Use VGAController.createRawPixel to create raw pixel colors.
   * A raw pixel (or raw color) is a byte (uint8_t) that contains color information and synchronization signals.
   * Pixels are arranged in 32 bit packes as follows:
   *   pixel 0 = byte 2, pixel 1 = byte 3, pixel 2 = byte 0, pixel 3 = byte 1 :
   *   pixel : 0  1  2  3  4  5  6  7  8  9 10 11 ...etc...
   *   byte  : 2  3  0  1  6  7  4  5 10 11  8  9 ...etc...
   *   dword : 0           1           2          ...etc...
   *
   * @param y Vertical scanline position (0 = top row)
   */
    uint8_t *scanline(int y)
    {
        return (uint8_t *)_view_port[y];
    }

    /**
   * @brief Creates a raw pixel to use with VGAController.setRawPixel
   *
   * A raw pixel (or raw color) is a byte (uint8_t) that contains color information and synchronization signals.
   *
   * @param rgb Pixel RGB222 color
   *
   * Example:
   *
   *     // Set color of pixel at 100, 100
   *     VGAController.setRawPixel(100, 100, VGAController.createRawPixel(RGB222(3, 0, 0));
   */
    uint8_t create_raw_pixel(esp::gfx::rgb222 rgb)
    {
        return prepare_pixel(rgb);
    }

    uint8_t create_blank_raw_pixel()
    {
        return _hv_sync;
    }

    virtual bool suspend_double_buffering(bool value);


protected:

    static void setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode);

    void start_gpio_stream();

    void free_buffers();

    virtual void free_view_port();

    virtual void init();

    bool set_dma_buffers_count(int buffers_count);

    uint8_t pack_hv_sync(bool h_sync, bool v_sync);

    uint8_t inline __attribute__((always_inline)) prepare_pixel(esp::gfx::rgb222 rgb)
    {
        return _hv_sync | (rgb.b << VGA_BLUE_BIT) | (rgb.g << VGA_GREEN_BIT)
            | (rgb.r << VGA_RED_BIT);
    }

    uint8_t prepare_pixel_with_sync(esp::gfx::rgb222 rgb, bool h_sync, bool v_sync);

    void fill_vert_buffers(int offset_y);

    void fill_horiz_buffers(int offset_x);

    int fill(uint8_t volatile *buffer, int start_pos, int length, uint8_t red, uint8_t green,
             uint8_t blue, bool hsync, bool vsync);

    int calc_required_dma_buffers_count(int view_port_height);

    bool is_multi_scan_black_line(int scan);

    void set_dma_buffer_blank(int index, void volatile *address, int length, int scan,
                              bool is_start_of_vert_front_porch);
    void set_dma_buffer_view(int index, int row, int scan, volatile uint8_t **view_port,
                             bool on_visible_dma);
    void set_dma_buffer_view(int index, int row, int scan, bool is_start_of_vert_front_porch);

    virtual void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch,
                                     int scan, bool is_visible, int visible_row) = 0;

    void volatile *get_dma_buffer(int index, int *length);

    void allocate_view_port(uint32_t alloc_caps, int rowlen);
    virtual void allocate_view_port() = 0;
    virtual void check_view_port_size() {};

    // abstract method of BitmappedDisplayController
    virtual void swap_buffers();


    // when double buffer is enabled the "drawing" view port is always m_viewPort, while the "visible" view port is always m_viewPortVisible
    // when double buffer is not enabled then m_viewPort = m_viewPortVisible
    volatile uint8_t **_view_port;
    volatile uint8_t **_view_port_visible;

    // screen buffers (m_viewPort and m_viewPortVisible contain these values)
    volatile uint8_t **_view_ports[2];

    // true: double buffering is implemented in DMA
    bool _double_buffer_over_dma;

    volatile int _primitive_processing_suspended; // 0 = enabled, >0 suspended

    intr_handle_t _isr_handle;

    vga_timings _timings;
    int16_t _h_line_size;

    volatile int16_t _view_port_col;
    volatile int16_t _view_port_row;

    // contains H and V signals for visible line
    volatile uint8_t _hv_sync;


private:


    // bits per channel on VGA output
    // 1 = 8 colors, 2 = 64 colors, set by begin()
    int _bits_per_channel;

    gpio_stream _gpio_stream;

    lldesc_t volatile *_dma_buffers;
    int _dma_buffers_count;

    // when double buffer is enabled at DMA level the running DMA buffer is always m_DMABuffersVisible
    // when double buffer is not enabled then m_DMABuffers = m_DMABuffersVisible
    lldesc_t volatile *_dma_buffers_head;
    lldesc_t volatile *_dma_buffers_visible;

    // These buffers contains a full line, with FrontPorch, Sync, BackPorch and blank visible area, in the
    // order specified by timings.HStartingBlock
    volatile uint8_t *_h_blank_line_with_v_sync;
    volatile uint8_t *_h_blank_line;

    uint8_t **_view_port_memory_pool; // array ends with nullptr

    int16_t _raw_frame_height;
};


} // end of namespace
