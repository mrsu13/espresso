// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Состояние мыши esp::ps2::mouse_status.
 */


#include "esp_ps2_mouse_buttons.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Абсолютное положение мыши, поворот колеса и кнопки.
 *
 * Положение считает esp::ps2::mouse::update_absolute_position() из
 * перемещений мыши в пределах области, заданной
 * esp::ps2::mouse::setup_absolute_positioner().
 */
struct mouse_status
{
    inline mouse_status();

    /** @brief Положение по горизонтали, пиксели. */
    std::int16_t x;

    /** @brief Положение по вертикали, пиксели. */
    std::int16_t y;

    /** @brief Поворот колеса с прошлого события; вниз — положительный. */
    std::int8_t wheel_delta;

    /** @brief Кнопки. */
    mouse_buttons buttons;
};

}


// Реализация

/**
 * @brief Создаёт состояние в точке (0, 0) без поворота колеса и с отпущенными кнопками.
 */
esp::ps2::mouse_status::mouse_status()
    : x{0}
    , y{0}
    , wheel_delta{0}
{
}
