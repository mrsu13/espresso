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


#include "esp_utils.h"


namespace esp::vga
{


//////////////////////////////////////////////////////////////////////////////////////////////
// PSRAM_HACK
// ESP32 Revision 1 has following bug: "When the CPU accesses external SRAM through cache, under certain conditions read and write errors occur"
// A workaround is done by the compiler, so whenever PSRAM is enabled the workaround is automatically applied (-mfix-esp32-psram-cache-issue compiler option).
// Unfortunately this workaround reduces performance, even when SRAM is not access, like in VGAXController interrupt handler. This is unacceptable for the interrupt routine.
// In order to confuse the compiler and prevent the workaround from being applied, a "nop" is added between load and store instructions (PSRAM_HACK).

#ifdef ARDUINO
#   ifdef BOARD_HAS_PSRAM
#      define FABGL_NEED_PSRAM_DISABLE_HACK
#   endif
#else
#   ifdef CONFIG_SPIRAM_SUPPORT
#      define FABGL_NEED_PSRAM_DISABLE_HACK
#   endif
#endif

// since IDF 5 the compiler workaround has its own option (and CONFIG_SPIRAM_SUPPORT does not exist anymore)
#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0) \
    && defined(CONFIG_SPIRAM_CACHE_WORKAROUND)
#   define FABGL_NEED_PSRAM_DISABLE_HACK
#endif

#ifdef FABGL_NEED_PSRAM_DISABLE_HACK
#   define PSRAM_HACK asm(" nop")
#else
#   define PSRAM_HACK
#endif


#ifdef FABGL_EMULATED

#   define ASM_MEMW
#   define ASM_NOP
#   define PSRAM_WORKAROUND1
#   define PSRAM_WORKAROUND2

#else

// ESP32 PSRAM bug workaround (use when the library is NOT compiled with PSRAM hack enabled)
// Place between a write and a read PSRAM operation (write->ASM_MEMW->read), not viceversa
#   define ASM_MEMW asm(" MEMW");

#   define ASM_NOP asm(" NOP");

#   define PSRAM_WORKAROUND1 asm(" nop;nop;nop;nop");
#   define PSRAM_WORKAROUND2 asm(" memw");

#endif


void rgb222_to_hsv(int R, int G, int B, double *h, double *s, double *v);


struct APLLParams
{
    uint8_t sdm0;
    uint8_t sdm1;
    uint8_t sdm2;
    uint8_t o_div;
};

void APLLCalcParams(double freq, APLLParams *params, uint8_t *a, uint8_t *b, double *out_freq,
                    double *error);


#ifndef FABGL_EMULATED

inline __attribute__((always_inline)) uint32_t getCycleCount()
{
    uint32_t ccount;
    __asm__ __volatile__("esync \n\t"
                         "rsr %0, ccount \n\t"
                         : "=a"(ccount));
    return ccount;
}

#endif


// enables APLL using specified coefficients, or disables it
void APLLEnable(bool enable, uint32_t sdm0 = 0, uint32_t sdm1 = 0, uint32_t sdm2 = 0,
                uint32_t o_div = 0);


// mode: GPIO_MODE_DISABLE,
//       GPIO_MODE_INPUT,
//       GPIO_MODE_OUTPUT,
//       GPIO_MODE_OUTPUT_OD (open drain),
//       GPIO_MODE_INPUT_OUTPUT_OD (open drain),
//       GPIO_MODE_INPUT_OUTPUT
void configureGPIO(gpio_num_t gpio, gpio_mode_t mode);


uint32_t getCPUFrequencyMHz();


} // end of namespace
