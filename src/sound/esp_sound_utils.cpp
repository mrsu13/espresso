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


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "driver/rtc_io.h"
#include "esp_ipc.h"
#include "esp_task_wdt.h"
#include "soc/dport_reg.h"
#include "soc/efuse_reg.h"
#include "soc/rtc.h"
#include "soc/rtc_io_struct.h"
#include "soc/sens_struct.h"

#include "esp_sound_utils.h"

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace esp::sound
{


#ifndef FABGL_EMULATED


////////////////////////////////////////////////////////////////////////////////////////////
// DACEnableI2SOutput
// Equivalent to legacy dac_i2s_enable() + dac_output_enable(). The new DAC driver cannot be used
// because it wants to own I2S0.

void DACEnableI2SOutput(gpio_num_t gpio)
{
    // I2S0 clock and data connected to the DAC
    SENS.sar_dac_ctrl1.dac_dig_force = 1;
    SENS.sar_dac_ctrl1.dac_clk_inv = 1;

    // DAC pad as floating RTC pad
    rtc_gpio_init(gpio);
    rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_DISABLED);
    rtc_gpio_pullup_dis(gpio);
    rtc_gpio_pulldown_dis(gpio);

    // power up
    int channel = (gpio == GPIO_NUM_25 ? 0 : 1);
    RTCIO.pad_dac[channel].dac_xpd_force = 1;
    RTCIO.pad_dac[channel].xpd_dac = 1;

    // ADC1 sampling must not drive DAC power
    SENS.sar_meas_ctrl2.sar1_dac_xpd_fsm = 0;
}


#endif // #ifndef FABGL_EMULATED


#ifndef FABGL_EMULATED

// derived from: https://linuxos.sk/blog/mirecove-dristy/detail/esp32-dynamicka-zmena-vzorkovacej-frekvencie-/
// A    : 1..63
// B    : 0..63
// N    : 2..254
// M    : 1..63
// ret: actual sample rate
int calcI2STimingParams(int sampleRate, int *outA, int *outB, int *outN, int *outM)
{
    *outM = 1;

    double N = (double)APB_CLK_FREQ / sampleRate;
    while (N >= 255)
    {
        ++(*outM);
        N = (double)APB_CLK_FREQ / sampleRate / *outM;
    }

    double min_error = 1.0;
    *outN = N;
    *outB = 0;
    *outA = 1;

    for (int a = 1; a < 64; ++a)
    {
        int b = (N - (double)(*outN)) * (double)a;
        if (b > 63)
            continue;

        double divisor = (double)(*outN) + (double)b / (double)a;
        double error = divisor > N ? divisor - N : N - divisor;
        if (error < min_error)
        {
            min_error = error;
            *outA = a;
            *outB = b;
        }

        ++b;
        if (b > 63)
            continue;
        divisor = (double)(*outN) + (double)b / (double)a;
        error = divisor > N ? divisor - N : N - divisor;
        if (error < min_error)
        {
            min_error = error;
            *outA = a;
            *outB = b;
        }
    }

    return APB_CLK_FREQ / ((double)(*outN) + (double)(*outB) / (*outA)) / *outM;
}

#endif


} // end of namespace
