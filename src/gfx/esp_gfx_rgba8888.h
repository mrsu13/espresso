// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief 32-разрядный цвет с прозрачностью esp::gfx::rgba8888.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Цвет с прозрачностью из четырёх 8-разрядных каналов.
 *
 * Пиксель растра в формате @c esp::gfx::pixel_format::rgba8888. Каждый
 * канал — от 0 до 255; альфа-канал 0 — полностью прозрачный пиксель,
 * любое другое значение рисуется как непрозрачное.
 */
struct rgba8888
{
    inline rgba8888();
    inline rgba8888(int red, int green, int blue, int alpha);

    /** @brief Красный канал, от 0 до 255. */
    std::uint8_t r;

    /** @brief Зелёный канал, от 0 до 255. */
    std::uint8_t g;

    /** @brief Синий канал, от 0 до 255. */
    std::uint8_t b;

    /** @brief Альфа-канал: 0 — пиксель прозрачен, иначе он рисуется. */
    std::uint8_t a;
};

}


// Реализация

/**
 * @brief Создаёт прозрачный чёрный пиксель.
 */
esp::gfx::rgba8888::rgba8888()
    : r{0}
    , g{0}
    , b{0}
    , a{0}
{
}

/**
 * @brief Создаёт пиксель по значениям каналов.
 * @param red Красный канал, от 0 до 255.
 * @param green Зелёный канал, от 0 до 255.
 * @param blue Синий канал, от 0 до 255.
 * @param alpha Альфа-канал, от 0 до 255.
 */
esp::gfx::rgba8888::rgba8888(int red, int green, int blue, int alpha)
    : r{static_cast<std::uint8_t>(red)}
    , g{static_cast<std::uint8_t>(green)}
    , b{static_cast<std::uint8_t>(blue)}
    , a{static_cast<std::uint8_t>(alpha)}
{
}
