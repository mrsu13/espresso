// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Режим клавиатуры esp::ps2::kbd_mode.
 */


namespace esp::ps2
{

/**
 * @brief Получать ли виртуальные клавиши и ставить ли их в очередь.
 */
enum class kbd_mode
{
    /** @brief Виртуальные клавиши не получаются; приложение читает скан-коды. */
    no_virtual_keys,

    /** @brief Виртуальные клавиши получаются, но не ставятся в очередь: работает только esp::ps2::keyboard::is_vk_down(). */
    generate_virtual_keys,

    /** @brief Виртуальные клавиши получаются и ставятся в очередь: работают и esp::ps2::keyboard::get_next_virtual_key(). */
    create_virtual_keys_queue,
};

}
