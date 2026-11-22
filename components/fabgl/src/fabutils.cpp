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

#include "fabglconf.h"
#include "fabutils.h"

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace fabgl
{


////////////////////////////////////////////////////////////////////////////////////////////
// TimeOut


TimeOut::TimeOut()
    : m_start(esp_timer_get_time())
{
}


bool TimeOut::expired(int valueMS)
{
    return valueMS > -1 && ((esp_timer_get_time() - m_start) / 1000) > valueMS;
}


////////////////////////////////////////////////////////////////////////////////////////////
// isqrt

// Integer square root by Halleck's method, with Legalize's speedup
int isqrt(int x)
{
    if (x < 1)
        return 0;
    int squaredbit = 0x40000000;
    int remainder = x;
    int root = 0;
    while (squaredbit > 0)
    {
        if (remainder >= (squaredbit | root))
        {
            remainder -= (squaredbit | root);
            root >>= 1;
            root |= squaredbit;
        }
        else
            root >>= 1;
        squaredbit >>= 2;
    }
    return root;
}


////////////////////////////////////////////////////////////////////////////////////////////
// calcParity

bool calcParity(uint8_t v)
{
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}


////////////////////////////////////////////////////////////////////////////////////////////
// msToTicks

uint32_t msToTicks(int ms)
{
    return ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(ms);
}


#ifndef FABGL_EMULATED


////////////////////////////////////////////////////////////////////////////////////////////
// I2SModuleEnable

void I2SModuleEnable(int num, bool enable)
{
#   ifdef PERIPH_RCC_ATOMIC
    // PERIPH_I2S1_MODULE is no more available, so set bus clock and reset directly
    uint32_t clk = num == 0 ? DPORT_I2S0_CLK_EN : DPORT_I2S1_CLK_EN;
    uint32_t rst = num == 0 ? DPORT_I2S0_RST : DPORT_I2S1_RST;
    PERIPH_RCC_ATOMIC()
    {
        if (enable)
        {
            DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, clk);
            DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, rst);
        }
        else
        {
            DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, clk);
            DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, rst);
        }
    }
#   else
    auto module = num == 0 ? PERIPH_I2S0_MODULE : PERIPH_I2S1_MODULE;
    if (enable)
        periph_module_enable(module);
    else
        periph_module_disable(module);
#   endif
}


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


////////////////////////////////////////////////////////////////////////////////////////////
// esp_intr_alloc_pinnedToCore

#ifndef FABGL_EMULATED

struct esp_intr_alloc_args
{
    int source;
    int flags;
    intr_handler_t handler;
    void *arg;
    intr_handle_t *ret_handle;
    TaskHandle_t waitingTask;
};


void esp_intr_alloc_pinnedToCore_call(void *arg)
{
    auto args = (esp_intr_alloc_args *)arg;
    esp_intr_alloc(args->source, args->flags, args->handler, args->arg, args->ret_handle);
}


void esp_intr_alloc_pinnedToCore(int source, int flags, intr_handler_t handler, void *arg,
                                 intr_handle_t *ret_handle, int core)
{
    esp_intr_alloc_args args = {source, flags,      handler,
                                arg,    ret_handle, xTaskGetCurrentTaskHandle()};
    esp_ipc_call_blocking(core, esp_intr_alloc_pinnedToCore_call, &args);
}

#endif


////////////////////////////////////////////////////////////////////////////////////////////
// Sutherland-Cohen line clipping algorithm

static int clipLine_code(int x, int y, Rect const &clipRect)
{
    int code = 0;
    if (x < clipRect.X1)
        code = 1;
    else if (x > clipRect.X2)
        code = 2;
    if (y < clipRect.Y1)
        code |= 4;
    else if (y > clipRect.Y2)
        code |= 8;
    return code;
}

// false = line is out of clipping rect
// true = line intersects or is inside the clipping rect (x1, y1, x2, y2 are changed if checkOnly=false)
bool clipLine(int &x1, int &y1, int &x2, int &y2, Rect const &clipRect, bool checkOnly)
{
    int newX1 = x1;
    int newY1 = y1;
    int newX2 = x2;
    int newY2 = y2;
    int topLeftCode = clipLine_code(newX1, newY1, clipRect);
    int bottomRightCode = clipLine_code(newX2, newY2, clipRect);
    while (true)
    {
        if ((topLeftCode == 0) && (bottomRightCode == 0))
        {
            if (!checkOnly)
            {
                x1 = newX1;
                y1 = newY1;
                x2 = newX2;
                y2 = newY2;
            }
            return true;
        }
        else if (topLeftCode & bottomRightCode)
        {
            break;
        }
        else
        {
            int x = 0, y = 0;
            int ncode = topLeftCode != 0 ? topLeftCode : bottomRightCode;
            if (ncode & 8)
            {
                x = newX1 + (newX2 - newX1) * (clipRect.Y2 - newY1) / (newY2 - newY1);
                y = clipRect.Y2;
            }
            else if (ncode & 4)
            {
                x = newX1 + (newX2 - newX1) * (clipRect.Y1 - newY1) / (newY2 - newY1);
                y = clipRect.Y1;
            }
            else if (ncode & 2)
            {
                y = newY1 + (newY2 - newY1) * (clipRect.X2 - newX1) / (newX2 - newX1);
                x = clipRect.X2;
            }
            else if (ncode & 1)
            {
                y = newY1 + (newY2 - newY1) * (clipRect.X1 - newX1) / (newX2 - newX1);
                x = clipRect.X1;
            }
            if (ncode == topLeftCode)
            {
                newX1 = x;
                newY1 = y;
                topLeftCode = clipLine_code(newX1, newY1, clipRect);
            }
            else
            {
                newX2 = x;
                newY2 = y;
                bottomRightCode = clipLine_code(newX2, newY2, clipRect);
            }
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Rect

Rect IRAM_ATTR Rect::merge(Rect const &rect) const
{
    return Rect(imin(rect.X1, X1), imin(rect.Y1, Y1), imax(rect.X2, X2), imax(rect.Y2, Y2));
}


Rect IRAM_ATTR Rect::intersection(Rect const &rect) const
{
    return Rect(tmax(X1, rect.X1), tmax(Y1, rect.Y1), tmin(X2, rect.X2), tmin(Y2, rect.Y2));
}


///////////////////////////////////////////////////////////////////////////////////
// rgb222_to_hsv
// R, G, B in the 0..3 range
void rgb222_to_hsv(int R, int G, int B, double *h, double *s, double *v)
{
    double r = R / 3.0;
    double g = G / 3.0;
    double b = B / 3.0;
    double cmax = tmax<double>(tmax<double>(r, g), b);
    double cmin = tmin<double>(tmin<double>(r, g), b);
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
// LightMemoryPool


void LightMemoryPool::mark(int pos, int16_t size, bool allocated)
{
    m_mem[pos] = size & 0xff;
    m_mem[pos + 1] = ((size >> 8) & 0x7f) | (allocated ? 0x80 : 0);
}


int16_t LightMemoryPool::getSize(int pos)
{
    return m_mem[pos] | ((m_mem[pos + 1] & 0x7f) << 8);
}


bool LightMemoryPool::isFree(int pos)
{
    return (m_mem[pos + 1] & 0x80) == 0;
}


LightMemoryPool::LightMemoryPool(int poolSize)
{
    m_poolSize = poolSize + 2;
    m_mem = (uint8_t *)heap_caps_malloc(m_poolSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    mark(0, m_poolSize - 2, false);
}


LightMemoryPool::~LightMemoryPool()
{
    heap_caps_free(m_mem);
}


void *LightMemoryPool::alloc(int size)
{
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos))
        {
            if (blockSize == size)
            {
                // found a block having the same size
                mark(pos, size, true);
                return m_mem + pos + 2;
            }
            else if (blockSize > size)
            {
                // found a block having larger size
                int remainingSize = blockSize - size - 2;
                if (remainingSize > 0)
                    mark(pos + 2 + size, remainingSize,
                         false); // create new free block at the end of this block
                else
                    size = blockSize; // to avoid to waste last block
                mark(pos, size, true); // reduce size of this block and mark as allocated
                return m_mem + pos + 2;
            }
            else
            {
                // this block hasn't enough space
                // can merge with next block?
                int nextBlockPos = pos + 2 + blockSize;
                if (nextBlockPos < m_poolSize && isFree(nextBlockPos))
                {
                    // join blocks and stay at this pos
                    mark(pos, blockSize + getSize(nextBlockPos) + 2, false);
                }
                else
                {
                    // move to the next block
                    pos += blockSize + 2;
                }
            }
        }
        else
        {
            // move to the next block
            pos += blockSize + 2;
        }
    }
    return nullptr;
}


bool LightMemoryPool::memCheck()
{
    int pos = 0;
    while (pos < m_poolSize)
    {
        int16_t blockSize = getSize(pos);
        pos += blockSize + 2;
    }
    return pos == m_poolSize;
}


int LightMemoryPool::totFree()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos))
            r += blockSize;
        pos += blockSize + 2;
    }
    return r;
}


int LightMemoryPool::totAllocated()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (!isFree(pos))
            r += blockSize;
        pos += blockSize + 2;
    }
    return r;
}


int LightMemoryPool::largestFree()
{
    int r = 0;
    for (int pos = 0; pos < m_poolSize;)
    {
        int16_t blockSize = getSize(pos);
        if (isFree(pos) && blockSize > r)
            r = blockSize;
        pos += blockSize + 2;
    }
    return r;
}


// LightMemoryPool
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// CoreUsage


int CoreUsage::s_busiestCore = FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE;


// CoreUsage
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// CurrentVideoMode


VideoMode CurrentVideoMode::s_videoMode = VideoMode::None;


// CurrentVideoMode
///////////////////////////////////////////////////////////////////////////////////


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
            for (int isdm1 = tmax(minSDM1, startSDM1); isdm1 <= maxSDM1; ++isdm1)
            {
#else
            int isdm1 = startSDM1;
            {
#endif

                int sdm1 = isdm1;
                sdm1 = tmax(minSDM1, sdm1);
                sdm1 = tmin(maxSDM1, sdm1);

                // apll_freq = XTAL * (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / divisor   ->   sdm0 = (apll_freq * divisor - XTAL * 4 - XTAL * sdm2 - XTAL * sdm1 / 256) * 65536 / XTAL
                int sdm0 =
                    ((apll_freq * idivisor - FXTAL * 4.0 - FXTAL * sdm2 - FXTAL * sdm1 / 256.0)
                     * 65536.0 / FXTAL);
                // from tables above
                sdm0 = (sdm2 == 8 && sdm1 == 128 ? 0 : tmin(255, sdm0));
                sdm0 = tmax(0, sdm0);

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
                        ob = tclamp(num, 0, 63);
                        oa = tclamp(den, 0, 63);
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


}
