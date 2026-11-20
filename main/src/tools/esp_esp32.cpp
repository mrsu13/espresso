#include "esp_esp32.h"

#include "sdkconfig.h"

#if CONFIG_IDF_TARGET_ESP32
#   include "esp_efuse.h"
#   include "soc/efuse_defs.h"
#endif


esp::ChipPackage esp::chip_package()
{
#if CONFIG_IDF_TARGET_ESP32
    // CHIP_VER_PKG: eFuse block 0, both the 3-bit field and its 4th bit.
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

const char *esp::chip_package_name(esp::ChipPackage package)
{
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
