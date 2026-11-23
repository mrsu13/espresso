// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Многоугольник для рисования esp::gfx::path.
 */


#include "esp_gfx_point.h"

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Замкнутый многоугольник — параметр примитивов рисования и заливки пути.
 *
 * Последняя вершина соединяется с первой. Структура упакована, потому что
 * входит в примитив рисования @c esp::gfx::primitive.
 */
struct path
{
    /**
     * @brief Вершины многоугольника.
     *
     * Контроллер дисплея копирует небольшие пути в свой пул памяти, чтобы
     * вызывающему не нужно было хранить вершины до рисования; тогда
     * @c free_points равно @c true.
     */
    const point *points;

    /** @brief Число вершин. */
    int points_count;

    /** @brief Освободить ли вершины в пуле контроллера после рисования. */
    bool free_points;
} PACKED_ATTR;

}
