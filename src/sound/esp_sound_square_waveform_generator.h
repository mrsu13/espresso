// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_sound_waveform_generator.h"

#include <cstdint>


namespace esp::sound
{

class square_waveform_generator : public waveform_generator
{

public:

    square_waveform_generator();

    void set_frequency(int value) override;
    void set_duty_cycle(int duty_cycle);

    int sample() override;

private:

    /**
     * @brief Приращение фазы за один отсчёт.
     *
     * Фаза — 19-разрядное число с фиксированной запятой: старшие 8 разрядов
     * дают положение в периоде от 0 до 255, младшие 11 — дробную часть.
     * Приращение вычисляет set_frequency() из частоты волны и частоты
     * дискретизации.
     */
    std::uint32_t _phase_inc;

    /**
     * @brief Аккумулятор фазы: текущая фаза волны.
     *
     * На каждом отсчёте к нему прибавляется @c _phase_inc по модулю 2^19.
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
    std::int8_t _last_sample;

    /**
     * @brief Коэффициент заполнения, от 0 до 255.
     *
     * Высокий уровень держится, пока положение в периоде (от 0 до 255) не
     * больше этого значения, то есть (@c _duty_cycle + 1) / 256 периода.
     * По умолчанию 127 — меандр.
     */
    std::uint8_t _duty_cycle;
};

}
