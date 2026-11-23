// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Общие параметры рисования esp::gfx::paint_options.
 */


#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Общие параметры рисования: обмен цветов и инверсия.
 *
 * Действуют на примитивы, которые холст ставит в очередь после
 * esp::gfx::canvas::set_paint_options(). Структура упакована, потому что
 * входит в примитив рисования @c esp::gfx::primitive.
 */
struct paint_options
{
    inline paint_options();

    /** @brief Поменять местами цвета пера и кисти во всех примитивах. */
    std::uint8_t swap_fgbg : 1;

    /**
     * @brief Инвертировать пиксели вместо рисования цветом.
     *
     * Действует только на прямые линии и незалитые прямоугольники; повторное
     * рисование возвращает прежние пиксели.
     */
    std::uint8_t negate : 1;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт параметры без обмена цветов и без инверсии.
 */
esp::gfx::paint_options::paint_options()
    : swap_fgbg(false)
    , negate(false)
{
}
