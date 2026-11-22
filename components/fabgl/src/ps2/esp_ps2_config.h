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


/** Stack size of the task that converts scancodes to Virtual Keys Keyboard */
#define FABGLIB_DEFAULT_SCODETOVK_TASK_STACK_SIZE 1500


/** Priority of the task that converts scancodes to Virtual Keys Keyboard */
#define FABGLIB_SCODETOVK_TASK_PRIORITY 5


/** Optional feature. Enables Keyboard.virtualKeyToString() method */
#define FABGLIB_HAS_VirtualKeyO_STRING 1


/** Size of virtualkey queue */
#define FABGLIB_KEYBOARD_VIRTUALKEY_QUEUE_SIZE 32


/** Size of mouse events queue */
#define FABGLIB_MOUSE_EVENTS_QUEUE_SIZE 64
