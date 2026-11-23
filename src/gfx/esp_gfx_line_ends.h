// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Концы толстых линий esp::gfx::line_ends.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Форма концов линий толщиной больше одного пикселя.
 *
 * Толстая линия рисуется залитым четырёхугольником; концы задают, чем
 * дополнить его на обоих концах. Задаётся esp::gfx::canvas::set_line_ends().
 */
enum class line_ends : std::uint8_t
{
    /** @brief Без дополнения: линия обрывается поперёк. */
    none,

    /** @brief Круглые концы: на каждом конце рисуется круг диаметром в толщину линии. */
    circle,
};

}
