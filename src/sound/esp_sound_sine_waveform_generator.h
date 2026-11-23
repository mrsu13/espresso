// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_sound_waveform_generator.h"

#include <cstdint>


namespace esp::sound
{

class sine_waveform_generator : public waveform_generator
{

public:

    sine_waveform_generator();

    void set_frequency(int value) override;
    int sample() override;

private:

    /**
     * @brief Приращение фазы за один отсчёт.
     *
     * Фаза — 19-разрядное число с фиксированной запятой: старшие 8 разрядов
     * дают номер элемента таблицы синуса, младшие 11 — дробную часть.
     * Приращение вычисляет set_frequency() из частоты волны и частоты
     * дискретизации.
     */
    std::uint32_t _phase_inc;

    /**
     * @brief Аккумулятор фазы: текущая фаза волны.
     *
     * На каждом отсчёте к нему прибавляется @c _phase_inc по модулю 2^19,
     * так что за период волны фаза один раз проходит всю таблицу синуса.
     * Сбрасывается в ноль, когда при нулевой частоте выход затих.
     */
    std::uint32_t _phase_acc;

    /**
     * @brief Частота волны, Гц.
     *
     * Ноль означает тишину: выход плавно сходит к нулю. Изменение частоты
     * не сбрасывает фазу, поэтому волна продолжается без разрыва.
     */
    std::uint16_t _frequency;

    /**
     * @brief Последний выданный отсчёт.
     *
     * Когда частота становится нулевой, выход не обрывается, а сходит к
     * нулю от этого значения на единицу за отсчёт: резкий скачок был бы
     * слышен как щелчок.
     */
    std::int16_t _last_sample;
};

}
