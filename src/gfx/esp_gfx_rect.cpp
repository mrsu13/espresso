// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_rect.h
 * @brief Объявление прямоугольника esp::gfx::rect.
 */

/**
 * @file esp_gfx_rect.cpp
 * @brief Прямоугольник: объединение и пересечение.
 */

#include "esp_gfx_rect.h"

#include "esp_tools.h"

#include <esp_attr.h>


#pragma GCC optimize("O2")


/**
 * @class esp::gfx::rect
 * @brief Прямоугольник, заданный левым верхним и правым нижним углами.
 *
 * Обе границы входят в прямоугольник: ширина равна @c x2 − @c x1 + 1, и
 * прямоугольник (0, 0, 0, 0) — это один пиксель. Прямоугольник с
 * @c x2 < @c x1 или @c y2 < @c y1 пуст; такой получается, например, при
 * пересечении непересекающихся прямоугольников. Структура упакована,
 * потому что входит в примитив рисования @c esp::gfx::primitive.
 *
 * Объединение (merge()) и пересечение (intersection()) лежат в IRAM: ими
 * пользуется рисование в обработчике прерывания кадровой развёртки.
 */


// Открытые

/**
 * @brief Возвращает пересечение прямоугольников.
 *
 * Границы пересечения — бо́льшие из левых и верхних и меньшие из правых и
 * нижних границ. Если прямоугольники не пересекаются, результат пуст
 * (правая граница левее левой или нижняя выше верхней).
 *
 * @param rect Второй прямоугольник.
 * @return Общая часть прямоугольников.
 */
esp::gfx::rect IRAM_ATTR esp::gfx::rect::intersection(const rect &rect) const
{
    return esp::gfx::rect(esp::tmax(x1, rect.x1), esp::tmax(y1, rect.y1), esp::tmin(x2, rect.x2),
                          esp::tmin(y2, rect.y2));
}

/**
 * @brief Возвращает наименьший прямоугольник, содержащий оба.
 *
 * Так копится область экрана, которую изменили примитивы рисования.
 *
 * @param rect Второй прямоугольник.
 * @return Объединяющий прямоугольник.
 */
esp::gfx::rect IRAM_ATTR esp::gfx::rect::merge(const rect &rect) const
{
    return esp::gfx::rect(esp::imin(rect.x1, x1), esp::imin(rect.y1, y1), esp::imax(rect.x2, x2),
                          esp::imax(rect.y2, y2));
}
