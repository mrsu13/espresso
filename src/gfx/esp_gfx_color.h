// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Именованные цвета esp::gfx::color.
 */


namespace esp::gfx
{

/**
 * @brief Шестнадцать именованных цветов палитры терминала.
 *
 * Первые восемь — тёмные цвета (каналы 0 или 128), вторые восемь — яркие
 * (каналы 0 или 255, кроме серого @c bright_black). В режиме с одним
 * разрядом на канал (8 цветов) тёмные и яркие цвета совпадают. Значение
 * переводится в @c esp::gfx::rgb888 конструктором этой структуры; номер
 * цвета служит и индексом палитры в 16-цветных режимах.
 */
enum class color
{
    /** @brief Чёрный: rgb888(0, 0, 0). */
    black,

    /** @brief Красный: rgb888(128, 0, 0). */
    red,

    /** @brief Зелёный: rgb888(0, 128, 0). */
    green,

    /** @brief Жёлтый (коричневый): rgb888(128, 128, 0). */
    yellow,

    /** @brief Синий: rgb888(0, 0, 128). */
    blue,

    /** @brief Пурпурный: rgb888(128, 0, 128). */
    magenta,

    /** @brief Бирюзовый: rgb888(0, 128, 128). */
    cyan,

    /** @brief Белый (светло-серый): rgb888(128, 128, 128). */
    white,

    /** @brief Тёмно-серый: rgb888(64, 64, 64). */
    bright_black,

    /** @brief Ярко-красный: rgb888(255, 0, 0). */
    bright_red,

    /** @brief Ярко-зелёный: rgb888(0, 255, 0). */
    bright_green,

    /** @brief Ярко-жёлтый: rgb888(255, 255, 0). */
    bright_yellow,

    /** @brief Ярко-синий: rgb888(0, 0, 255). */
    bright_blue,

    /** @brief Ярко-пурпурный: rgb888(255, 0, 255). */
    bright_magenta,

    /** @brief Ярко-бирюзовый: rgb888(0, 255, 255). */
    bright_cyan,

    /** @brief Ярко-белый: rgb888(255, 255, 255). */
    bright_white,
};

}
