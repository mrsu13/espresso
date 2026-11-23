// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Текущие параметры рисования esp::gfx::paint_state.
 */


#include "esp_gfx_glyph_options.h"
#include "esp_gfx_line_ends.h"
#include "esp_gfx_paint_options.h"
#include "esp_gfx_point.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"

#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Текущие параметры рисования контроллера дисплея.
 *
 * Их меняют примитивы рисования по мере выполнения: цвета, позиция,
 * параметры знаков, области. Начальные значения задаёт
 * esp::gfx::bitmapped_display_controller::reset_paint_state().
 */
struct paint_state
{
    /** @brief Цвет пера: линии, рамки, точки, знаки. */
    rgb888 pen_color;

    /** @brief Цвет кисти: заливки, фон знаков, очистка. */
    rgb888 brush_color;

    /** @brief Текущая позиция в абсолютных координатах экрана — начало координат уже прибавлено. */
    point position;

    /** @brief Параметры рисования знаков. */
    esp::gfx::glyph_options glyph_options;

    /** @brief Общие параметры рисования: обмен цветов и инверсия. */
    esp::gfx::paint_options paint_options;

    /** @brief Область прокрутки в абсолютных координатах; по умолчанию вся область вывода. */
    rect scrolling_region;

    /** @brief Начало координат: на него сдвигаются координаты всех примитивов. */
    point origin;

    /** @brief Область отсечения относительно начала координат, как её задал холст. */
    rect clipping_rect;

    /**
     * @brief Действующая область отсечения в абсолютных координатах.
     *
     * Вычисляется из @c clipping_rect и @c origin и обрезается по области
     * вывода, когда меняется одно из них
     * (esp::gfx::bitmapped_display_controller::update_absolute_clipping_rect()).
     */
    rect abs_clipping_rect;

    /** @brief Толщина пера, пиксели; не меньше 1. */
    std::int16_t pen_width;

    /** @brief Форма концов линий толщиной больше 1. */
    esp::gfx::line_ends line_ends;
};

}
