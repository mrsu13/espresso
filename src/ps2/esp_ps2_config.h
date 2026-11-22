// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Настройки модуля PS/2 esp::ps2.
 */


/** Имена виртуальных клавиш: метод keyboard::virtual_key_to_string(). */
#define ESP_PS2_VIRTUAL_KEY_NAMES 1


namespace esp::ps2
{

/** @brief Стек задачи, переводящей коды клавиш в виртуальные клавиши. */
constexpr int SCANCODE_TASK_STACK_SIZE = 1500;

/** @brief Приоритет задачи, переводящей коды клавиш в виртуальные клавиши. */
constexpr int SCANCODE_TASK_PRIORITY = 5;

/** @brief Длина очереди виртуальных клавиш. */
constexpr int VIRTUAL_KEY_QUEUE_SIZE = 32;

/** @brief Длина очереди событий мыши. */
constexpr int MOUSE_EVENTS_QUEUE_SIZE = 64;

}
