// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Перемещение мыши esp::ps2::mouse_delta.
 */


#include "esp_ps2_mouse_buttons.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Перемещение мыши с прошлого пакета и состояние кнопок.
 *
 * Так esp::ps2::mouse::get_next_delta() расшифровывает пакет мыши.
 */
struct mouse_delta
{
    /** @brief Перемещение по горизонтали; вправо — положительное. */
    std::int16_t delta_x;

    /** @brief Перемещение по вертикали; вверх — положительное. */
    std::int16_t delta_y;

    /** @brief Поворот колеса; вниз — положительный. */
    std::int8_t delta_z;

    /** @brief Кнопки. */
    mouse_buttons buttons;

    /** @brief Переполнение счётчика перемещения по горизонтали: мышь двигалась слишком быстро. */
    std::uint8_t overflow_x : 1;

    /** @brief Переполнение счётчика перемещения по вертикали: мышь двигалась слишком быстро. */
    std::uint8_t overflow_y : 1;
};

}
