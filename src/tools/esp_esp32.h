#pragma once

#include <driver/gpio.h>


namespace esp
{

enum class chip_package
{
    unknown,
    d0wdq6,
    d0wdq5,
    d2wdq5,
    u4wdh,
    pico_d4,
    pico_v302,
    d0wdr2v3,
};

esp::chip_package detected_chip_package();
const char *chip_package_name(esp::chip_package package);

#define ESP_GPIO_UNUSED GPIO_NUM_MAX

inline gpio_num_t int2gpio(int gpio);

}


// Реализация

/**
 * @brief Приводит номер вывода из @c int к типу @c gpio_num_t.
 * @param gpio Номер вывода или -1, если вывод не используется.
 * @return Номер вывода; ESP_GPIO_UNUSED, если @p gpio равен -1.
 */
gpio_num_t esp::int2gpio(int gpio)
{
    return gpio == -1 ? ESP_GPIO_UNUSED : (gpio_num_t)gpio;
}
