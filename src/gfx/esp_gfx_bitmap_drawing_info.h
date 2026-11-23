// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Параметры рисования растра esp::gfx::bitmap_drawing_info.
 */


#include "esp_gfx_bitmap.h"

#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Растр и место, где его нарисовать, — параметр примитива рисования растра.
 *
 * Растр передаётся указателем и не копируется: он должен существовать, пока
 * примитив не выполнен. Структура упакована, потому что входит в примитив
 * рисования @c esp::gfx::primitive.
 */
struct bitmap_drawing_info
{
    inline bitmap_drawing_info(int x, int y, const esp::gfx::bitmap *bitmap);

    /** @brief Левая граница растра, пиксели, относительно начала координат холста. */
    std::int16_t x;

    /** @brief Верхняя граница растра, пиксели, относительно начала координат холста. */
    std::int16_t y;

    /** @brief Рисуемый растр. */
    const esp::gfx::bitmap *bitmap;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Задаёт растр и его положение.
 * @param x Левая граница.
 * @param y Верхняя граница.
 * @param bitmap Растр.
 */
esp::gfx::bitmap_drawing_info::bitmap_drawing_info(int x, int y, const esp::gfx::bitmap *bitmap)
    : x{static_cast<std::int16_t>(x)}
    , y{static_cast<std::int16_t>(y)}
    , bitmap{bitmap}
{
}
