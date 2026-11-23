// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Виртуальная клавиша с модификаторами esp::ps2::alt_virtual_key_def.
 */


#include "esp_ps2_virtual_key.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Элемент раскладки: виртуальная клавиша с модификаторами и что из неё получается.
 *
 * Например, @c vk_1 с SHIFT даёт @c vk_exclaim. Элемент действует, если
 * нажаты все отмеченные модификаторы.
 */
struct alt_virtual_key_def
{
    /** @brief Исходная виртуальная клавиша, полученная из скан-кода; @c vk_none завершает таблицу. */
    esp::ps2::virtual_key req_virtual_key;

    struct
    {
        /** @brief Нужен нажатый CTRL. */
        std::uint8_t ctrl : 1;

        /** @brief Нужен нажатый левый ALT. */
        std::uint8_t lalt : 1;

        /** @brief Нужен нажатый правый ALT (ALTGR). */
        std::uint8_t ralt : 1;

        /** @brief Нужен нажатый SHIFT; CAPS LOCK для букв меняет его значение на обратное. */
        std::uint8_t shift : 1;
    };

    /** @brief Получающаяся виртуальная клавиша. */
    esp::ps2::virtual_key virtual_key;
};

}
