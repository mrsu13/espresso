// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::vga::VGAPalettedController definition.
 */


#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_gfx_display_controller.h"
#include "esp_tools.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_vga_base_controller.h"


namespace esp::vga
{


/**
* @brief Represents the base class for paletted bitmapped controllers like VGA16Controller, VGA8Controller, etc..
*/
class vga_paletted_controller : public vga_base_controller
{

public:

    vga_paletted_controller(int lines_count, int columns_quantum,
                            esp::gfx::native_pixel_format native_pixel_format,
                            int view_port_ratio_div, int view_port_ratio_mul,
                            intr_handler_t isr_handler);
    ~vga_paletted_controller();

    // unwanted methods
    vga_paletted_controller(vga_paletted_controller const &) = delete;
    void operator=(vga_paletted_controller const &) = delete;

    void end();

    // abstract method of BitmappedDisplayController
    void suspend_background_primitive_execution();

    // import "modeline" version of setResolution
    using vga_base_controller::set_resolution;

    void set_resolution(vga_timings const &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false);

    int palette_size();

    virtual int colors_count()
    {
        return palette_size();
    }

    /**
   * @brief Determines the maximum time allowed to process primitives
   *
   * Primitives processing is always started at the beginning of vertical blank.
   * Unfortunately this time is limited and not all primitive may be processed, so processing all primitives may required more frames.
   * This method expands the allowed time to half of a frame. This increase drawing speed but may show some flickering.
   *
   * The default is False (fast drawings, possible flickering).
   *
   * @param value True = allowed time to process primitives is limited to the vertical blank. Slow, but avoid flickering. False = allowed time is the half of an entire frame. Fast, but may flick.
   */
    void set_process_primitives_on_blank(bool value)
    {
        _process_primitives_on_blank = value;
    }

    // returns "static" version of m_viewPort
    static uint8_t *sget_scanline(int y)
    {
        return (uint8_t *)_isr_view_port[y];
    }

    // abstract method of BitmappedDisplayController
    esp::gfx::native_pixel_format native_pixel_format()
    {
        return _native_pixel_format;
    }

    virtual bool suspend_double_buffering(bool value);


protected:

    void init();

    virtual void setup_default_palette() = 0;

    void update_rgb2_palette_lut();
    void calculate_available_cycles_for_drawings();
    static void primitive_exec_task(void *arg);

    uint8_t rgb888_to_palette_index(esp::gfx::rgb888 const &rgb)
    {
        return _packed_rgb222_to_palette_index[esp::gfx::rgb888_to_packed_rgb222(rgb)];
    }

    uint8_t rgb2222_to_palette_index(uint8_t value)
    {
        return _packed_rgb222_to_palette_index[value & 0b00111111];
    }

    uint8_t rgb8888_to_palette_index(esp::gfx::rgba8888 value)
    {
        return rgb888_to_palette_index(esp::gfx::rgb888(value.r, value.g, value.b));
    }

    // abstract method of BitmappedDisplayController
    void swap_buffers();


    TaskHandle_t _primitive_exec_task;

    volatile uint8_t **_lines;

    // optimization: clones of m_viewPort and m_viewPortVisible
    static volatile uint8_t **_isr_view_port;
    static volatile uint8_t **_isr_view_port_visible;

    static lldesc_t volatile *_frame_reset_desc;

    static volatile int _scan_line;

    esp::gfx::rgb222 *_palette;


private:

    void allocate_view_port();
    void free_view_port();
    void check_view_port_size();
    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row);

    // Maximum time (in CPU cycles) available for primitives drawing
    volatile uint32_t _primitive_exec_timeout_cycles;

    volatile bool _task_processing_primitives;

    // true = allowed time to process primitives is limited to the vertical blank. Slow, but avoid flickering
    // false = allowed time is the half of an entire frame. Fast, but may flick
    bool _process_primitives_on_blank;

    uint8_t _packed_rgb222_to_palette_index[64];

    // configuration
    int _lines_count; // viewport height must be divisible by m_linesCount
    int _columns_quantum; // viewport width must be divisble by m_columnsQuantum
    esp::gfx::native_pixel_format _native_pixel_format;
    int _view_port_ratio_div;
    int _view_port_ratio_mul;
    intr_handler_t _isr_handler;
};


} // end of namespace
