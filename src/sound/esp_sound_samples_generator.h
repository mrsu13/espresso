// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_sound_waveform_generator.h"

#include <cstdint>


namespace esp::sound
{

class samples_generator : public waveform_generator
{

public:

    samples_generator(const std::int8_t *data, int length);

    void set_frequency(int value) override;
    int sample() override;

private:

    /**
     * @brief Воспроизводимые отсчёты.
     *
     * Генератор их не копирует и не освобождает: память принадлежит
     * вызывающему и должна существовать, пока генератор подключён. Обычно
     * это константа во флеш-памяти.
     */
    const std::int8_t *_data;

    /**
     * @brief Число отсчётов в @c _data.
     *
     * Дойдя до конца данных, воспроизведение продолжается с начала;
     * сколько раз пройти данные, определяет длительность генератора.
     */
    int _length;

    /**
     * @brief Номер следующего выдаваемого отсчёта в @c _data.
     *
     * Растёт на каждом отсчёте и обнуляется на @c _length.
     */
    int _index;
};

}
