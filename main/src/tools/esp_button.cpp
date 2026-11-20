#include "esp_button.h"

#include "esp_err.h"
#include "esp_log.h"


namespace
{

const char *const TAG = "button";

}


// Public

esp::button::button(gpio_num_t gpio, const char *name)
    : _gpio(gpio)
    , _name(name)
    , _down(false)
    , _count(0)
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << gpio;
    config.mode = GPIO_MODE_INPUT;
    // GPIO34-39 are input only and have no internal pull-up;
    // boards put an external one on buttons wired to them.
    config.pull_up_en = GPIO_IS_VALID_OUTPUT_GPIO(gpio)
        ? GPIO_PULLUP_ENABLE
        : GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s.", (int)gpio, esp_err_to_name(ret));

    // A button held at start-up is not a press: it has to be released first.
    _down = gpio_get_level(gpio) == 0;
}

bool esp::button::pressed()
{
    const bool down = gpio_get_level(_gpio) == 0;
    if (down == _down)
    {
        _count = 0;
        return false;
    }

    if (++_count < DEBOUNCE_SAMPLES)
        return false;

    _down = down;
    _count = 0;

    ESP_LOGI(TAG, "%s button (GPIO%d) %s.", _name, (int)_gpio, down ? "pressed" : "released");

    return down;
}
