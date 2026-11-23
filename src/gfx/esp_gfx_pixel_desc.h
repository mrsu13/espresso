// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Пиксель с цветом esp::gfx::pixel_desc.
 */


#include "esp_gfx_point.h"
#include "esp_gfx_rgb888.h"

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Пиксель: положение и цвет — параметр примитива рисования точки.
 *
 * Структура упакована, потому что входит в примитив рисования
 * @c esp::gfx::primitive.
 */
struct pixel_desc
{
    /** @brief Положение относительно начала координат холста. */
    point pos;

    /** @brief Цвет пикселя. */
    rgb888 color;
} PACKED_ATTR;

}
