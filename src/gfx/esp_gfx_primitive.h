// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Примитив рисования esp::gfx::primitive.
 */


#include "esp_gfx_bitmap_drawing_info.h"
#include "esp_gfx_glyph.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_line_ends.h"
#include "esp_gfx_paint_options.h"
#include "esp_gfx_path.h"
#include "esp_gfx_pixel_desc.h"
#include "esp_gfx_point.h"
#include "esp_gfx_primitive_cmd.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_size.h"

#include <cstdint>

#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace esp::gfx
{

/**
 * @brief Примитив рисования: команда и её параметр.
 *
 * Холст @c esp::gfx::canvas не рисует сам, а ставит примитивы в очередь
 * контроллера дисплея; тот выполняет их в обработчике кадровой развёртки
 * или сразу (esp::gfx::bitmapped_display_controller::add_primitive()).
 * Параметры разных команд лежат в одном объединении, а вся структура
 * упакована, чтобы элемент очереди был как можно меньше. Какое поле
 * объединения действует, определяет команда @c cmd (см.
 * @c esp::gfx::primitive_cmd).
 */
struct primitive
{
    inline primitive();
    inline explicit primitive(primitive_cmd cmd);
    inline primitive(primitive_cmd cmd, const esp::gfx::rect &rect);

    /** @brief Команда; определяет, какое поле объединения служит параметром. */
    primitive_cmd cmd;

    union
    {
        /** @brief Целое: шаг прокрутки или толщина пера. */
        std::int16_t ivalue;

        /** @brief Цвет пера или кисти. */
        rgb888 color;

        /** @brief Точка: позиция, конец линии, вершина точки или начало координат. */
        point position;

        /** @brief Размер эллипса. */
        esp::gfx::size size;

        /** @brief Знак и его положение. */
        esp::gfx::glyph glyph;

        /** @brief Прямоугольник: заливка, рамка, инверсия, копирование, области. */
        esp::gfx::rect rect;

        /** @brief Параметры рисования знаков. */
        esp::gfx::glyph_options glyph_options;

        /** @brief Общие параметры рисования. */
        esp::gfx::paint_options paint_options;

        /** @brief Растр и его положение. */
        esp::gfx::bitmap_drawing_info bitmap_drawing_info;

        /** @brief Многоугольник. */
        esp::gfx::path path;

        /** @brief Точка с цветом. */
        esp::gfx::pixel_desc pixel_desc;

        /** @brief Форма концов толстых линий. */
        esp::gfx::line_ends line_ends;

        /** @brief Задача, которую нужно разбудить после обмена буферов. */
        TaskHandle_t notify_task;
    } PACKED_ATTR;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт примитив без команды и параметра.
 *
 * Поля намеренно не заполняются: примитив создаётся на каждый вызов
 * рисования и сразу получает команду и параметр.
 */
esp::gfx::primitive::primitive() {}

/**
 * @brief Создаёт примитив команды без параметра.
 * @param cmd Команда.
 */
esp::gfx::primitive::primitive(primitive_cmd cmd)
    : cmd{cmd}
{
}

/**
 * @brief Создаёт примитив команды с прямоугольником.
 * @param cmd Команда.
 * @param rect Прямоугольник — параметр команды.
 */
esp::gfx::primitive::primitive(primitive_cmd cmd, const esp::gfx::rect &rect)
    : cmd{cmd}
    , rect{rect}
{
}
