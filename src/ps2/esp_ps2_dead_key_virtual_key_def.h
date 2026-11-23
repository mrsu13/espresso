// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Сочетание с мёртвой клавишей esp::ps2::dead_key_virtual_key_def.
 */


#include "esp_ps2_virtual_key.h"


namespace esp::ps2
{

/**
 * @brief Элемент раскладки: мёртвая клавиша, следующая за ней клавиша и получающийся знак.
 *
 * Мёртвая клавиша сама знака не даёт, а меняет следующий: например, акут и
 * затем @c vk_e дают @c vk_acute_e.
 */
struct dead_key_virtual_key_def
{
    /** @brief Нажатая перед этим мёртвая клавиша; @c vk_none завершает таблицу. */
    esp::ps2::virtual_key dead_key;

    /** @brief Нажатая за ней клавиша. */
    esp::ps2::virtual_key req_virtual_key;

    /** @brief Получающаяся виртуальная клавиша. */
    esp::ps2::virtual_key virtual_key;
};

}
