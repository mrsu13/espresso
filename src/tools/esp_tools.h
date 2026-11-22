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


#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <esp_system.h>
#if __has_include("esp_idf_version.h")
#   include "esp_idf_version.h"
#endif


// manage IDF versioning
#ifdef ESP_IDF_VERSION
#   define FABGL_ESP_IDF_VERSION_VAL ESP_IDF_VERSION_VAL
#   define FABGL_ESP_IDF_VERSION ESP_IDF_VERSION
#else
#   define FABGL_ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#   define FABGL_ESP_IDF_VERSION FABGL_ESP_IDF_VERSION_VAL(0, 0, 0)
#endif


#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
// IDF 5 no longer includes many headers implicitly
#   include "driver/gpio.h"
#   include "esp32/rom/ets_sys.h"
#   include "esp32/rom/gpio.h"
#   include "esp32/rom/lldesc.h"
#   include "esp_cpu.h"
#   include "esp_heap_caps.h"
#   include "esp_rom_sys.h"
#   include "esp_timer.h"
#   include "soc/gpio_periph.h"
#   include "soc/io_mux_reg.h"
#else
#   include "rom/lldesc.h"
#endif
#include "esp_rom_gpio.h"
#include "hal/spi_types.h"
#include "soc/frc_timer_reg.h"
#include "soc/gpio_sig_map.h"

// HSPI_HOST and VSPI_HOST are no more defined since IDF 5
#ifndef HSPI_HOST
#   define HSPI_HOST SPI2_HOST
#endif
#ifndef VSPI_HOST
#   define VSPI_HOST SPI3_HOST
#endif

#ifdef ARDUINO
#   include "Arduino.h"
#endif
#include <esp_wifi.h>


/** Core to use for CPU intensive tasks like VGA signals generation in VGATextController or VGAXXController */
#define FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE \
    (WIFI_TASK_CORE_ID ^ 1) // using the same core of WiFi may cause flickering


namespace esp
{


#define GPIO_UNUSED (GPIO_NUM_MAX)
#define GPIO_AUTO ((gpio_num_t)(GPIO_NUM_MAX + 1))


#define TORAD(a) ((a) * M_PI / 180.)


template<typename T>
const T &tmax(const T &a, const T &b)
{
    return (a < b) ? b : a;
}


constexpr auto imax = tmax<int>;


template<typename T>
const T &tmin(const T &a, const T &b)
{
    return !(b < a) ? a : b;
}


constexpr auto imin = tmin<int>;


template<typename T>
const T &tclamp(const T &v, const T &lo, const T &hi)
{
    return (v < lo ? lo : (v > hi ? hi : v));
}


constexpr auto iclamp = tclamp<int>;


template<typename T>
void tswap(T &v1, T &v2)
{
    T t = v1;
    v1 = v2;
    v2 = t;
}


// milliseconds to FreeRTOS ticks.
// ms = -1 => maximum delay (portMAX_DELAY)
uint32_t ms_to_ticks(int ms);


// enables/disables bus clock of the specified I2S module (0 = I2S0, 1 = I2S1)
void i2s_module_enable(int num, bool enable);


void esp_intr_alloc_pinned_to_core(int source, int flags, intr_handler_t handler, void *arg,
                                   intr_handle_t *ret_handle, int core);


///////////////////////////////////////////////////////////////////////////////////
// CoreUsage

/**
 * @brief This class helps to choice a core for intensive processing tasks
 */
struct core_usage
{

    static int busiest_core()
    {
        return _busiest_core;
    }
    static int quiet_core()
    {
        return _busiest_core != -1 ? _busiest_core ^ 1 : -1;
    }
    static void set_busiest_core(int core)
    {
        _busiest_core = core;
    }

private:

    static int
        _busiest_core; // 0 = core 0, 1 = core 1 (default is FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE)
};


///////////////////////////////////////////////////////////////////////////////////
// VideoMode

/** \ingroup Enumerations
 * @brief Specifies a video mode
 */
enum class video_mode
{
    none, /**< Video mode has not been set. */
    vga, /**< VGA display. */
    cvbs, /**< Composite display. */
    i2c, /**< I2C display. */
    spi, /**< SPI display. */
};


/**
 * @brief This class helps to know which is the current video output (VGA or Composite)
 */
struct current_video_mode
{

    static video_mode get()
    {
        return _video_mode;
    }
    static void set(video_mode value)
    {
        _video_mode = value;
    }

private:

    static video_mode _video_mode;
};


} // end of namespace
