// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Соответствие виртуальной клавиши и кода esp::ps2::virtual_key_ascii.
 */


#include "esp_ps2_virtual_key.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Элемент таблицы кодовой страницы: виртуальная клавиша и её код.
 *
 * Таблица кодовой страницы (@c esp::ps2::code_page) переводит в коды
 * знаки, которых нет в ASCII: буквы с диакритикой, валюты и другие.
 */
struct virtual_key_ascii
{
    /** @brief Виртуальная клавиша; @c vk_none завершает таблицу. */
    virtual_key vk;

    /** @brief Код знака в кодовой странице. */
    std::uint8_t ascii;
};

}
