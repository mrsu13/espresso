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
 * @brief This file contains FabGL library configuration settings, like
 *        number of supported colors, maximum usable memory, terminal fonts, etc...
 *
 */


/** Defines ESP32 XTAL frequency in Hz. */
#define FABGLIB_XTAL 40000000


/** Optional feature. Use b/a coeff to fine tune frequency. Unfortunately output is not very stable when enabled! */
#define FABGLIB_USE_APLL_AB_COEF 0


/** To reduce memory overhead the viewport is allocated as few big buffers. This parameter defines the maximum number of these big buffers. */
#define FABGLIB_VIEWPORT_MEMORY_POOL_COUNT 128


/** Size (in bytes) of largest block to maintain free */
#define FABGLIB_MINFREELARGESTBLOCK 40000


/** Stack size of primitives drawing task of paletted based VGA controllers */
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_STACK_SIZE 1200


/** Priority of primitives drawing task of paletted based VGA controllers */
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_PRIORITY 5


// debug options
#define FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK 0
