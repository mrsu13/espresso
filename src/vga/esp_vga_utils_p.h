// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include <cstdint>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <sdkconfig.h>


/**
 * @brief Определён, если компилятор обходит ошибку PSRAM ESP32 ревизии 1.
 *
 * В ESP32 ревизии 1 обращения процессора к внешней SRAM через кэш иногда
 * дают ошибки чтения и записи. Компилятор обходит это вставками между
 * загрузкой и записью (ключ -mfix-esp32-psram-cache-issue), что замедляет
 * и код, который к PSRAM не обращается, — в том числе обработчики
 * прерываний VGA. С IDF 5 у обхода свой параметр
 * CONFIG_SPIRAM_CACHE_WORKAROUND, а CONFIG_SPIRAM_SUPPORT больше нет.
 */
#if defined(CONFIG_SPIRAM_SUPPORT) || defined(CONFIG_SPIRAM_CACHE_WORKAROUND)
#   define ESP_VGA_PSRAM_HACK_NEEDED
#endif

/**
 * @def ESP_VGA_PSRAM_HACK()
 * @brief Команда @c nop между загрузкой и записью в обработчике прерывания VGA.
 *
 * Сбивает компилятор, и он не вставляет медленный обход ошибки PSRAM
 * (см. ESP_VGA_PSRAM_HACK_NEEDED); без обхода пустая.
 */
#ifdef ESP_VGA_PSRAM_HACK_NEEDED
#   define ESP_VGA_PSRAM_HACK() asm(" nop")
#else
#   define ESP_VGA_PSRAM_HACK()
#endif


namespace esp::vga::p
{

/**
 * @brief Коэффициенты генератора APLL.
 *
 * Частота APLL = XTAL × (4 + sdm2 + sdm1 / 256 + sdm0 / 65536) / (2 × o_div + 4).
 */
struct apll_params
{
    /** @brief Дробная часть множителя, младший байт: 0…255. */
    std::uint8_t sdm0;

    /** @brief Дробная часть множителя, старший байт: 0…255. */
    std::uint8_t sdm1;

    /** @brief Целая часть множителя сверх 4: 0…63. */
    std::uint8_t sdm2;

    /** @brief Выходной делитель: 0…31. */
    std::uint8_t o_div;
};

void apll_calc_params(double freq, apll_params *params, std::uint8_t *a, std::uint8_t *b,
                      double *out_freq, double *error);
void apll_enable(bool enable, std::uint32_t sdm0 = 0, std::uint32_t sdm1 = 0,
                 std::uint32_t sdm2 = 0, std::uint32_t o_div = 0);

void configure_gpio(gpio_num_t gpio, gpio_mode_t mode);
std::uint32_t cpu_frequency_mhz();
FORCE_INLINE_ATTR std::uint32_t cycle_count();

void rgb222_to_hsv(int red, int green, int blue, double *h, double *s, double *v);

}


// Реализация

/**
 * @brief Возвращает счётчик тактов процессора (регистр CCOUNT).
 *
 * Чтение занимает около 0,07 мкс против 0,78 мкс у esp_timer_get_time().
 * У каждого ядра свой счётчик, поэтому сравнивать отсчёты можно только в
 * задаче, закреплённой за одним ядром.
 *
 * @return Число тактов процессора.
 */
std::uint32_t esp::vga::p::cycle_count()
{
    std::uint32_t ccount;
    __asm__ __volatile__("esync \n\t"
                         "rsr %0, ccount \n\t"
                         : "=a"(ccount));
    return ccount;
}
