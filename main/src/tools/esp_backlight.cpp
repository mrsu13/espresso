#include "esp_backlight.h"

#include "esp_err.h"
#include "esp_log.h"


namespace
{

const char *const TAG = "backlight";

}


// Public

esp::backlight::backlight(gpio_num_t gpio)
    : _gpio(gpio)
    , _on(false)
{
    // Set the level first, so that the pin does not flash high
    // when it becomes an output.
    gpio_set_level(gpio, 0);

    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << gpio;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s.", (int)gpio, esp_err_to_name(ret));
}

void esp::backlight::on()
{
    set(true);
}

void esp::backlight::off()
{
    set(false);
}


// Private

void esp::backlight::set(bool on)
{
    if (on == _on)
        return;

    gpio_set_level(_gpio, on ? 1 : 0);
    _on = on;

    ESP_LOGI(TAG, "Backlight %s.", on ? "on" : "off");
}
