#include "esp_app.h"

#include "esp_esp32.h"

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <cinttypes>
#include <cstdio>
#include <string>


namespace
{

const char *const TAG = "app";

// Modules whose debug output is shown in builds with assertions enabled.
const char *const DEBUG_TAGS[] = {"app", "backlight", "button", "sdcard"};

// TTGO T-Display: display backlight (on = high) and the two buttons;
// with the USB socket down, GPIO0 (BOOT) is on the left, GPIO35 on the right.
constexpr gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_4;
constexpr gpio_num_t BUTTON_LEFT_GPIO = GPIO_NUM_0;
constexpr gpio_num_t BUTTON_RIGHT_GPIO = GPIO_NUM_35;

// Button polling period; a press must hold for a few periods (debouncing).
constexpr int POLL_PERIOD_MS = 10;

}


// Public

esp::app::app()
    : _backlight(BACKLIGHT_GPIO)
    , _buttons{button(BUTTON_LEFT_GPIO), button(BUTTON_RIGHT_GPIO)}
{
#ifndef NDEBUG
    for (const char *tag : DEBUG_TAGS)
        esp_log_level_set(tag, ESP_LOG_DEBUG);
#endif

    if (!_sd_card.mount())
        ESP_LOGW(TAG, "No SD card.");

    hello();

    ESP_LOGD(TAG, "Pins: backlight GPIO%d, buttons GPIO%d and GPIO%d.",
             (int)BACKLIGHT_GPIO, (int)BUTTON_LEFT_GPIO, (int)BUTTON_RIGHT_GPIO);
    ESP_LOGD(TAG, "Application initialized.");
}

esp::app::~app()
{
    ESP_LOGD(TAG, "Application finished.");
}

void esp::app::dispatch()
{
    vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));

    // A press of either button turns the screen light on.
    for (button &b : _buttons)
    {
        if (b.pressed())
            _backlight.on();
    }
}

void esp::app::exec()
{
    while (true)
        dispatch();
}


// Private

void esp::app::hello()
{
    static const char *const BANNER_FMT =
R"(                         Version ........... %s
        (  (             Chip .............. %s
         )  )            Silicon revision .. v%d.%d
        (  (             CPU cores ......... %d
     .---------.         Wireless .......... %s
     |         |--.      Flash ............. %s
     |         |  |      PSRAM ............. %s
     \         /--'      Available memory .. %zu kB
   ___`-------'___       Free memory ....... %zu kB
  `---------------'      SD card ........... %s
      Espresso           MAC address ....... %02x:%02x:%02x:%02x:%02x:%02x
                         ESP-IDF ........... %s
)";

    esp_chip_info_t info;
    esp_chip_info(&info);

    const esp::ChipPackage package = esp::chip_package();
    const char *const chip = package == esp::ChipPackage::Unknown
        ? CONFIG_IDF_TARGET
        : esp::chip_package_name(package);

    std::string wireless;
    {
        const struct
        {
            std::uint32_t feature;
            const char *name;
        } features[] =
        {
            {CHIP_FEATURE_WIFI_BGN, "Wi-Fi"},
            {CHIP_FEATURE_BT, "BT"},
            {CHIP_FEATURE_BLE, "BLE"},
            {CHIP_FEATURE_IEEE802154, "802.15.4"},
        };

        for (const auto &f : features)
        {
            if (info.features & f.feature)
            {
                if (!wireless.empty())
                    wireless += '/';
                wireless += f.name;
            }
        }

        if (wireless.empty())
            wireless = "none";
    }

    char flash[48];
    {
        std::uint32_t size = 0;
        if (esp_flash_get_physical_size(nullptr, &size) == ESP_OK)
            std::snprintf(flash, sizeof(flash), "%" PRIu32 " MB %s",
                          size / (1024 * 1024),
                          (info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        else
            std::snprintf(flash, sizeof(flash), "%s", "unknown");
    }

    char psram[48];
    {
        const size_t size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        if (size)
            std::snprintf(psram, sizeof(psram), "%zu kB %s", size / 1024,
                          (info.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded" : "external");
        else
            std::snprintf(psram, sizeof(psram), "%s",
                          (info.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded, not enabled" : "not used");
    }

    char sdcard[48];
    {
        if (_sd_card.mounted())
            std::snprintf(sdcard, sizeof(sdcard), "%" PRIu64 " MB", _sd_card.capacity() / (1024 * 1024));
        else
            std::snprintf(sdcard, sizeof(sdcard), "%s", "No SD card");
    }

    std::uint8_t mac[6] = {};
    esp_efuse_mac_get_default(mac);

    const esp_app_desc_t *const desc = esp_app_get_description();

    std::printf(BANNER_FMT,
                desc->version,
                chip,
                info.revision / 100, info.revision % 100,
                info.cores,
                wireless.c_str(),
                flash,
                psram,
                heap_caps_get_total_size(MALLOC_CAP_32BIT) / 1024,
                heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024,
                sdcard,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                esp_get_idf_version());

    ESP_LOGD(TAG, "Package %s, features 0x%08" PRIx32 ", largest free block %zu bytes.",
             esp::chip_package_name(package), info.features,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
