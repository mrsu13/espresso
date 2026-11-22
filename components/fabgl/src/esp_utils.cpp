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

#include "esp_utils.h"

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace esp
{


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


#endif // #ifndef FABGL_EMULATED


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


} // end of namespace
