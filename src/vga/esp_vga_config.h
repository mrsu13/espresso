// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Настройки модуля VGA esp::vga.
 */


/** Подстройка частоты APLL коэффициентами a и b: точнее, но
 * сигнал менее устойчив. */
#define ESP_VGA_USE_APLL_AB_COEF 0


/** Подсчёт тактов, занятых обработчиком прерывания VGA (отладка). */
#define ESP_VGA_PERFORMANCE_CHECK 0


namespace esp::vga
{

/** @brief Частота кварцевого генератора ESP32, Гц. */
constexpr int XTAL_FREQUENCY = 40000000;

/** @brief Наибольшее число больших блоков памяти, из которых
 *        собирается буфер кадра. */
constexpr int VIEW_PORT_MEMORY_POOL_COUNT = 128;

/** @brief Сколько байтов в наибольшем свободном блоке кучи должно
 *        остаться после выделения буфера кадра. */
constexpr int MIN_FREE_LARGEST_BLOCK = 40000;

/** @brief Стек задачи, рисующей примитивы, у контроллеров с палитрой. */
constexpr int PRIMITIVE_TASK_STACK_SIZE = 1200;

/** @brief Приоритет задачи, рисующей примитивы, у контроллеров с палитрой. */
constexpr int PRIMITIVE_TASK_PRIORITY = 5;

}
