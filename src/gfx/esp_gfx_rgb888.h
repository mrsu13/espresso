// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_color.h"

#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

struct rgb888
{
    inline constexpr rgb888();
    explicit rgb888(esp::gfx::color color);
    inline constexpr rgb888(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

    /** @brief Красный канал, от 0 до 255. */
    std::uint8_t r;

    /** @brief Зелёный канал, от 0 до 255. */
    std::uint8_t g;

    /** @brief Синий канал, от 0 до 255. */
    std::uint8_t b;
} PACKED_ATTR;

inline bool operator==(const rgb888 &lhs, const rgb888 &rhs);
inline bool operator!=(const rgb888 &lhs, const rgb888 &rhs);

}


// Реализация

/**
 * @brief Создаёт чёрный цвет.
 */
constexpr esp::gfx::rgb888::rgb888()
    : r{0}
    , g{0}
    , b{0}
{
}

/**
 * @brief Создаёт цвет по значениям каналов.
 * @param red Красный канал, от 0 до 255.
 * @param green Зелёный канал, от 0 до 255.
 * @param blue Синий канал, от 0 до 255.
 */
constexpr esp::gfx::rgb888::rgb888(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    : r{red}
    , g{green}
    , b{blue}
{
}

/**
 * @brief Сравнивает цвета.
 * @param lhs Первый цвет.
 * @param rhs Второй цвет.
 * @return @c true, если совпадают все три канала.
 */
bool esp::gfx::operator==(const rgb888 &lhs, const rgb888 &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

/**
 * @brief Сравнивает цвета.
 * @param lhs Первый цвет.
 * @param rhs Второй цвет.
 * @return @c true, если различается хотя бы один канал.
 */
bool esp::gfx::operator!=(const rgb888 &lhs, const rgb888 &rhs)
{
    return lhs.r != rhs.r || lhs.g != rhs.g || lhs.b != rhs.b;
}
