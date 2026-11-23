// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_sound_square_waveform_generator.h
 * @brief Объявление генератора прямоугольной волны esp::sound::square_waveform_generator.
 */

/**
 * @file esp_sound_square_waveform_generator.cpp
 * @brief Генератор прямоугольной волны с заданным коэффициентом заполнения.
 */

#include "esp_sound_square_waveform_generator.h"

#include <cstdint>


#pragma GCC optimize("O2")


/**
 * @class esp::sound::square_waveform_generator
 * @brief Генератор прямоугольной волны.
 *
 * Выход равен 127, пока положение в периоде не больше коэффициента
 * заполнения, и −127 на остальной части периода. Частота волны задаётся
 * в герцах и должна быть меньше половины частоты дискретизации; при
 * частоте выше 8191 Гц, кроме того, переполняется вычисление приращения
 * фазы.
 */


// Открытые

/**
 * @brief Создаёт генератор меандра с нулевой частотой: до set_frequency() он молчит.
 */
esp::sound::square_waveform_generator::square_waveform_generator()
    : _phase_inc{0}
    , _phase_acc{0}
    , _frequency{0}
    , _last_sample{0}
    , _duty_cycle{127}
{
}

/**
 * @brief Задаёт частоту прямоугольной волны.
 *
 * Приращение фазы пересчитывается, только если частота изменилась; фаза
 * при этом не сбрасывается.
 *
 * @param value Частота, Гц; 0 — тишина.
 * @pre Генератор подключён к генератору звука
 *      (esp::sound::sound_generator::attach()): до подключения частота
 *      дискретизации равна нулю, и деление на неё вызывает исключение
 *      процессора.
 */
void esp::sound::square_waveform_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value;
        // За секунду фаза должна _frequency раз пройти 256 положений периода
        // (с 11 дробными разрядами) за sample_rate() отсчётов.
        _phase_inc = ((static_cast<std::uint32_t>(_frequency) * 256) << 11) / sample_rate();
    }
}

/**
 * @brief Задаёт коэффициент заполнения.
 *
 * @param duty_cycle Коэффициент заполнения, от 0 до 255: высокий уровень
 *        занимает (@p duty_cycle + 1) / 256 периода; 127 — меандр,
 *        255 — постоянный высокий уровень.
 */
void esp::sound::square_waveform_generator::set_duty_cycle(int duty_cycle)
{
    _duty_cycle = duty_cycle;
}

/**
 * @brief Выдаёт следующий отсчёт прямоугольной волны.
 *
 * При нулевой частоте или истёкшей длительности выход сходит к нулю на
 * единицу за отсчёт; когда он дошёл до нуля, фаза сбрасывается.
 *
 * @return Отсчёт от −127 до 127 с учётом громкости.
 */
int esp::sound::square_waveform_generator::sample()
{
    if (!_frequency || !duration())
    {
        // Плавное затухание вместо обрыва: скачок к нулю слышен как щелчок.
        if (_last_sample > 0)
            --_last_sample;
        else if (_last_sample < 0)
            ++_last_sample;
        else
            _phase_acc = 0;
        return _last_sample;
    }

    // Положение в периоде — целая часть фазы.
    const std::uint32_t index = _phase_acc >> 11;
    int value = index <= _duty_cycle ? 127 : -127;

    // Громкость.
    value = value * volume() / 127;

    _last_sample = value;

    _phase_acc = (_phase_acc + _phase_inc) & 0x7ffff;

    dec_duration();

    return value;
}
