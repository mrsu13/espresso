/**
 * @file esp_esp32.h
 * @brief Сведения о чипе ESP32: корпус и вспомогательные функции для выводов.
 */

/**
 * @file esp_esp32.cpp
 * @brief Определение корпуса чипа ESP32 по eFuse.
 */

#include "esp_esp32.h"

#include "sdkconfig.h"

#if CONFIG_IDF_TARGET_ESP32
#   include "esp_efuse.h"
#   include "soc/efuse_defs.h"
#endif


/**
 * @enum esp::ChipPackage
 * @brief Корпус чипа ESP32, записанный в eFuse (поле CHIP_VER_PKG).
 *
 * Такие корпуса бывают только у классического ESP32; для других целей
 * сборки esp::chip_package() возвращает ChipPackage::Unknown. В описаниях
 * значений перечислены модули и платы на этих корпусах.
 *
 * @var esp::ChipPackage::Unknown
 * Корпус неизвестен, или чип — не ESP32.
 *
 * @var esp::ChipPackage::D0WDQ6
 * ESP32-D0WDQ6, корпус QFN 6 × 6: модули ESP32-WROOM-32 и ESP32-WROVER,
 * плата TTGO T-Display.
 *
 * @var esp::ChipPackage::D0WDQ5
 * ESP32-D0WD (D0WDQ5), корпус QFN 5 × 5: модули ESP32-WROOM-32D/32U и
 * ESP32-WROVER-B; ревизия v3 (ESP32-D0WD-V3) — ESP32-WROOM-32E и
 * ESP32-WROVER-E.
 *
 * @var esp::ChipPackage::D2WDQ5
 * ESP32-D2WD (D2WDQ5): 2 МБ флеш-памяти в корпусе.
 *
 * @var esp::ChipPackage::U4WDH
 * ESP32-U4WDH: 4 МБ флеш-памяти в корпусе; модуль ESP32-MINI-1.
 *
 * @var esp::ChipPackage::PicoD4
 * Система в корпусе ESP32-PICO-D4 с 4 МБ флеш-памяти: плата ESP32-PICO-KIT,
 * плата TTGO VGA32.
 *
 * @var esp::ChipPackage::PicoV302
 * Система в корпусе ESP32-PICO-V3-02: 8 МБ флеш-памяти и 2 МБ PSRAM;
 * модуль ESP32-PICO-MINI-02.
 *
 * @var esp::ChipPackage::D0WDR2V3
 * ESP32-D0WDR2-V3: 2 МБ PSRAM в корпусе.
 */

/**
 * @def ESP_GPIO_UNUSED
 * @brief Номер вывода со смыслом «вывод не используется»; см. esp::int2gpio().
 */


/**
 * @brief Определяет корпус чипа по eFuse.
 * @return Корпус чипа; ChipPackage::Unknown, если корпус неизвестен или
 *         цель сборки — не ESP32.
 */
esp::ChipPackage esp::chip_package()
{
#if CONFIG_IDF_TARGET_ESP32
    // Поле CHIP_VER_PKG занимает три бита блока 0 eFuse и ещё один бит
    // отдельно; esp_efuse_get_pkg_ver() собирает их в одно число.
    switch (esp_efuse_get_pkg_ver())
    {
        case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6:
            return ChipPackage::D0WDQ6;
        case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ5:
            return ChipPackage::D0WDQ5;
        case EFUSE_RD_CHIP_VER_PKG_ESP32D2WDQ5:
            return ChipPackage::D2WDQ5;
        case EFUSE_RD_CHIP_VER_PKG_ESP32U4WDH:
            return ChipPackage::U4WDH;
        case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
            return ChipPackage::PicoD4;
        case EFUSE_RD_CHIP_VER_PKG_ESP32PICOV302:
            return ChipPackage::PicoV302;
        case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDR2V3:
            return ChipPackage::D0WDR2V3;
        default:
            break;
    }
#endif

    return ChipPackage::Unknown;
}

/**
 * @brief Возвращает название корпуса.
 * @param package Корпус чипа.
 * @return Название, например «ESP32-D0WDQ6»; «unknown» для неизвестного корпуса.
 */
const char *esp::chip_package_name(esp::ChipPackage package)
{
    // Названия даны как в техническом описании ESP32: корпус D0WDQ5
    // продаётся под именем ESP32-D0WD, D2WDQ5 — под именем ESP32-D2WD.
    switch (package)
    {
        case ChipPackage::D0WDQ6:
            return "ESP32-D0WDQ6";
        case ChipPackage::D0WDQ5:
            return "ESP32-D0WD";
        case ChipPackage::D2WDQ5:
            return "ESP32-D2WD";
        case ChipPackage::U4WDH:
            return "ESP32-U4WDH";
        case ChipPackage::PicoD4:
            return "ESP32-PICO-D4";
        case ChipPackage::PicoV302:
            return "ESP32-PICO-V3-02";
        case ChipPackage::D0WDR2V3:
            return "ESP32-D0WDR2-V3";
        case ChipPackage::Unknown:
            break;
    }

    return "unknown";
}
