// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_rgb888.h"

#include <cstdint>


namespace esp::gfx
{

struct rgb222
{
    inline rgb222();
    inline rgb222(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
    rgb222(const rgb888 &value);

    /**
     * @brief Сводить ли цвета к одному разряду на канал (8 цветов).
     *
     * @c true — режим 8 цветов: любой ненулевой канал становится 3 (полная
     * яркость). @c false — режим 64 цветов: канал берёт два старших
     * разряда. Устанавливает контроллер VGA по числу выводов цвета.
     */
    static bool low_bit_only;

    /** @brief Красный канал, от 0 до 3 (в режиме 8 цветов — 0 или 3). */
    std::uint8_t r : 2;

    /** @brief Зелёный канал, от 0 до 3 (в режиме 8 цветов — 0 или 3). */
    std::uint8_t g : 2;

    /** @brief Синий канал, от 0 до 3 (в режиме 8 цветов — 0 или 3). */
    std::uint8_t b : 2;
};

inline bool operator==(const rgb222 &lhs, const rgb222 &rhs);
inline bool operator!=(const rgb222 &lhs, const rgb222 &rhs);

std::uint8_t rgb888_to_packed_rgb222(const rgb888 &rgb);

}


// Реализация

/**
 * @brief Создаёт чёрный цвет.
 */
esp::gfx::rgb222::rgb222()
    : r(0)
    , g(0)
    , b(0)
{
}

/**
 * @brief Создаёт цвет по значениям каналов.
 * @param red Красный канал, от 0 до 3.
 * @param green Зелёный канал, от 0 до 3.
 * @param blue Синий канал, от 0 до 3.
 */
esp::gfx::rgb222::rgb222(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    : r(red)
    , g(green)
    , b(blue)
{
}

/**
 * @brief Сравнивает цвета.
 * @param lhs Первый цвет.
 * @param rhs Второй цвет.
 * @return @c true, если совпадают все три канала.
 */
bool esp::gfx::operator==(const rgb222 &lhs, const rgb222 &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

/**
 * @brief Сравнивает цвета.
 * @param lhs Первый цвет.
 * @param rhs Второй цвет.
 * @return @c true, если различается хотя бы один канал.
 */
bool esp::gfx::operator!=(const rgb222 &lhs, const rgb222 &rhs)
{
    return lhs.r != rhs.r || lhs.g != rhs.g || lhs.b != rhs.b;
}
