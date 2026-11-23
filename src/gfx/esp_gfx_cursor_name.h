// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Встроенные курсоры мыши esp::gfx::cursor_name.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Встроенные курсоры мыши.
 *
 * Передаётся в esp::gfx::bitmapped_display_controller::set_mouse_cursor().
 * Изображения курсоров — растры формата @c esp::gfx::pixel_format::rgba2222;
 * размеры указаны в описании каждого значения (ширина × высота).
 */
enum class cursor_name : std::uint8_t
{
    /** @brief Цветная стрелка в духе Amiga, 11 × 11. */
    pointer_amiga_like,

    /** @brief Небольшая стрелка, 10 × 15. */
    pointer_simple_reduced,

    /** @brief Стрелка, 11 × 19. */
    pointer_simple,

    /** @brief Стрелка с тенью, 11 × 19. */
    pointer_shadowed,

    /** @brief Стрелка, 12 × 17. */
    pointer,

    /** @brief Перо, 16 × 16. */
    pen,

    /** @brief Крестик, 9 × 9. */
    cross_1,

    /** @brief Крестик, 11 × 11. */
    cross_2,

    /** @brief Точка, 5 × 5. */
    point,

    /** @brief Стрелка влево, 11 × 9. */
    left_arrow,

    /** @brief Стрелка вправо, 11 × 9. */
    right_arrow,

    /** @brief Стрелка вниз, 9 × 11. */
    down_arrow,

    /** @brief Стрелка вверх, 9 × 11. */
    up_arrow,

    /** @brief Перемещение — стрелки во все стороны, 19 × 19. */
    move,

    /** @brief Изменение размера по диагонали, 12 × 12. */
    resize_1,

    /** @brief Изменение размера по другой диагонали, 12 × 12. */
    resize_2,

    /** @brief Изменение размера по вертикали, 9 × 17. */
    resize_3,

    /** @brief Изменение размера по горизонтали, 17 × 9. */
    resize_4,

    /** @brief Текстовый курсор, 7 × 13. */
    text_input,
};

}
