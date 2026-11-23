// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_current_video_mode.h
 * @brief Объявление текущего видеовыхода esp::current_video_mode.
 */

/**
 * @file esp_current_video_mode.cpp
 * @brief Текущий видеовыход до настройки драйвера.
 */

#include "esp_current_video_mode.h"


/**
 * @class esp::current_video_mode
 * @brief Хранит вид текущего видеовыхода.
 *
 * Драйвер видеовыхода задаёт его при настройке: esp::vga::vga_base_controller
 * задаёт esp::video_mode::vga. Другие модули узнают по нему, какой
 * видеовыход работает: генератор звука со способом вывода
 * esp::sound::sound_gen_method::automatic выбирает для композитного
 * видеовыхода сигма-дельта-модулятор, для остальных — ЦАП.
 *
 * Все методы статические, объект создать нельзя.
 */


esp::video_mode esp::current_video_mode::_video_mode = esp::video_mode::none;
