// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Кодовая страница esp::ps2::code_page.
 */


#include "esp_ps2_virtual_key_ascii.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Кодовая страница: номер и таблица перевода знаков за пределами ASCII.
 *
 * Готовые страницы — @c esp::ps2::CODE_PAGE_437 и @c esp::ps2::CODE_PAGE_1252;
 * клавиатура выбирает страницу esp::ps2::keyboard::set_code_page().
 */
struct code_page
{
    /** @brief Номер кодовой страницы, например 437. */
    std::uint16_t codepage;

    /** @brief Таблица перевода; последний элемент — @c vk_none. */
    const virtual_key_ascii *conv_table;
};

}
