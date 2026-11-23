// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Собственный формат пикселя дисплея esp::gfx::native_pixel_format.
 */


#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Формат пикселя в буфере кадра контроллера дисплея.
 *
 * Сообщает контроллер (esp::gfx::bitmapped_display_controller::native_pixel_format()).
 * В этом формате хранятся растры @c esp::gfx::pixel_format::native и
 * сохранённый под спрайтами фон.
 */
enum class native_pixel_format : std::uint8_t
{
    /** @brief 1 разряд на пиксель: 0 — чёрный, 1 — белый. */
    mono,

    /**
     * @brief 8 разрядов на пиксель: VHBBGGRR.
     *
     * Разряд 7 — кадровая синхронизация, 6 — строчная, далее по два
     * разряда синего, зелёного и красного каналов (от 0 до 3).
     */
    sbgr2222,

    /** @brief 16 разрядов на пиксель: RGB565, старший байт первым. */
    rgb565be,

    /** @brief Палитра из 2 цветов: 1 разряд на пиксель, 8 пикселей в байте. */
    palette2,

    /** @brief Палитра из 4 цветов: 2 разряда на пиксель, 4 пикселя в байте. */
    palette4,

    /** @brief Палитра из 8 цветов: 3 разряда на пиксель, 8 пикселей в 3 байтах. */
    palette8,

    /** @brief Палитра из 16 цветов: 4 разряда на пиксель, 2 пикселя в байте. */
    palette16,
};

}
