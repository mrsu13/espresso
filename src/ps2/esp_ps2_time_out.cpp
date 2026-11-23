// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_time_out.h
 * @brief Объявление отсчёта времени ожидания esp::ps2::time_out.
 */

/**
 * @file esp_ps2_time_out.cpp
 * @brief Отсчёт времени ожидания.
 */

#include "esp_ps2_time_out.h"

#include <esp_timer.h>


#pragma GCC optimize("O2")


/**
 * @class esp::ps2::time_out
 * @brief Отсчёт времени ожидания от момента создания.
 *
 * Помогает ждать ответа устройства PS/2 в цикле: объект запоминает момент
 * создания, а expired() сообщает, прошло ли с тех пор заданное время.
 */


// Открытые

/**
 * @brief Начинает отсчёт.
 */
esp::ps2::time_out::time_out()
    : _start{esp_timer_get_time()}
{
}

/**
 * @brief Сообщает, истекло ли время ожидания.
 * @param value_ms Время ожидания, мс; −1 — бесконечно, тогда время не истекает.
 * @return @c true, если с начала отсчёта прошло больше @p value_ms.
 */
bool esp::ps2::time_out::expired(int value_ms) const
{
    return value_ms > -1 && ((esp_timer_get_time() - _start) / 1000) > value_ms;
}
