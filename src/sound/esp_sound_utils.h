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


#include "esp_tools.h"


namespace esp::sound
{


#ifndef FABGL_EMULATED
int calcI2STimingParams(int sampleRate, int *outA, int *outB, int *outN, int *outM);
#endif


// routes I2S0 output to the internal DAC and powers up the DAC channel of the specified gpio (GPIO_NUM_25 or GPIO_NUM_26)
void DACEnableI2SOutput(gpio_num_t gpio);


} // end of namespace
