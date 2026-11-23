// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Соответствие скан-кода и виртуальной клавиши esp::ps2::virtual_key_def.
 */


#include "esp_ps2_virtual_key.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Элемент раскладки: скан-код клавиши и её виртуальная клавиша без модификаторов.
 */
struct virtual_key_def
{
    /** @brief Скан-код набора 2, принятый от клавиатуры; 0 завершает таблицу. */
    std::uint8_t scancode;

    /** @brief Виртуальная клавиша без SHIFT и других модификаторов. */
    esp::ps2::virtual_key virtual_key;
};

}
