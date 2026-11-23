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

/**
 * @brief Длина очереди примитивов контроллера дисплея по умолчанию, элементы.
 *
 * Холст ставит в очередь примитив на каждый вызов рисования, а контроллер
 * разбирает её на обратном ходе луча. Если очередь полна, рисующая задача
 * ждёт. Значение можно изменить до запуска контроллера через
 * esp::gfx::bitmapped_display_controller::queue_size.
 */
constexpr int DISPLAY_CONTROLLER_QUEUE_SIZE = 1024;

/**
 * @brief Размер пула памяти для данных примитивов, байты.
 *
 * В пул копируются вершины многоугольников esp::gfx::canvas::draw_path()
 * и esp::gfx::canvas::fill_path(), чтобы вызывающему не нужно было
 * хранить их до рисования. Многоугольник, вершины которого не помещаются
 * в пул, не копируется.
 */
constexpr int PRIMITIVE_BUFFER_SIZE = 512;

/**
 * @brief Строка подчёркивания знака, считая от нижнего края.
 *
 * 0 — подчёркивание на нижней строке знакоместа.
 */
constexpr int UNDERLINE_POSITION = 0;

}
