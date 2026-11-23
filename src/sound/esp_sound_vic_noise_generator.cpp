// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_sound_vic_noise_generator.h
 * @brief Объявление эмулятора шумового голоса VIC-20 esp::sound::vic_noise_generator.
 */

/**
 * @file esp_sound_vic_noise_generator.cpp
 * @brief Эмулятор шумового голоса микросхемы VIC 6561 компьютера VIC-20.
 */

#include "esp_sound_vic_noise_generator.h"


#pragma GCC optimize("O2")


/**
 * @class esp::sound::vic_noise_generator
 * @brief Эмулятор шумового голоса VIC 6561 (VIC-20).
 *
 * Повторяет шумовой генератор микросхемы по её описанию на VHDL,
 * восстановленному обратной разработкой и опубликованному на форуме Denial:
 * http://sleepingelephant.com/ipw-web/bulletin/bb/viewtopic.php?f=11&t=8733
 *
 * Счётчик тактов, перезагружаемый значением регистра частоты, задаёт темп
 * шагов регистра сдвига, а выходной сдвиговый регистр даёт уровень выхода.
 * Такты эмулируются с тактовой частотой VIC-20 и на каждый отсчёт вывода
 * усредняются.
 */


// Открытые

/**
 * @brief Создаёт генератор шума с регистром частоты, равным нулю, — самым низким звуком.
 */
esp::sound::vic_noise_generator::vic_noise_generator()
    : _frequency{0}
    , _counter{0}
    , _lfsr{LFSR_INIT}
    , _out_sr{0}
{
}

/**
 * @brief Задаёт значение регистра частоты шумового голоса VIC.
 *
 * Значение — не герцы, а содержимое регистра микросхемы: чем оно больше,
 * тем выше звук. При смене значения регистры шума и счётчик
 * сбрасываются.
 *
 * @param value Значение регистра, от 0 до 126; 127 и больше дают 0.
 */
void esp::sound::vic_noise_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value >= 127 ? 0 : value;
        _lfsr = LFSR_INIT;
        _counter = 0;
        _out_sr = 0;
    }
}

/**
 * @brief Выдаёт следующий отсчёт шума.
 *
 * Эмулирует такты счётчика VIC, приходящиеся на один отсчёт вывода, и
 * возвращает средний уровень выхода за эти такты.
 *
 * @return Отсчёт от −128 до 127 с учётом громкости; 0, если длительность истекла.
 * @pre Генератор подключён к генератору звука
 *      (esp::sound::sound_generator::attach()): до подключения частота
 *      дискретизации равна нулю, и деление на неё вызывает исключение
 *      процессора.
 */
int esp::sound::vic_noise_generator::sample()
{
    if (!duration())
        return 0;

    // Сколько тактов счётчика VIC приходится на один отсчёт вывода.
    const int ratio = CLOCK_FREQUENCY / 8 / sample_rate();

    int value = 0;

    for (int i = 0; i < ratio; ++i)
    {
        if (_counter >= 127)
        {
            // Счётчик дошёл до конца: перезагрузка значением регистра
            // частоты и шаг регистров шума.
            _counter = _frequency;

            if (_lfsr & 1)
                _out_sr = ((_out_sr << 1) | ~(_out_sr >> 7));

            _lfsr <<= 1;
            const int bit3 = (_lfsr >> 3) & 1;
            const int bit12 = (_lfsr >> 12) & 1;
            const int bit14 = (_lfsr >> 14) & 1;
            const int bit15 = (_lfsr >> 15) & 1;
            _lfsr |= (bit3 ^ bit12) ^ (bit14 ^ bit15);
        }
        else
            ++_counter;

        value += _out_sr & 1 ? 127 : -128;
    }

    // Средний уровень выхода за такты отсчёта.
    value = value / ratio;

    // Громкость.
    value = value * volume() / 127;

    dec_duration();

    return value;
}
