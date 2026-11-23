// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Параметры развёртки VGA esp::vga::vga_timings.
 */


#include "esp_vga_vga_scan_start.h"

#include <cstdint>


namespace esp::vga
{

/**
 * @brief Параметры развёртки: разобранный модлайн.
 *
 * Их получает из строки модлайна
 * esp::vga::vga_base_controller::convert_modeline_to_timings(); готовые
 * модлайны — в esp_vga_modelines.h.
 */
struct vga_timings
{
    /** @brief Имя режима из модлайна, например «640x480@60Hz». */
    char label[22];

    /** @brief Частота пикселей, Гц. */
    int frequency;

    /** @brief Видимая часть строки, пиксели. */
    std::int16_t h_visible_area;

    /** @brief Передний гасящий интервал строки, пиксели. */
    std::int16_t h_front_porch;

    /** @brief Строчный синхроимпульс, пиксели. */
    std::int16_t h_sync_pulse;

    /** @brief Задний гасящий интервал строки, пиксели. */
    std::int16_t h_back_porch;

    /** @brief Видимые строки кадра. */
    std::int16_t v_visible_area;

    /** @brief Передний гасящий интервал кадра, строки. */
    std::int16_t v_front_porch;

    /** @brief Кадровый синхроимпульс, строки. */
    std::int16_t v_sync_pulse;

    /** @brief Задний гасящий интервал кадра, строки. */
    std::int16_t v_back_porch;

    /** @brief Полярность строчного синхроимпульса: «+» или «-». */
    char h_sync_logic;

    /** @brief Полярность кадрового синхроимпульса: «+» или «-». */
    char v_sync_logic;

    /** @brief Сколько раз выводится каждая строка: 1, 2 (DoubleScan) или 4 (QuadScan). */
    std::uint8_t scan_count;

    /** @brief Повторы строки: 0 — те же пиксели, 1 — чёрные (MultiScanBlank). */
    std::uint8_t multi_scan_black;

    /** @brief С какого участка начинается строка в буфере DMA. */
    vga_scan_start h_starting_block;
};

}
