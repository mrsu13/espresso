// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Формат пикселя растра esp::gfx::pixel_format.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Формат пикселей растра @c esp::gfx::bitmap.
 *
 * От формата зависят размер данных растра и способ, которым контроллер
 * дисплея переносит его на экран.
 */
enum class pixel_format : std::uint8_t
{
    /** @brief Формат не задан; такой растр не рисуется. */
    undefined,

    /** @brief Собственный формат контроллера (esp::gfx::native_pixel_format); пиксели копируются как есть. */
    native,

    /**
     * @brief Маска: 1 разряд на пиксель.
     *
     * 0 — прозрачный пиксель, 1 — непрозрачный цвета
     * esp::gfx::bitmap::foreground_color. Строки по целому числу байтов,
     * старший разряд — левый пиксель.
     */
    mask,

    /**
     * @brief 8 разрядов на пиксель: AABBGGRR (@c esp::gfx::rgba2222).
     *
     * Каналы от 0 до 3; пиксель с альфа-каналом 0 прозрачен, остальные
     * рисуются непрозрачными.
     */
    rgba2222,

    /**
     * @brief 32 разряда на пиксель: байты R, G, B, A (@c esp::gfx::rgba8888).
     *
     * Каналы от 0 до 255; пиксель с альфа-каналом 0 прозрачен, остальные
     * рисуются непрозрачными.
     */
    rgba8888,
};

}
