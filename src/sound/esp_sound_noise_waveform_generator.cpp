// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_sound_noise_waveform_generator.h
 * @brief Объявление генератора белого шума esp::sound::noise_waveform_generator.
 */

/**
 * @file esp_sound_noise_waveform_generator.cpp
 * @brief Генератор белого шума на регистре сдвига.
 */

#include "esp_sound_noise_waveform_generator.h"


#pragma GCC optimize("O2")


/**
 * @class esp::sound::noise_waveform_generator
 * @brief Генератор белого шума.
 *
 * Шум даёт 16-разрядный регистр сдвига с линейной обратной связью в
 * схеме Галуа с многочленом x^16 + x^14 + x^13 + x^11 + 1: его
 * последовательность повторяется через 65535 отсчётов. Частоты у шума
 * нет, set_frequency() ничего не делает.
 */


// Открытые

/**
 * @brief Создаёт генератор шума.
 */
esp::sound::noise_waveform_generator::noise_waveform_generator()
    : _noise{0xFAB7}
{
}

/**
 * @brief Ничего не делает: частоты у белого шума нет.
 * @param value Не используется.
 */
void esp::sound::noise_waveform_generator::set_frequency(int value) {}

/**
 * @brief Выдаёт следующий отсчёт шума.
 * @return Отсчёт от −128 до 127 с учётом громкости; 0, если длительность истекла.
 */
int esp::sound::noise_waveform_generator::sample()
{
    if (!duration())
        return 0;

    // Шаг регистра Галуа: сдвиг вправо и, если выдвинута единица,
    // инверсия разрядов по маске многочлена.
    _noise = (_noise >> 1) ^ (-(_noise & 1) & 0xB400u);
    int value = 127 - (_noise >> 8);

    // Громкость.
    value = value * volume() / 127;

    dec_duration();

    return value;
}
