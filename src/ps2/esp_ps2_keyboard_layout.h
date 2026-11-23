// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Раскладка клавиатуры esp::ps2::keyboard_layout.
 */


#include "esp_ps2_alt_virtual_key_def.h"
#include "esp_ps2_dead_key_virtual_key_def.h"
#include "esp_ps2_virtual_key.h"
#include "esp_ps2_virtual_key_def.h"


namespace esp::ps2
{

/**
 * @brief Раскладка клавиатуры: перевод скан-кодов и сочетаний клавиш в виртуальные клавиши.
 *
 * Раскладка может наследовать другую (обычно US): то, чего нет в её
 * таблицах, ищется в унаследованной. Таблицы заканчиваются нулевым
 * элементом; размеры массивов — наибольшие среди готовых раскладок.
 * Готовые раскладки объявлены в esp_ps2_keyboard_layouts.h.
 */
struct keyboard_layout
{
    /** @brief Короткое имя раскладки, например «US». */
    const char *name;

    /** @brief Описание раскладки, например «US English». */
    const char *desc;

    /** @brief Унаследованная раскладка или @c nullptr. */
    const keyboard_layout *inherited;

    /** @brief Однобайтовые скан-коды и их виртуальные клавиши. */
    virtual_key_def scancode_to_vk[86];

    /** @brief Расширенные скан-коды (с префиксом 0xE0) и их виртуальные клавиши. */
    virtual_key_def ex_scancode_to_vk[22];

    /** @brief Виртуальные клавиши, которые получаются из других с модификаторами. */
    alt_virtual_key_def alternate_vk[73];

    /** @brief Мёртвые клавиши раскладки. */
    virtual_key dead_keys_vk[8];

    /** @brief Сочетания мёртвых клавиш со следующей клавишей. */
    dead_key_virtual_key_def deadkeys_to_vk[60];
};

}
