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

#include "esp_ps2_utils.h"

#include "esp_private/periph_ctrl.h"


#pragma GCC optimize("O2")


namespace esp::ps2
{


////////////////////////////////////////////////////////////////////////////////////////////
// TimeOut


time_out::time_out()
    : _start(esp_timer_get_time())
{
}


bool time_out::expired(int value_ms)
{
    return value_ms > -1 && ((esp_timer_get_time() - _start) / 1000) > value_ms;
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

bool calc_parity(uint8_t v)
{
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}


} // end of namespace
