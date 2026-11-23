// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Пакет мыши esp::ps2::mouse_packet.
 */


#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Необработанный пакет, принятый от мыши.
 *
 * Три байта у обычной мыши и четыре у мыши с колесом: состояние кнопок и
 * знаки перемещений, перемещение по горизонтали, по вертикали и поворот
 * колеса. Разбирает его esp::ps2::mouse::get_next_delta().
 */
struct mouse_packet
{
    /** @brief Байты пакета. */
    std::uint8_t data[4];
};

}
