// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief 8-разрядный цвет с прозрачностью esp::gfx::rgba2222.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Цвет с прозрачностью в одном байте: по два разряда на канал.
 *
 * Пиксель растра в формате @c esp::gfx::pixel_format::rgba2222: младшие
 * разряды — красный канал, старшие — альфа-канал (AABBGGRR). Каждый канал —
 * от 0 до 3; пиксель с альфа-каналом 0 прозрачен.
 */
struct rgba2222
{
    inline rgba2222(int red, int green, int blue, int alpha);

    /** @brief Красный канал, от 0 до 3; разряды 0–1 байта. */
    std::uint8_t r : 2;

    /** @brief Зелёный канал, от 0 до 3; разряды 2–3 байта. */
    std::uint8_t g : 2;

    /** @brief Синий канал, от 0 до 3; разряды 4–5 байта. */
    std::uint8_t b : 2;

    /** @brief Альфа-канал, от 0 до 3; разряды 6–7 байта. 0 — пиксель прозрачен. */
    std::uint8_t a : 2;
};

}


// Реализация

/**
 * @brief Создаёт пиксель по значениям каналов.
 * @param red Красный канал, от 0 до 3.
 * @param green Зелёный канал, от 0 до 3.
 * @param blue Синий канал, от 0 до 3.
 * @param alpha Альфа-канал, от 0 до 3.
 */
esp::gfx::rgba2222::rgba2222(int red, int green, int blue, int alpha)
    : r(red)
    , g(green)
    , b(blue)
    , a(alpha)
{
}
