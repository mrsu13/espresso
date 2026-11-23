// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_utils_p.h
 * @brief Закрытые помощники модуля VGA: APLL, выводы GPIO, такты процессора, цвета.
 */

/**
 * @file esp_vga_utils.cpp
 * @brief Закрытые помощники модуля VGA: подбор коэффициентов APLL и другие.
 */

#include "esp_vga_utils_p.h"

#include "esp_tools.h"
#include "esp_vga_config.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <driver/gpio.h>
#include <soc/io_mux_reg.h>
#include <soc/rtc.h>


#pragma GCC optimize("O2")


#if ESP_VGA_USE_APLL_AB_COEF
namespace esp::vga::p
{

void float_to_fraction(double value, int max_den, int *num, int *den);

}
#endif


// Закрытые

/**
 * @brief Включает генератор APLL с заданными коэффициентами или выключает его.
 * @param enable @c true — включить, @c false — выключить.
 * @param sdm0 Дробная часть множителя, младший байт.
 * @param sdm1 Дробная часть множителя, старший байт.
 * @param sdm2 Целая часть множителя сверх 4.
 * @param o_div Выходной делитель.
 */
void esp::vga::p::apll_enable(bool enable, std::uint32_t sdm0, std::uint32_t sdm1,
                              std::uint32_t sdm2, std::uint32_t o_div)
{
    rtc_clk_apll_enable(enable);
    if (enable)
        rtc_clk_apll_coeff_set(o_div, sdm0, sdm1, sdm2);
}

/**
 * @brief Переключает вывод на функцию GPIO и задаёт его направление.
 * @param gpio Вывод.
 * @param mode Режим: вход, выход, открытый сток и так далее.
 */
void esp::vga::p::configure_gpio(gpio_num_t gpio, gpio_mode_t mode)
{
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
    gpio_set_direction(gpio, mode);
}

/**
 * @brief Возвращает частоту процессора.
 * @return Частота, МГц.
 */
std::uint32_t esp::vga::p::cpu_frequency_mhz()
{
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return conf.freq_mhz;
}

/**
 * @brief Переводит цвет из RGB по 2 разряда на канал в HSV.
 * @param red Красный канал: 0…3.
 * @param green Зелёный канал: 0…3.
 * @param blue Синий канал: 0…3.
 * @param h Куда записать тон, градусы: 0…360.
 * @param s Куда записать насыщенность, проценты.
 * @param v Куда записать яркость, проценты.
 */
void esp::vga::p::rgb222_to_hsv(int red, int green, int blue, double *h, double *s, double *v)
{
    const double r = red / 3.0;
    const double g = green / 3.0;
    const double b = blue / 3.0;
    const double cmax = esp::tmax<double>(esp::tmax<double>(r, g), b);
    const double cmin = esp::tmin<double>(esp::tmin<double>(r, g), b);
    const double diff = cmax - cmin;
    if (cmax == cmin)
        *h = 0;
    else if (cmax == r)
        *h = std::fmod((60.0 * ((g - b) / diff) + 360.0), 360.0);
    else if (cmax == g)
        *h = std::fmod((60.0 * ((b - r) / diff) + 120.0), 360.0);
    else if (cmax == b)
        *h = std::fmod((60.0 * ((r - g) / diff) + 240.0), 360.0);
    *s = cmax == 0 ? 0 : (diff / cmax) * 100.0;
    *v = cmax * 100.0;
}

#if ESP_VGA_USE_APLL_AB_COEF
/**
 * @brief Приближает число дробью с ограниченным знаменателем (цепной дробью).
 * @param value Число, не меньше 0.
 * @param max_den Наибольший знаменатель, больше 1.
 * @param num Куда записать числитель.
 * @param den Куда записать знаменатель.
 */
void esp::vga::p::float_to_fraction(double value, int max_den, int *num, int *den)
{
    std::int64_t a, h[3] = {0, 1, 0}, k[3] = {1, 0, 0};
    std::int64_t x, d, n = 1;
    while (value != std::floor(value))
    {
        n <<= 1;
        value *= 2;
    }
    d = value;
    for (int i = 0; i < 64; ++i)
    {
        a = n ? d / n : 0;
        if (i && !a)
            break;
        x = d;
        d = n;
        n = x % n;
        x = a;
        if (k[1] * a + k[0] >= max_den)
        {
            x = (max_den - k[0]) / k[1];
            if (x * 2 >= a || k[1] >= max_den)
                i = 65;
            else
                break;
        }
        h[2] = x * h[1] + h[0];
        h[0] = h[1];
        h[1] = h[2];
        k[2] = x * k[1] + k[0];
        k[0] = k[1];
        k[1] = k[2];
    }
    *den = k[1];
    *num = h[1];
}
#endif

/**
 * @brief Подбирает коэффициенты APLL для частоты пикселей.
 *
 * Частота APLL = XTAL × (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / (2 × o_div + 4),
 * где делимое XTAL × (…) должно быть от 350 до 500 МГц; частота пикселей
 * = APLL / (2 + b / a) при tx_bck_div_num = 1 и clkm_div_num = 2.
 * Диапазоны: sdm0 и sdm1 — 0…255, sdm2 — 0…63, o_div — 0…31; XTAL —
 * esp::vga::XTAL_FREQUENCY (40 МГц). Отсюда частота APLL — от 5 303 030 до
 * 125 000 000 Гц; границы для каждого o_div:
 * @verbatim
 * MIN 87500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 0
 * MAX 125000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 0
 *
 * MIN 58333333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 1
 * MAX 83333333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 1
 *
 * MIN 43750000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 2
 * MAX 62500000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 2
 *
 * MIN 35000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 3
 * MAX 50000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 3
 *
 * MIN 29166666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 4
 * MAX 41666666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 4
 *
 * MIN 25000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 5
 * MAX 35714285Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 5
 *
 * MIN 21875000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 6
 * MAX 31250000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 6
 *
 * MIN 19444444Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 7
 * MAX 27777777Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 7
 *
 * MIN 17500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 8
 * MAX 25000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 8
 *
 * MIN 15909090Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 9
 * MAX 22727272Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 9
 *
 * MIN 14583333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 10
 * MAX 20833333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 10
 *
 * MIN 13461538Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 11
 * MAX 19230769Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 11
 *
 * MIN 12500000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 12
 * MAX 17857142Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 12
 *
 * MIN 11666666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 13
 * MAX 16666666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 13
 *
 * MIN 10937500Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 14
 * MAX 15625000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 14
 *
 * MIN 10294117Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 15
 * MAX 14705882Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 15
 *
 * MIN 9722222Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 16
 * MAX 13888888Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 16
 *
 * MIN 9210526Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 17
 * MAX 13157894Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 17
 *
 * MIN 8750000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 18
 * MAX 12500000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 18
 *
 * MIN 8333333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 19
 * MAX 11904761Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 19
 *
 * MIN 7954545Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 20
 * MAX 11363636Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 20
 *
 * MIN 7608695Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 21
 * MAX 10869565Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 21
 *
 * MIN 7291666Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 22
 * MAX 10416666Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 22
 *
 * MIN 7000000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 23
 * MAX 10000000Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 23
 *
 * MIN 6730769Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 24
 * MAX 9615384Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 24
 *
 * MIN 6481481Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 25
 * MAX 9259259Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 25
 *
 * MIN 6250000Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 26
 * MAX 8928571Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 26
 *
 * MIN 6034482Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 27
 * MAX 8620689Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 27
 *
 * MIN 5833333Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 28
 * MAX 8333333Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 28
 *
 * MIN 5645161Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 29
 * MAX 8064516Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 29
 *
 * MIN 5468750Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 30
 * MAX 7812500Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 30
 *
 * MIN 5303030Hz - sdm0 = 0 sdm1 = 192 sdm2 = 4 o_div = 31
 * MAX 7575757Hz - sdm0 = 0 sdm1 = 128 sdm2 = 8 o_div = 31
 * @endverbatim
 * Коэффициенты a и b подбираются, только если ESP_VGA_USE_APLL_AB_COEF
 * не 0; иначе a = 1, b = 0.
 *
 * @param freq Нужная частота пикселей, Гц.
 * @param params Куда записать коэффициенты APLL.
 * @param a Куда записать знаменатель дробного делителя.
 * @param b Куда записать числитель дробного делителя.
 * @param out_freq Куда записать получившуюся частоту, Гц.
 * @param error Куда записать погрешность: нужная частота минус получившаяся, Гц.
 */
void esp::vga::p::apll_calc_params(double freq, apll_params *params, std::uint8_t *a,
                                   std::uint8_t *b, double *out_freq, double *error)
{
    const double fxtal = esp::vga::XTAL_FREQUENCY;

    *error = 999999999;

    const double apll_freq = freq * 2;

    for (int o_div = 0; o_div <= 31; ++o_div)
    {
        const int idivisor = (2 * o_div + 4);

        for (int sdm2 = 4; sdm2 <= 8; ++sdm2)
        {
            // Пределы sdm1 — из таблицы выше.
            const int min_sdm1 = (sdm2 == 4 ? 192 : 0);
            const int max_sdm1 = (sdm2 == 8 ? 128 : 255);
            // apll_freq = XTAL * (4 + sdm2 + sdm1 / 256) / divisor ->
            // sdm1 = (apll_freq * divisor - XTAL * 4 - XTAL * sdm2) * 256 / XTAL
            const int start_sdm1 =
                ((apll_freq * idivisor - fxtal * 4.0 - fxtal * sdm2) * 256.0 / fxtal);
#if ESP_VGA_USE_APLL_AB_COEF
            for (int isdm1 = esp::tmax(min_sdm1, start_sdm1); isdm1 <= max_sdm1; ++isdm1)
            {
#else
            int isdm1 = start_sdm1;
            {
#endif

                int sdm1 = isdm1;
                sdm1 = esp::tmax(min_sdm1, sdm1);
                sdm1 = esp::tmin(max_sdm1, sdm1);

                // apll_freq = XTAL * (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / divisor ->
                // sdm0 = (apll_freq * divisor - XTAL * 4 - XTAL * sdm2 - XTAL * sdm1 / 256)
                //        * 65536 / XTAL
                int sdm0 =
                    ((apll_freq * idivisor - fxtal * 4.0 - fxtal * sdm2 - fxtal * sdm1 / 256.0)
                     * 65536.0 / fxtal);
                // Предел sdm0 — из таблицы выше.
                sdm0 = (sdm2 == 8 && sdm1 == 128 ? 0 : esp::tmin(255, sdm0));
                sdm0 = esp::tmax(0, sdm0);

                // Делимое должно быть от 350 до 500 МГц.
                const double dividend = fxtal * (4.0 + sdm2 + sdm1 / 256.0 + sdm0 / 65536.0);
                if (dividend >= 350000000 && dividend <= 500000000)
                {
                    const double oapll_freq = dividend / idivisor;

                    // Подстройка дробью b / a при tx_bck_div_num = 1 и
                    // clkm_div_num = 2:
                    //   freq = apll_clk / (2 + clkm_div_b / clkm_div_a)
                    //     abr = clkm_div_b / clkm_div_a
                    //     freq = apll_clk / (2 + abr) => abr = apll_clk / freq - 2
                    std::uint8_t oa = 1, ob = 0;
#if ESP_VGA_USE_APLL_AB_COEF
                    const double abr = oapll_freq / freq - 2.0;
                    if (abr > 0 && abr < 1)
                    {
                        int num, den;
                        esp::vga::p::float_to_fraction(abr, 63, &num, &den);
                        ob = esp::tclamp(num, 0, 63);
                        oa = esp::tclamp(den, 0, 63);
                    }
#endif

                    // Лучше найденного раньше?
                    const double ofreq = oapll_freq / (2.0 + static_cast<double>(ob) / oa);
                    const double err = freq - ofreq;
                    if (std::abs(err) < std::abs(*error))
                    {
                        *params = esp::vga::p::apll_params{
                            static_cast<std::uint8_t>(sdm0), static_cast<std::uint8_t>(sdm1),
                            static_cast<std::uint8_t>(sdm2), static_cast<std::uint8_t>(o_div)};
                        *a = oa;
                        *b = ob;
                        *out_freq = ofreq;
                        *error = err;
                        if (err == 0.0)
                            return;
                    }
                }
            }
        }
    }
}
