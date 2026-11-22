// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


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

#include "esp_tools.h"

#include "esp_private/periph_ctrl.h"


#pragma GCC optimize("O2")


namespace esp
{


////////////////////////////////////////////////////////////////////////////////////////////
// msToTicks

uint32_t ms_to_ticks(int ms)
{
    return ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(ms);
}


////////////////////////////////////////////////////////////////////////////////////////////
// I2SModuleEnable

void i2s_module_enable(int num, bool enable)
{
#ifdef PERIPH_RCC_ATOMIC
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
#else
    auto module = num == 0 ? PERIPH_I2S0_MODULE : PERIPH_I2S1_MODULE;
    if (enable)
        periph_module_enable(module);
    else
        periph_module_disable(module);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////
// esp_intr_alloc_pinnedToCore


struct esp_intr_alloc_args
{
    int source;
    int flags;
    intr_handler_t handler;
    void *arg;
    intr_handle_t *ret_handle;
    TaskHandle_t waiting_task;
};


void esp_intr_alloc_pinned_to_core_call(void *arg)
{
    auto args = (esp_intr_alloc_args *)arg;
    esp_intr_alloc(args->source, args->flags, args->handler, args->arg, args->ret_handle);
}


void esp_intr_alloc_pinned_to_core(int source, int flags, intr_handler_t handler, void *arg,
                                   intr_handle_t *ret_handle, int core)
{
    esp_intr_alloc_args args = {source, flags,      handler,
                                arg,    ret_handle, xTaskGetCurrentTaskHandle()};
    esp_ipc_call_blocking(core, esp_intr_alloc_pinned_to_core_call, &args);
}


///////////////////////////////////////////////////////////////////////////////////
// CoreUsage


int core_usage::_busiest_core = VIDEO_TASKS_CORE;


// CoreUsage
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// CurrentVideoMode


video_mode current_video_mode::_video_mode = video_mode::none;


// CurrentVideoMode
///////////////////////////////////////////////////////////////////////////////////


} // end of namespace
