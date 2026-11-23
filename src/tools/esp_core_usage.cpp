// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_core_usage.h
 * @brief Объявление выбора ядра для нагруженных задач esp::core_usage.
 */

/**
 * @file esp_core_usage.cpp
 * @brief Самое загруженное ядро по умолчанию — ядро видеовывода.
 */

#include "esp_core_usage.h"

#include "esp_tools.h"


/**
 * @class esp::core_usage
 * @brief Помогает выбрать ядро для задач и прерываний, нагружающих процессор.
 *
 * Хранит самое загруженное ядро — по умолчанию ядро видеовывода
 * esp::VIDEO_TASKS_CORE — и подсказывает другое, менее загруженное. На
 * ядре видеовывода работает обработчик прерывания VGA, который готовит
 * строки развёртки; задача примитивов контроллера VGA и обработчик
 * прерывания звука идут на другое ядро:
 * @code
 * xTaskCreatePinnedToCore(task, "", stack_size, arg, priority, &handle,
 *                         esp::core_usage::quiet_core());
 * @endcode
 *
 * Все методы статические, объект создать нельзя.
 */


int esp::core_usage::_busiest_core = esp::VIDEO_TASKS_CORE;
