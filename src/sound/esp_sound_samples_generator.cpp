// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_sound_samples_generator.h
 * @brief Объявление генератора отсчётов esp::sound::samples_generator.
 */

/**
 * @file esp_sound_samples_generator.cpp
 * @brief Генератор, воспроизводящий готовые отсчёты из памяти.
 */

#include "esp_sound_samples_generator.h"

#include <cstdint>


#pragma GCC optimize("O2")


/**
 * @class esp::sound::samples_generator
 * @brief Генератор, воспроизводящий готовые отсчёты из памяти.
 *
 * Отсчёты — 8-разрядные целые со знаком без сжатия, записанные с той же
 * частотой дискретизации, что у генератора звука: генератор отсчётов
 * выдаёт их по одному на отсчёт вывода, без пересчёта частоты. По концу
 * данных воспроизведение продолжается с начала, пока не истечёт
 * длительность. Частоты у генератора нет, set_frequency() ничего не
 * делает.
 *
 * Обычно генератор создаёт esp::sound::sound_generator::play_samples().
 */


// Открытые

/**
 * @brief Создаёт генератор для готовых отсчётов.
 *
 * @param data Отсчёты; не копируются и должны существовать, пока
 *        генератор подключён.
 * @param length Число отсчётов, больше нуля.
 */
esp::sound::samples_generator::samples_generator(const std::int8_t *data, int length)
    : _data{data}
    , _length{length}
    , _index{0}
{
}

/**
 * @brief Ничего не делает: отсчёты выдаются с частотой дискретизации.
 * @param value Не используется.
 */
void esp::sound::samples_generator::set_frequency(int value) {}

/**
 * @brief Выдаёт следующий отсчёт из памяти.
 * @return Отсчёт от −128 до 127 с учётом громкости; 0, если длительность истекла.
 */
int esp::sound::samples_generator::sample()
{
    if (!duration())
        return 0;

    int value = _data[_index++];

    // По концу данных воспроизведение продолжается с начала.
    if (_index == _length)
        _index = 0;

    // Громкость.
    value = value * volume() / 127;

    dec_duration();

    return value;
}
