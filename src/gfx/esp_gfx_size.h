// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Размер на экране esp::gfx::size.
 */


#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Размер: ширина и высота в пикселях.
 *
 * Задаёт, например, размеры эллипса. Структура упакована, потому что входит
 * в примитив рисования @c esp::gfx::primitive.
 */
struct size
{
    inline constexpr size();
    inline constexpr size(int width, int height);

    inline bool operator==(const size &r) const;
    inline bool operator!=(const size &r) const;

    /** @brief Ширина, пиксели. */
    std::int16_t width;

    /** @brief Высота, пиксели. */
    std::int16_t height;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт нулевой размер.
 */
constexpr esp::gfx::size::size()
    : width{0}
    , height{0}
{
}

/**
 * @brief Создаёт размер с заданными шириной и высотой.
 * @param width Ширина; сохраняется в 16 разрядах.
 * @param height Высота; сохраняется в 16 разрядах.
 */
constexpr esp::gfx::size::size(int width, int height)
    : width{static_cast<std::int16_t>(width)}
    , height{static_cast<std::int16_t>(height)}
{
}

/**
 * @brief Сравнивает размеры.
 * @param r Второй размер.
 * @return @c true, если совпадают и ширина, и высота.
 */
bool esp::gfx::size::operator==(const size &r) const
{
    return width == r.width && height == r.height;
}

/**
 * @brief Сравнивает размеры.
 * @param r Второй размер.
 * @return @c true, если различается ширина или высота.
 */
bool esp::gfx::size::operator!=(const size &r) const
{
    return width != r.width || height != r.height;
}
