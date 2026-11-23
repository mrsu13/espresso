#pragma once

#include "esp_tools.h"

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

inline gpio_num_t int2gpio(int gpio);

}


// Реализация

/**
 * @brief Приводит номер вывода из @c int к типу @c gpio_num_t.
 * @param gpio Номер вывода или -1, если вывод не используется.
 * @return Номер вывода; esp::GPIO_UNUSED, если @p gpio равен -1.
 */
gpio_num_t esp::int2gpio(int gpio)
{
    return gpio == -1 ? esp::GPIO_UNUSED : static_cast<gpio_num_t>(gpio);
}
