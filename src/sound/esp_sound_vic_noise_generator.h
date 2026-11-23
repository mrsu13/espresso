// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_sound_waveform_generator.h"

#include <cstdint>


namespace esp::sound
{

class vic_noise_generator : public waveform_generator
{

public:

    vic_noise_generator();

    void set_frequency(int value) override;
    inline std::uint16_t frequency() const;

    int sample() override;

private:

    /**
     * @brief Начальное состояние регистра сдвига @c _lfsr.
     *
     * Регистр возвращается к нему при каждой смене частоты, поэтому шум
     * одной частоты всегда начинается одинаково.
     */
    static constexpr std::uint16_t LFSR_INIT = 0x0202;

    /**
     * @brief Тактовая частота VIC-20, Гц.
     *
     * 4,433618 МГц — частота кварца PAL-версии компьютера. Счётчик шума
     * работает на восьмой её части, поэтому на один отсчёт вывода
     * приходится @c CLOCK_FREQUENCY / 8 / частота дискретизации тактов
     * счётчика.
     */
    static constexpr int CLOCK_FREQUENCY = 4433618;

    /**
     * @brief Значение регистра частоты шумового голоса VIC, от 0 до 126.
     *
     * Им перезагружается счётчик @c _counter, который считает до 127,
     * поэтому чем больше значение, тем чаще такты регистра сдвига и тем
     * выше звук.
     */
    std::uint16_t _frequency;

    /**
     * @brief Делитель частоты: счётчик тактов до очередного шага регистра сдвига.
     *
     * Растёт на единицу за такт и, дойдя до 127, перезагружается значением
     * @c _frequency; в этот момент сдвигаются регистры шума.
     */
    std::uint16_t _counter;

    /**
     * @brief Регистр сдвига с линейной обратной связью — источник случайных разрядов.
     *
     * Сдвигается влево на каждом шаге; в освободившийся младший разряд
     * записывается сумма по модулю 2 разрядов 3, 12, 14 и 15 сдвинутого
     * значения. Младший разряд перед сдвигом решает, сдвигается ли выходной
     * регистр @c _out_sr.
     */
    std::uint16_t _lfsr;

    /**
     * @brief Выходной сдвиговый регистр шума.
     *
     * Младший разряд — текущий уровень выхода: единица даёт 127, ноль —
     * −128. Регистр сдвигается, только когда младший разряд @c _lfsr равен
     * единице.
     */
    std::uint16_t _out_sr;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает значение регистра частоты.
 * @return Значение регистра частоты шумового голоса VIC, от 0 до 126.
 */
std::uint16_t esp::sound::vic_noise_generator::frequency() const
{
    return _frequency;
}
