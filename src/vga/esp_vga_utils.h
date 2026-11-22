// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains some utility classes and functions
 *
 */


#include "esp_tools.h"


namespace esp::vga
{


//////////////////////////////////////////////////////////////////////////////////////////////
// PSRAM_HACK
// ESP32 Revision 1 has following bug: "When the CPU accesses external SRAM through cache, under certain conditions read and write errors occur"
// A workaround is done by the compiler, so whenever PSRAM is enabled the workaround is automatically applied (-mfix-esp32-psram-cache-issue compiler option).
// Unfortunately this workaround reduces performance, even when SRAM is not access, like in VGAXController interrupt handler. This is unacceptable for the interrupt routine.
// In order to confuse the compiler and prevent the workaround from being applied, a "nop" is added between load and store instructions (PSRAM_HACK).

#ifdef CONFIG_SPIRAM_SUPPORT
#   define ESP_VGA_PSRAM_HACK_NEEDED
#endif

// since IDF 5 the compiler workaround has its own option (and CONFIG_SPIRAM_SUPPORT does not exist anymore)
#if defined(CONFIG_SPIRAM_CACHE_WORKAROUND)
#   define ESP_VGA_PSRAM_HACK_NEEDED
#endif

#ifdef ESP_VGA_PSRAM_HACK_NEEDED
#   define PSRAM_HACK asm(" nop")
#else
#   define PSRAM_HACK
#endif


// ESP32 PSRAM bug workaround (use when the library is NOT compiled with PSRAM hack enabled)
// Place between a write and a read PSRAM operation (write->ASM_MEMW->read), not viceversa
#define ASM_MEMW asm(" MEMW");

#define ASM_NOP asm(" NOP");

#define PSRAM_WORKAROUND1 asm(" nop;nop;nop;nop");
#define PSRAM_WORKAROUND2 asm(" memw");


void rgb222_to_hsv(int red, int green, int blue, double *h, double *s, double *v);


struct apll_params
{
    uint8_t sdm0;
    uint8_t sdm1;
    uint8_t sdm2;
    uint8_t o_div;
};

void apll_calc_params(double freq, apll_params *params, uint8_t *a, uint8_t *b, double *out_freq,
                      double *error);


inline __attribute__((always_inline)) uint32_t cycle_count()
{
    uint32_t ccount;
    __asm__ __volatile__("esync \n\t"
                         "rsr %0, ccount \n\t"
                         : "=a"(ccount));
    return ccount;
}


// enables APLL using specified coefficients, or disables it
void apll_enable(bool enable, uint32_t sdm0 = 0, uint32_t sdm1 = 0, uint32_t sdm2 = 0,
                 uint32_t o_div = 0);


// mode: GPIO_MODE_DISABLE,
//       GPIO_MODE_INPUT,
//       GPIO_MODE_OUTPUT,
//       GPIO_MODE_OUTPUT_OD (open drain),
//       GPIO_MODE_INPUT_OUTPUT_OD (open drain),
//       GPIO_MODE_INPUT_OUTPUT
void configure_gpio(gpio_num_t gpio, gpio_mode_t mode);


uint32_t cpu_frequency_mhz();


} // end of namespace
