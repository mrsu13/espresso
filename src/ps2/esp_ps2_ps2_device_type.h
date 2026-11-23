// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Тип устройства PS/2 esp::ps2::ps2_device_type.
 */


namespace esp::ps2
{

/**
 * @brief Тип устройства на порту PS/2 по его ответу на команду опознания.
 */
enum class ps2_device_type
{
    /** @brief Устройство не опознано или не отвечает. */
    unknown_ps2_device,

    /** @brief Старая клавиатура AT: на опознание не отвечает. */
    old_at_keyboard,

    /** @brief Обычная мышь. */
    mouse_standard,

    /** @brief Мышь с колесом. */
    mouse_with_scroll_wheel,

    /** @brief Мышь с пятью кнопками. */
    mouse_5_buttons,

    /** @brief Клавиатура MF2 с трансляцией скан-кодов. */
    mf2_keyboard_with_translation,

    /** @brief Клавиатура MF2 — так отвечает большинство современных клавиатур USB/PS/2. */
    m2_keyboard,
};

}
