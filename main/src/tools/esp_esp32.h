#pragma once

#include <driver/gpio.h>


namespace esp
{

/// ESP32 chip package, read from eFuse (CHIP_VER_PKG).
/// Only the classic ESP32 has these packages; on other targets
/// chip_package() returns ChipPackage::Unknown.
enum class ChipPackage
{
    Unknown,  ///< Unknown package or not an ESP32.
    D0WDQ6,   ///< ESP32-D0WDQ6, QFN 6x6: ESP32-WROOM-32, ESP32-WROVER.
    D0WDQ5,   ///< ESP32-D0WD (D0WDQ5), QFN 5x5: ESP32-WROOM-32D/32U, ESP32-WROVER-B;
              ///< revision v3 (ESP32-D0WD-V3): ESP32-WROOM-32E, ESP32-WROVER-E.
    D2WDQ5,   ///< ESP32-D2WD (D2WDQ5): 2 MB in-package flash.
    U4WDH,    ///< ESP32-U4WDH: 4 MB in-package flash; ESP32-MINI-1.
    PicoD4,   ///< ESP32-PICO-D4 SiP, 4 MB flash: ESP32-PICO-KIT, TTGO VGA32.
    PicoV302, ///< ESP32-PICO-V3-02 SiP, 8 MB flash, 2 MB PSRAM: ESP32-PICO-MINI-02.
    D0WDR2V3, ///< ESP32-D0WDR2-V3: 2 MB in-package PSRAM.
};

esp::ChipPackage chip_package();
const char *chip_package_name(esp::ChipPackage package);

#define ESP_GPIO_UNUSED GPIO_NUM_MAX

inline gpio_num_t int2gpio(int gpio);

}


// Implementation

gpio_num_t esp::int2gpio(int gpio)
{
    return gpio == -1
        ? ESP_GPIO_UNUSED
        : (gpio_num_t)gpio;
}
