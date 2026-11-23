// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Вид видеовыхода esp::video_mode.
 */


namespace esp
{

/**
 * @brief Вид видеовыхода, на который выводится изображение.
 *
 * Текущий вид хранит esp::current_video_mode. Из видеовыходов в программе
 * есть только VGA: значение @c vga задаёт esp::vga::vga_base_controller,
 * остальные значения пока никто не задаёт.
 */
enum class video_mode
{
    /** @brief Видеовыход ещё не настроен. */
    none,

    /** @brief Монитор VGA. */
    vga,

    /** @brief Композитный видеосигнал (CVBS) для телевизора. */
    cvbs,

    /** @brief Дисплей на шине I2C. */
    i2c,

    /** @brief Дисплей на шине SPI. */
    spi,
};

}
