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
#include <iterator>
#include <string>
#include <type_traits>


namespace
{

const char *const TAG = "app";

// Modules whose debug output is shown in builds with assertions enabled.
const char *const DEBUG_TAGS[] = {"app", "backlight", "button", "display", "sdcard"};

// TTGO T-Display: display backlight (on = high) and the two buttons;
// with the USB socket down, GPIO0 (BOOT) is on the left, GPIO35 on the right.
constexpr gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_4;
constexpr gpio_num_t BUTTON_LEFT_GPIO = GPIO_NUM_0;
constexpr gpio_num_t BUTTON_RIGHT_GPIO = GPIO_NUM_35;

// Button polling period; a press must hold for a few periods (debouncing).
constexpr int POLL_PERIOD_MS = 10;

// Pin probe, temporary: finds out which pin drives the backlight on this
// board. The left button toggles GPIO4, the right one GPIO14 (both start
// low), and every raw level change of the button pins is printed. Set
// PIN_PROBE to false for the normal behaviour: either button turns the
// backlight on.
constexpr bool PIN_PROBE = false;
constexpr gpio_num_t PROBE_GPIOS[] = {GPIO_NUM_4, GPIO_NUM_14}; // Per button: left, right.

struct probe_state
{
    int raw[std::size(PROBE_GPIOS)]; // Last raw level of each button pin.
    int out[std::size(PROBE_GPIOS)]; // Current level of each probed pin.
};

probe_state probe = {};

}


// Public

esp::app::app()
    : _backlight(BACKLIGHT_GPIO)
    , _buttons{button(BUTTON_LEFT_GPIO, "Left"), button(BUTTON_RIGHT_GPIO, "Right")}
{
#ifndef NDEBUG
    for (const char *tag : DEBUG_TAGS)
        esp_log_level_set(tag, ESP_LOG_DEBUG);
#endif

    // The T-Display has no SD card slot, and the mount attempt drives
    // GPIO14 and GPIO16 (the display's data/command line).
    // if (!_sd_card.mount())
    //     ESP_LOGW(TAG, "No SD card.");

    // White screen, so that the backlight switching on is easy to see.
    if (!_display.fill(display::WHITE))
        ESP_LOGW(TAG, "No display.");

    hello();

    ESP_LOGD(TAG, "Pins: backlight GPIO%d, buttons GPIO%d and GPIO%d.",
             (int)BACKLIGHT_GPIO, (int)BUTTON_LEFT_GPIO, (int)BUTTON_RIGHT_GPIO);
    if (PIN_PROBE)
    {
        static_assert(std::extent_v<decltype(_buttons)> == std::size(PROBE_GPIOS),
                      "one probed pin per button");

        for (const gpio_num_t gpio : PROBE_GPIOS)
        {
            gpio_set_level(gpio, 0);
            if (gpio == BACKLIGHT_GPIO)
                continue; // Already an output, set up by _backlight.

            gpio_config_t config = {};
            config.pin_bit_mask = 1ULL << gpio;
            config.mode = GPIO_MODE_OUTPUT;
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_DISABLE;
            ESP_ERROR_CHECK(gpio_config(&config));
        }

        for (size_t i = 0; i < std::size(_buttons); ++i)
            probe.raw[i] = gpio_get_level(_buttons[i].gpio());

        ESP_LOGI(TAG, "Pin probe: the left button toggles GPIO%d, the right one GPIO%d; both are low now.",
                 (int)PROBE_GPIOS[0], (int)PROBE_GPIOS[1]);
        ESP_LOGI(TAG, "Raw levels: GPIO%d = %d, GPIO%d = %d.",
                 (int)_buttons[0].gpio(), probe.raw[0], (int)_buttons[1].gpio(), probe.raw[1]);
    }

    ESP_LOGD(TAG, "Application initialized.");
}

esp::app::~app()
{
    ESP_LOGD(TAG, "Application finished.");
}

void esp::app::dispatch()
{
    vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));

    for (size_t i = 0; i < std::size(_buttons); ++i)
    {
        button &b = _buttons[i];

        if (PIN_PROBE)
        {
            const int raw = gpio_get_level(b.gpio());
            if (raw != probe.raw[i])
            {
                probe.raw[i] = raw;
                ESP_LOGI(TAG, "Raw: GPIO%d = %d.", (int)b.gpio(), raw);
            }
        }

        if (!b.pressed())
            continue;

        if (PIN_PROBE)
        {
            probe.out[i] = !probe.out[i];
            gpio_set_level(PROBE_GPIOS[i], probe.out[i]);
            ESP_LOGI(TAG, "%s button: GPIO%d is now %s; watch the screen.",
                     b.name(), (int)PROBE_GPIOS[i], probe.out[i] ? "high" : "low");
        }
        else
        {
            // A press of either button turns the screen light on.
            _backlight.on();
        }
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
