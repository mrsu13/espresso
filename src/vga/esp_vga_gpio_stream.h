// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_tools.h"

#include <cstdint>

#include <driver/gpio.h>
#if __has_include(<esp32/rom/lldesc.h>)
#   include <esp32/rom/lldesc.h>
#else
#   include <rom/lldesc.h>
#endif


namespace esp::vga
{

class gpio_stream
{

public:

    void begin();
    void begin(bool div_1_on_gpio0, gpio_num_t div2 = esp::GPIO_UNUSED,
               gpio_num_t div4 = esp::GPIO_UNUSED, gpio_num_t div8 = esp::GPIO_UNUSED,
               gpio_num_t div16 = esp::GPIO_UNUSED, gpio_num_t div32 = esp::GPIO_UNUSED,
               gpio_num_t div64 = esp::GPIO_UNUSED, gpio_num_t div128 = esp::GPIO_UNUSED,
               gpio_num_t div256 = esp::GPIO_UNUSED);
    void end();

    void play(int freq, lldesc_t volatile *dma_buffers = nullptr);
    void stop();

private:

    void setup_clock(int freq);
    static void setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode);

    /** @brief Запущены ли модуль I2S1 и его DMA. */
    bool _dma_started;

    /** @brief Дескриптор DMA потока делителей частоты; @c nullptr, пока поток не создан. */
    volatile lldesc_t *_dma_buffer;

    /** @brief Данные потока делителей частоты: байты от 0 до 255. */
    volatile std::uint8_t *_dma_data;
};

}
