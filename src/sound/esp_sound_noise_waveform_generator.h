// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_sound_waveform_generator.h"

#include <cstdint>


namespace esp::sound
{

class noise_waveform_generator : public waveform_generator
{

public:

    noise_waveform_generator();

    void set_frequency(int value) override;
    int sample() override;

private:

    /**
     * @brief Состояние 16-разрядного регистра сдвига с линейной обратной связью.
     *
     * Каждый отсчёт сдвигает регистр на разряд; старший байт состояния
     * даёт отсчёт шума. Начальное значение 0xFAB7 — любое ненулевое: из
     * нулевого состояния регистр не выходит.
     */
    std::uint16_t _noise;
};

}
