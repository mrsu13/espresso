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

#include "esp_vga_config.h"
#include "esp_vga_utils.h"

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace esp::vga
{


#ifndef FABGL_EMULATED


////////////////////////////////////////////////////////////////////////////////////////////
// APLLEnable

void APLLEnable(bool enable, uint32_t sdm0, uint32_t sdm1, uint32_t sdm2, uint32_t o_div)
{
#   if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
    rtc_clk_apll_enable(enable);
    if (enable)
        rtc_clk_apll_coeff_set(o_div, sdm0, sdm1, sdm2);
#   else
    rtc_clk_apll_enable(enable, sdm0, sdm1, sdm2, o_div);
#   endif
}


#endif // #ifndef FABGL_EMULATED


////////////////////////////////////////////////////////////////////////////////////////////
// configureGPIO
void configureGPIO(gpio_num_t gpio, gpio_mode_t mode)
{
#ifndef FABGL_EMULATED
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
    gpio_set_direction(gpio, mode);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////
// getCPUFrequencyMHz
#ifndef FABGL_EMULATED
uint32_t getCPUFrequencyMHz()
{
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return conf.freq_mhz;
}
#endif


///////////////////////////////////////////////////////////////////////////////////
// rgb222_to_hsv
// R, G, B in the 0..3 range
void rgb222_to_hsv(int R, int G, int B, double *h, double *s, double *v)
{
    double r = R / 3.0;
    double g = G / 3.0;
    double b = B / 3.0;
    double cmax = esp::tmax<double>(esp::tmax<double>(r, g), b);
    double cmin = esp::tmin<double>(esp::tmin<double>(r, g), b);
    double diff = cmax - cmin;
    if (cmax == cmin)
        *h = 0;
    else if (cmax == r)
        *h = fmod((60.0 * ((g - b) / diff) + 360.0), 360.0);
    else if (cmax == g)
        *h = fmod((60.0 * ((b - r) / diff) + 120.0), 360.0);
    else if (cmax == b)
        *h = fmod((60.0 * ((r - g) / diff) + 240.0), 360.0);
    *s = cmax == 0 ? 0 : (diff / cmax) * 100.0;
    *v = cmax * 100.0;
}


///////////////////////////////////////////////////////////////////////////////////
// APLLCalcParams


// Must be:
//   maxDen > 1
//   value >= 0
#if FABGLIB_USE_APLL_AB_COEF
void floatToFraction(double value, int maxDen, int *num, int *den)
{
    int64_t a, h[3] = {0, 1, 0}, k[3] = {1, 0, 0};
    int64_t x, d, n = 1;
    while (value != floor(value))
    {
        n <<= 1;
        value *= 2;
    }
    d = value;
    for (int i = 0; i < 64; ++i)
    {
        a = n ? d / n : 0;
        if (i && !a)
            break;
        x = d;
        d = n;
        n = x % n;
        x = a;
        if (k[1] * a + k[0] >= maxDen)
        {
            x = (maxDen - k[0]) / k[1];
            if (x * 2 >= a || k[1] >= maxDen)
                i = 65;
            else
                break;
        }
        h[2] = x * h[1] + h[0];
        h[0] = h[1];
        h[1] = h[2];
        k[2] = x * k[1] + k[0];
        k[0] = k[1];
        k[1] = k[2];
    }
    *den = k[1];
    *num = h[1];
}
#endif


// definitions:
//   apll_clk = XTAL * (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / (2 * o_div + 4)
//     dividend = XTAL * (4 + sdm2 + sdm1 / 256 + sdm0 / 65536)
//     divisor  = (2 * o_div + 4)
//   freq = apll_clk / (2 + b / a)        => assumes  tx_bck_div_num = 1 and clkm_div_num = 2
// Other values range:
//   sdm0  0..255
//   sdm1  0..255
//   sdm2  0..63
//   o_div 0..31
// Assume xtal = FABGLIB_XTAL (40MHz)
// The dividend should be in the range of 350 - 500 MHz (350000000-500000000), so these are the
// actual parameters ranges (so the minimum apll_clk is 5303030 Hz and maximum is 125000000Hz):
//  MIN 87500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 0
//  MAX 125000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 0
//
//  MIN 58333333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 1
//  MAX 83333333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 1
//
//  MIN 43750000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 2
//  MAX 62500000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 2
//
//  MIN 35000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 3
//  MAX 50000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 3
//
//  MIN 29166666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 4
//  MAX 41666666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 4
//
//  MIN 25000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 5
//  MAX 35714285Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 5
//
//  MIN 21875000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 6
//  MAX 31250000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 6
//
//  MIN 19444444Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 7
//  MAX 27777777Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 7
//
//  MIN 17500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 8
//  MAX 25000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 8
//
//  MIN 15909090Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 9
//  MAX 22727272Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 9
//
//  MIN 14583333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 10
//  MAX 20833333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 10
//
//  MIN 13461538Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 11
//  MAX 19230769Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 11
//
//  MIN 12500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 12
//  MAX 17857142Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 12
//
//  MIN 11666666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 13
//  MAX 16666666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 13
//
//  MIN 10937500Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 14
//  MAX 15625000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 14
//
//  MIN 10294117Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 15
//  MAX 14705882Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 15
//
//  MIN 9722222Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 16
//  MAX 13888888Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 16
//
//  MIN 9210526Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 17
//  MAX 13157894Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 17
//
//  MIN 8750000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 18
//  MAX 12500000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 18
//
//  MIN 8333333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 19
//  MAX 11904761Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 19
//
//  MIN 7954545Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 20
//  MAX 11363636Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 20
//
//  MIN 7608695Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 21
//  MAX 10869565Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 21
//
//  MIN 7291666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 22
//  MAX 10416666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 22
//
//  MIN 7000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 23
//  MAX 10000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 23
//
//  MIN 6730769Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 24
//  MAX 9615384Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 24
//
//  MIN 6481481Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 25
//  MAX 9259259Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 25
//
//  MIN 6250000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 26
//  MAX 8928571Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 26
//
//  MIN 6034482Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 27
//  MAX 8620689Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 27
//
//  MIN 5833333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 28
//  MAX 8333333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 28
//
//  MIN 5645161Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 29
//  MAX 8064516Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 29
//
//  MIN 5468750Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 30
//  MAX 7812500Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 30
//
//  MIN 5303030Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 31
//  MAX 7575757Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 31
void APLLCalcParams(double freq, APLLParams *params, uint8_t *a, uint8_t *b, double *out_freq,
                    double *error)
{
    double FXTAL = FABGLIB_XTAL;

    *error = 999999999;

    double apll_freq = freq * 2;

    for (int o_div = 0; o_div <= 31; ++o_div)
    {

        int idivisor = (2 * o_div + 4);

        for (int sdm2 = 4; sdm2 <= 8; ++sdm2)
        {

            // from tables above
            int minSDM1 = (sdm2 == 4 ? 192 : 0);
            int maxSDM1 = (sdm2 == 8 ? 128 : 255);
            // apll_freq = XTAL * (4 + sdm2 + sdm1 / 256) / divisor   ->   sdm1 = (apll_freq * divisor - XTAL * 4 - XTAL * sdm2) * 256 / XTAL
            int startSDM1 = ((apll_freq * idivisor - FXTAL * 4.0 - FXTAL * sdm2) * 256.0 / FXTAL);
#if FABGLIB_USE_APLL_AB_COEF
            for (int isdm1 = esp::tmax(minSDM1, startSDM1); isdm1 <= maxSDM1; ++isdm1)
            {
#else
            int isdm1 = startSDM1;
            {
#endif

                int sdm1 = isdm1;
                sdm1 = esp::tmax(minSDM1, sdm1);
                sdm1 = esp::tmin(maxSDM1, sdm1);

                // apll_freq = XTAL * (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / divisor   ->   sdm0 = (apll_freq * divisor - XTAL * 4 - XTAL * sdm2 - XTAL * sdm1 / 256) * 65536 / XTAL
                int sdm0 =
                    ((apll_freq * idivisor - FXTAL * 4.0 - FXTAL * sdm2 - FXTAL * sdm1 / 256.0)
                     * 65536.0 / FXTAL);
                // from tables above
                sdm0 = (sdm2 == 8 && sdm1 == 128 ? 0 : esp::tmin(255, sdm0));
                sdm0 = esp::tmax(0, sdm0);

                // dividend inside 350-500Mhz?
                double dividend = FXTAL * (4.0 + sdm2 + sdm1 / 256.0 + sdm0 / 65536.0);
                if (dividend >= 350000000 && dividend <= 500000000)
                {
                    // adjust output frequency using "b/a"
                    double oapll_freq = dividend / idivisor;

                    // Calculates "b/a", assuming tx_bck_div_num = 1 and clkm_div_num = 2:
                    //   freq = apll_clk / (2 + clkm_div_b / clkm_div_a)
                    //     abr = clkm_div_b / clkm_div_a
                    //     freq = apll_clk / (2 + abr)    =>    abr = apll_clk / freq - 2
                    uint8_t oa = 1, ob = 0;
#if FABGLIB_USE_APLL_AB_COEF
                    double abr = oapll_freq / freq - 2.0;
                    if (abr > 0 && abr < 1)
                    {
                        int num, den;
                        floatToFraction(abr, 63, &num, &den);
                        ob = esp::tclamp(num, 0, 63);
                        oa = esp::tclamp(den, 0, 63);
                    }
#endif

                    // is this the best?
                    double ofreq = oapll_freq / (2.0 + (double)ob / oa);
                    double err = freq - ofreq;
                    if (abs(err) < abs(*error))
                    {
                        *params = (APLLParams){(uint8_t)sdm0, (uint8_t)sdm1, (uint8_t)sdm2,
                                               (uint8_t)o_div};
                        *a = oa;
                        *b = ob;
                        *out_freq = ofreq;
                        *error = err;
                        if (err == 0.0)
                            return;
                    }
                }
            }
        }
    }
}


} // end of namespace
