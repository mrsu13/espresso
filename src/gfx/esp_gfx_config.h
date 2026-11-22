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


/** Size of display controller primitives queue */
#define FABGLIB_DEFAULT_DISPLAYCONTROLLER_QUEUE_SIZE 1024


/** Size (in bytes) of primitives dynamic buffers. Used by primitives like drawPath and fillPath to contain path points. */
#define FABGLIB_PRIMITIVES_DYNBUFFERS_SIZE 512


/** Defines the underline position starting from character bottom (0 = bottom of the character). */
#define FABGLIB_UNDERLINE_POSITION 0
