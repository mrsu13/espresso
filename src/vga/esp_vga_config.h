// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Настройки модуля VGA esp::vga.
 */


/**
 * @brief Подстраивать ли частоту APLL ещё и коэффициентами делителя a и b.
 *
 * 1 — частота пикселей точнее, но сигнал менее устойчив; по умолчанию 0.
 */
#define ESP_VGA_USE_APLL_AB_COEF 0


#include <cstddef>


namespace esp::vga
{

/** @brief Частота кварцевого генератора ESP32, Гц. */
constexpr int XTAL_FREQUENCY = 40000000;

/** @brief Наибольшее число блоков памяти, из которых собирается буфер кадра. */
constexpr int VIEW_PORT_MEMORY_POOL_COUNT = 128;

/**
 * @brief Сколько байтов кучи всего остаётся свободным после выделения
 *        буфера кадра, если контроллеру не задано иное
 *        (esp::vga::vga_base_controller::set_heap_reserve()).
 */
constexpr std::size_t DEFAULT_HEAP_RESERVE = 40000;

/** @brief Стек задачи, рисующей примитивы, у контроллеров с палитрой, байты. */
constexpr int PRIMITIVE_TASK_STACK_SIZE = 1200;

/** @brief Приоритет задачи, рисующей примитивы, у контроллеров с палитрой. */
constexpr int PRIMITIVE_TASK_PRIORITY = 5;

}
