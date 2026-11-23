// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Подключение устройств к портам PS/2 esp::ps2::ps2_preset.
 */


namespace esp::ps2
{

/**
 * @brief Что подключено к портам PS/2 — параметр esp::ps2::ps2_controller::begin().
 */
enum class ps2_preset
{
    /** @brief Клавиатура на порту 0, мышь на порту 1. */
    keyboard_port_0_mouse_port_1,

    /** @brief Клавиатура на порту 1, мышь на порту 0. */
    keyboard_port_1_mouse_port_0,

    /** @brief Только клавиатура, на порту 0. */
    keyboard_port_0,

    /** @brief Только клавиатура, на порту 1. */
    keyboard_port_1,

    /** @brief Только мышь, на порту 0. */
    mouse_port_0,

    /** @brief Только мышь, на порту 1. */
    mouse_port_1,
};

}
