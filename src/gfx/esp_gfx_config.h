// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Настройки модуля графики esp::gfx.
 */


namespace esp::gfx
{

/** @brief Длина очереди примитивов контроллера дисплея. */
constexpr int DISPLAY_CONTROLLER_QUEUE_SIZE = 1024;

/** @brief Размер динамических буферов примитивов в байтах: в них хранятся
 *        точки ломаных draw_path() и fill_path(). */
constexpr int PRIMITIVE_BUFFER_SIZE = 512;

/** @brief Положение подчёркивания в строках от нижнего края знака
 *        (0 — нижняя строка). */
constexpr int UNDERLINE_POSITION = 0;

}
