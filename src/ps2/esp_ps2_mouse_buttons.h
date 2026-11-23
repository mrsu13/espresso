// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Состояние кнопок мыши esp::ps2::mouse_buttons.
 */


#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Состояние трёх кнопок мыши.
 */
struct mouse_buttons
{
    inline mouse_buttons();

    /** @brief Нажата ли левая кнопка: 1 — нажата. */
    std::uint8_t left : 1;

    /** @brief Нажата ли средняя кнопка (колесо): 1 — нажата. */
    std::uint8_t middle : 1;

    /** @brief Нажата ли правая кнопка: 1 — нажата. */
    std::uint8_t right : 1;
};

}


// Реализация

/**
 * @brief Создаёт состояние с отпущенными кнопками.
 */
esp::ps2::mouse_buttons::mouse_buttons()
    : left{0}
    , middle{0}
    , right{0}
{
}
