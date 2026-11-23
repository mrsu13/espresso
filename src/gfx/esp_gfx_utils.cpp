// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_utils.h
 * @brief Объявление вспомогательных функций модуля esp::gfx.
 */

/**
 * @file esp_gfx_utils.cpp
 * @brief Вспомогательные функции графики: отсечение отрезка.
 */

#include "esp_gfx_utils.h"


#pragma GCC optimize("O2")


namespace esp::gfx::p
{

int clip_line_code(int x, int y, const esp::gfx::rect &clip_rect);

}


// Открытые

/**
 * @brief Отсекает отрезок прямоугольником по алгоритму Коэна — Сазерленда.
 *
 * Концы отрезка по очереди переносятся на границы прямоугольника, пока оба
 * не окажутся внутри или пока не станет ясно, что отрезок целиком снаружи.
 *
 * @param x1 Абсцисса первого конца; при успехе и @p check_only = @c false
 *        заменяется отсечённой.
 * @param y1 Ордината первого конца; заменяется так же.
 * @param x2 Абсцисса второго конца; заменяется так же.
 * @param y2 Ордината второго конца; заменяется так же.
 * @param clip_rect Прямоугольник отсечения.
 * @param check_only @c true — только проверить видимость, не меняя
 *        координат.
 * @return @c true, если отрезок хотя бы частично внутри прямоугольника.
 */
bool esp::gfx::clip_line(int &x1, int &y1, int &x2, int &y2, const rect &clip_rect, bool check_only)
{
    int new_x1 = x1;
    int new_y1 = y1;
    int new_x2 = x2;
    int new_y2 = y2;
    int top_left_code = esp::gfx::p::clip_line_code(new_x1, new_y1, clip_rect);
    int bottom_right_code = esp::gfx::p::clip_line_code(new_x2, new_y2, clip_rect);
    while (true)
    {
        if (!top_left_code && !bottom_right_code)
        {
            // Оба конца внутри.
            if (!check_only)
            {
                x1 = new_x1;
                y1 = new_y1;
                x2 = new_x2;
                y2 = new_y2;
            }
            return true;
        }
        else if (top_left_code & bottom_right_code)
        {
            // Оба конца по одну сторону от прямоугольника: отрезок снаружи.
            break;
        }
        else
        {
            // Конец, который снаружи, переносится на пересекаемую границу.
            int x = 0;
            int y = 0;
            const int ncode = top_left_code ? top_left_code : bottom_right_code;
            if (ncode & 8)
            {
                x = new_x1 + (new_x2 - new_x1) * (clip_rect.y2 - new_y1) / (new_y2 - new_y1);
                y = clip_rect.y2;
            }
            else if (ncode & 4)
            {
                x = new_x1 + (new_x2 - new_x1) * (clip_rect.y1 - new_y1) / (new_y2 - new_y1);
                y = clip_rect.y1;
            }
            else if (ncode & 2)
            {
                y = new_y1 + (new_y2 - new_y1) * (clip_rect.x2 - new_x1) / (new_x2 - new_x1);
                x = clip_rect.x2;
            }
            else if (ncode & 1)
            {
                y = new_y1 + (new_y2 - new_y1) * (clip_rect.x1 - new_x1) / (new_x2 - new_x1);
                x = clip_rect.x1;
            }
            if (ncode == top_left_code)
            {
                new_x1 = x;
                new_y1 = y;
                top_left_code = esp::gfx::p::clip_line_code(new_x1, new_y1, clip_rect);
            }
            else
            {
                new_x2 = x;
                new_y2 = y;
                bottom_right_code = esp::gfx::p::clip_line_code(new_x2, new_y2, clip_rect);
            }
        }
    }
    return false;
}

// Закрытые

/**
 * @brief Вычисляет код области точки для алгоритма Коэна — Сазерленда.
 *
 * Разряд 0 — точка левее прямоугольника, 1 — правее, 2 — выше, 3 — ниже;
 * 0 — точка внутри.
 *
 * @param x Абсцисса точки.
 * @param y Ордината точки.
 * @param clip_rect Прямоугольник отсечения.
 * @return Код области.
 */
int esp::gfx::p::clip_line_code(int x, int y, const esp::gfx::rect &clip_rect)
{
    int code = 0;
    if (x < clip_rect.x1)
        code = 1;
    else if (x > clip_rect.x2)
        code = 2;
    if (y < clip_rect.y1)
        code |= 4;
    else if (y > clip_rect.y2)
        code |= 8;
    return code;
}
