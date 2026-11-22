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

#include "esp_ps2_utils.h"

#if FABGL_ESP_IDF_VERSION >= FABGL_ESP_IDF_VERSION_VAL(5, 0, 0)
#   include "esp_private/periph_ctrl.h"
#else
#   include "driver/periph_ctrl.h"
#endif


#pragma GCC optimize("O2")


namespace esp::ps2
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


} // end of namespace
