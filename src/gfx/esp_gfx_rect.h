// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_point.h"
#include "esp_gfx_size.h"

#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

struct rect
{
    inline constexpr rect();
    inline constexpr rect(int x1, int y1, int x2, int y2);
    inline rect(const rect &r);
    rect &operator=(const rect &r) = default;

    inline bool operator==(const rect &r) const;
    inline bool operator!=(const rect &r) const;

    inline point pos() const;
    inline esp::gfx::size size() const;
    inline int width() const;
    inline int height() const;

    inline rect translate(int offset_x, int offset_y) const;
    inline rect translate(const point &offset) const;
    inline rect move(const point &position) const;
    inline rect move(int x, int y) const;
    inline rect shrink(int value) const;
    inline rect h_shrink(int value) const;
    inline rect v_shrink(int value) const;
    inline rect resize(int width, int height) const;
    inline rect resize(esp::gfx::size size) const;

    rect intersection(const rect &rect) const;
    inline bool intersects(const rect &rect) const;
    inline bool contains(const rect &rect) const;
    inline bool contains(const point &point) const;
    inline bool contains(int x, int y) const;
    rect merge(const rect &rect) const;

    /** @brief Левая граница, пиксели; столбец входит в прямоугольник. */
    std::int16_t x1;

    /** @brief Верхняя граница, пиксели; строка входит в прямоугольник. */
    std::int16_t y1;

    /** @brief Правая граница, пиксели; столбец входит в прямоугольник. */
    std::int16_t x2;

    /** @brief Нижняя граница, пиксели; строка входит в прямоугольник. */
    std::int16_t y2;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт прямоугольник из одного пикселя в точке (0, 0).
 */
constexpr esp::gfx::rect::rect()
    : x1{0}
    , y1{0}
    , x2{0}
    , y2{0}
{
}

/**
 * @brief Создаёт прямоугольник по двум углам.
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
constexpr esp::gfx::rect::rect(int x1, int y1, int x2, int y2)
    : x1{static_cast<std::int16_t>(x1)}
    , y1{static_cast<std::int16_t>(y1)}
    , x2{static_cast<std::int16_t>(x2)}
    , y2{static_cast<std::int16_t>(y2)}
{
}

/**
 * @brief Копирует прямоугольник.
 *
 * Конструктор задан явно, а не по умолчанию, и потому нетривиален: такой
 * прямоугольник функции возвращают через память вызывающего. Тривиально
 * копируемую упакованную структуру они возвращали бы в регистрах, и в
 * каждом вызове merge() её пришлось бы раскладывать в память по байтам —
 * это добавляет около 30 байтов IRAM на вызов в коде рисования.
 *
 * @param r Исходный прямоугольник.
 */
esp::gfx::rect::rect(const rect &r)
    : x1{r.x1}
    , y1{r.y1}
    , x2{r.x2}
    , y2{r.y2}
{
}

/**
 * @fn esp::gfx::rect &esp::gfx::rect::operator=(const rect &r)
 * @brief Копирует прямоугольник присваиванием.
 * @param r Исходный прямоугольник.
 * @return Этот прямоугольник.
 */

/**
 * @brief Сравнивает прямоугольники.
 * @param r Второй прямоугольник.
 * @return @c true, если совпадают все четыре границы.
 */
bool esp::gfx::rect::operator==(const rect &r) const
{
    return x1 == r.x1 && y1 == r.y1 && x2 == r.x2 && y2 == r.y2;
}

/**
 * @brief Сравнивает прямоугольники.
 * @param r Второй прямоугольник.
 * @return @c true, если различается хотя бы одна граница.
 */
bool esp::gfx::rect::operator!=(const rect &r) const
{
    return x1 != r.x1 || y1 != r.y1 || x2 != r.x2 || y2 != r.y2;
}

/**
 * @brief Возвращает левый верхний угол.
 * @return Точка (x1, y1).
 */
esp::gfx::point esp::gfx::rect::pos() const
{
    return point(x1, y1);
}

/**
 * @brief Возвращает размер прямоугольника.
 * @return Ширина и высота; обе границы входят в прямоугольник.
 */
esp::gfx::size esp::gfx::rect::size() const
{
    return esp::gfx::size(x2 - x1 + 1, y2 - y1 + 1);
}

/**
 * @brief Возвращает ширину.
 * @return Число столбцов от @c x1 до @c x2 включительно.
 */
int esp::gfx::rect::width() const
{
    return x2 - x1 + 1;
}

/**
 * @brief Возвращает высоту.
 * @return Число строк от @c y1 до @c y2 включительно.
 */
int esp::gfx::rect::height() const
{
    return y2 - y1 + 1;
}

/**
 * @brief Сдвигает прямоугольник.
 * @param offset_x Сдвиг по горизонтали.
 * @param offset_y Сдвиг по вертикали.
 * @return Сдвинутый прямоугольник того же размера.
 */
esp::gfx::rect esp::gfx::rect::translate(int offset_x, int offset_y) const
{
    return rect(x1 + offset_x, y1 + offset_y, x2 + offset_x, y2 + offset_y);
}

/**
 * @brief Сдвигает прямоугольник.
 * @param offset Сдвиг по обеим осям.
 * @return Сдвинутый прямоугольник того же размера.
 */
esp::gfx::rect esp::gfx::rect::translate(const point &offset) const
{
    return rect(x1 + offset.x, y1 + offset.y, x2 + offset.x, y2 + offset.y);
}

/**
 * @brief Переносит прямоугольник в заданное место.
 * @param position Новый левый верхний угол.
 * @return Прямоугольник того же размера с углом в @p position.
 */
esp::gfx::rect esp::gfx::rect::move(const point &position) const
{
    return rect(position.x, position.y, position.x + width() - 1, position.y + height() - 1);
}

/**
 * @brief Переносит прямоугольник в заданное место.
 * @param x Новая левая граница.
 * @param y Новая верхняя граница.
 * @return Прямоугольник того же размера с углом в (@p x, @p y).
 */
esp::gfx::rect esp::gfx::rect::move(int x, int y) const
{
    return rect(x, y, x + width() - 1, y + height() - 1);
}

/**
 * @brief Сжимает прямоугольник со всех сторон.
 * @param value На сколько пикселей сдвинуть каждую границу внутрь.
 * @return Сжатый прямоугольник.
 */
esp::gfx::rect esp::gfx::rect::shrink(int value) const
{
    return rect(x1 + value, y1 + value, x2 - value, y2 - value);
}

/**
 * @brief Сжимает прямоугольник слева и справа.
 * @param value На сколько пикселей сдвинуть левую и правую границы внутрь.
 * @return Сжатый прямоугольник.
 */
esp::gfx::rect esp::gfx::rect::h_shrink(int value) const
{
    return rect(x1 + value, y1, x2 - value, y2);
}

/**
 * @brief Сжимает прямоугольник сверху и снизу.
 * @param value На сколько пикселей сдвинуть верхнюю и нижнюю границы внутрь.
 * @return Сжатый прямоугольник.
 */
esp::gfx::rect esp::gfx::rect::v_shrink(int value) const
{
    return rect(x1, y1 + value, x2, y2 - value);
}

/**
 * @brief Меняет размер, сохраняя левый верхний угол.
 * @param width Новая ширина.
 * @param height Новая высота.
 * @return Прямоугольник нового размера.
 */
esp::gfx::rect esp::gfx::rect::resize(int width, int height) const
{
    return rect(x1, y1, x1 + width - 1, y1 + height - 1);
}

/**
 * @brief Меняет размер, сохраняя левый верхний угол.
 * @param size Новый размер.
 * @return Прямоугольник нового размера.
 */
esp::gfx::rect esp::gfx::rect::resize(esp::gfx::size size) const
{
    return rect(x1, y1, x1 + size.width - 1, y1 + size.height - 1);
}

/**
 * @brief Проверяет, пересекаются ли прямоугольники.
 * @param rect Второй прямоугольник.
 * @return @c true, если у прямоугольников есть общий пиксель.
 */
bool esp::gfx::rect::intersects(const rect &rect) const
{
    return x1 <= rect.x2 && x2 >= rect.x1 && y1 <= rect.y2 && y2 >= rect.y1;
}

/**
 * @brief Проверяет, лежит ли другой прямоугольник целиком внутри этого.
 * @param rect Второй прямоугольник.
 * @return @c true, если все пиксели @p rect принадлежат этому прямоугольнику.
 */
bool esp::gfx::rect::contains(const rect &rect) const
{
    return (rect.x1 >= x1) && (rect.y1 >= y1) && (rect.x2 <= x2) && (rect.y2 <= y2);
}

/**
 * @brief Проверяет, лежит ли точка внутри прямоугольника.
 * @param point Точка.
 * @return @c true, если точка внутри или на границе.
 */
bool esp::gfx::rect::contains(const point &point) const
{
    return point.x >= x1 && point.y >= y1 && point.x <= x2 && point.y <= y2;
}

/**
 * @brief Проверяет, лежит ли точка внутри прямоугольника.
 * @param x Горизонтальная координата точки.
 * @param y Вертикальная координата точки.
 * @return @c true, если точка внутри или на границе.
 */
bool esp::gfx::rect::contains(int x, int y) const
{
    return x >= x1 && y >= y1 && x <= x2 && y <= y2;
}
