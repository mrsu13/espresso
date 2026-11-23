// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Настройки модуля PS/2 esp::ps2.
 */


/**
 * @brief Нужны ли имена виртуальных клавиш: 1 — есть esp::ps2::keyboard::virtual_key_to_string().
 *
 * Таблица имён занимает около 3 КБ флеш-памяти.
 */
#define ESP_PS2_VIRTUAL_KEY_NAMES 1


namespace esp::ps2
{

/** @brief Стек задачи, переводящей скан-коды в виртуальные клавиши, байты. */
constexpr int SCANCODE_TASK_STACK_SIZE = 1500;

/** @brief Приоритет задачи, переводящей скан-коды в виртуальные клавиши. */
constexpr int SCANCODE_TASK_PRIORITY = 5;

/** @brief Длина очереди виртуальных клавиш. */
constexpr int VIRTUAL_KEY_QUEUE_SIZE = 32;

/** @brief Длина очереди состояний мыши (esp::ps2::mouse::get_next_status()). */
constexpr int MOUSE_EVENTS_QUEUE_SIZE = 64;

}
