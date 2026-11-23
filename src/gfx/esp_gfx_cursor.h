// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Курсор мыши esp::gfx::cursor.
 */


#include "esp_gfx_bitmap.h"

#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Курсор мыши: изображение и его «горячая» точка.
 *
 * Горячая точка — пиксель изображения, который указывает на позицию мыши;
 * у стрелки это её остриё. Встроенные курсоры перечислены в
 * @c esp::gfx::cursor_name; свой курсор передаётся в
 * esp::gfx::bitmapped_display_controller::set_mouse_cursor() указателем и
 * должен существовать, пока он установлен.
 */
struct cursor
{
    /** @brief Горизонтальное положение горячей точки в изображении; 0 — левый край. */
    std::int16_t hotspot_x;

    /** @brief Вертикальное положение горячей точки в изображении; 0 — верхний край. */
    std::int16_t hotspot_y;

    /** @brief Изображение курсора. */
    esp::gfx::bitmap bitmap;
};

}
