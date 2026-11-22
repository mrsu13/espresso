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


namespace esp::sound
{


int calc_i2s_timing_params(int sample_rate, int *out_a, int *out_b, int *out_n, int *out_m);


// routes I2S0 output to the internal DAC and powers up the DAC channel of the specified gpio (GPIO_NUM_25 or GPIO_NUM_26)
void dac_enable_i2s_output(gpio_num_t gpio);


} // end of namespace
