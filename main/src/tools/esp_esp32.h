#pragma once

#include <driver/gpio.h>


namespace esp
{

enum class ChipPackage
{
    Unknown,
    D0WDQ6,
    D0WDQ5,
    D2WDQ5,
    U4WDH,
    PicoD4,
    PicoV302,
    D0WDR2V3,
};

esp::ChipPackage chip_package();
const char *chip_package_name(esp::ChipPackage package);

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
    return gpio == -1
        ? ESP_GPIO_UNUSED
        : (gpio_num_t)gpio;
}
