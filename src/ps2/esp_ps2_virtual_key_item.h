// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Событие клавиатуры esp::ps2::virtual_key_item.
 */


#include "esp_ps2_virtual_key.h"

#include <cstdint>


namespace esp::ps2
{

/**
 * @brief Событие клавиатуры: виртуальная клавиша, её состояние, скан-код и знак ASCII.
 *
 * Такие события клавиатура esp::ps2::keyboard ставит в очередь; их
 * получают esp::ps2::keyboard::get_next_virtual_key() и обработчик
 * esp::ps2::keyboard::on_virtual_key. Флаги модификаторов показывают их
 * состояние в момент события.
 */
struct virtual_key_item
{
    /** @brief Виртуальная клавиша. */
    virtual_key vk;

    /** @brief Состояние клавиши: 1 — нажата, 0 — отпущена. */
    std::uint8_t down;

    /**
     * @brief Скан-код события.
     *
     * Байты скан-кода; если их меньше восьми, за последним идёт 0. Восемь
     * байтов без завершающего нуля бывают у PAUSE.
     */
    std::uint8_t scancode[8];

    /** @brief Знак ASCII клавиши; 0, если клавиша в ASCII не переводится. */
    std::uint8_t ascii;

    /** @brief Нажат ли CTRL. */
    std::uint8_t ctrl : 1;

    /** @brief Нажат ли левый ALT. */
    std::uint8_t lalt : 1;

    /** @brief Нажат ли правый ALT (ALTGR). */
    std::uint8_t ralt : 1;

    /** @brief Нажат ли SHIFT. */
    std::uint8_t shift : 1;

    /** @brief Нажата ли клавиша GUI (Windows). */
    std::uint8_t gui : 1;

    /** @brief Включён ли CAPS LOCK. */
    std::uint8_t capslock : 1;

    /** @brief Включён ли NUM LOCK. */
    std::uint8_t numlock : 1;

    /** @brief Включён ли SCROLL LOCK. */
    std::uint8_t scrolllock : 1;
};

}
