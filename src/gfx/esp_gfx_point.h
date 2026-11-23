// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Точка на экране esp::gfx::point.
 */


#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Точка: пара координат в пикселях.
 *
 * Координаты отсчитываются от левого верхнего угла экрана вправо и вниз.
 * Они могут быть отрицательными или выходить за экран: фигуры при рисовании
 * обрезаются. Структура упакована, потому что входит в примитив рисования
 * @c esp::gfx::primitive, а примитивы копируются через очередь контроллера
 * дисплея.
 */
struct point
{
    inline constexpr point();
    inline constexpr point(int x, int y);

    inline point add(const point &p) const;
    inline point sub(const point &p) const;
    inline point neg() const;

    inline bool operator==(const point &r) const;
    inline bool operator!=(const point &r) const;

    /** @brief Горизонтальная координата, пиксели; растёт вправо. */
    std::int16_t x;

    /** @brief Вертикальная координата, пиксели; растёт вниз. */
    std::int16_t y;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт точку (0, 0) — левый верхний угол.
 */
constexpr esp::gfx::point::point()
    : x{0}
    , y{0}
{
}

/**
 * @brief Создаёт точку с заданными координатами.
 * @param x Горизонтальная координата; сохраняется в 16 разрядах.
 * @param y Вертикальная координата; сохраняется в 16 разрядах.
 */
constexpr esp::gfx::point::point(int x, int y)
    : x{static_cast<std::int16_t>(x)}
    , y{static_cast<std::int16_t>(y)}
{
}

/**
 * @brief Складывает точки как векторы.
 * @param p Слагаемое.
 * @return Точка (x + p.x, y + p.y).
 */
esp::gfx::point esp::gfx::point::add(const point &p) const
{
    return point(x + p.x, y + p.y);
}

/**
 * @brief Вычитает точки как векторы.
 * @param p Вычитаемое.
 * @return Точка (x − p.x, y − p.y).
 */
esp::gfx::point esp::gfx::point::sub(const point &p) const
{
    return point(x - p.x, y - p.y);
}

/**
 * @brief Меняет знак обеих координат.
 * @return Точка (−x, −y).
 */
esp::gfx::point esp::gfx::point::neg() const
{
    return point(-x, -y);
}

/**
 * @brief Сравнивает точки.
 * @param r Вторая точка.
 * @return @c true, если обе координаты совпадают.
 */
bool esp::gfx::point::operator==(const point &r) const
{
    return x == r.x && y == r.y;
}

/**
 * @brief Сравнивает точки.
 * @param r Вторая точка.
 * @return @c true, если различается хотя бы одна координата.
 */
bool esp::gfx::point::operator!=(const point &r) const
{
    return x != r.x || y != r.y;
}
