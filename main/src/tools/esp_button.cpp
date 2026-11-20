/**
 * @file esp_button.h
 * @brief Объявление класса кнопки esp::button.
 */

/**
 * @file esp_button.cpp
 * @brief Кнопка с опросом и подавлением дребезга контактов.
 */

#include "esp_button.h"

#include "esp_err.h"
#include "esp_log.h"


/**
 * @class esp::button
 * @brief Кнопка между выводом и землёй: нажатой кнопке соответствует низкий уровень.
 *
 * Состояние кнопки опрашивается методом pressed(), который вызывают каждые
 * несколько миллисекунд. Дребезг контактов подавляется: новое состояние
 * принимается, только когда оно держится DEBOUNCE_SAMPLES опросов подряд.
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "button";

}


// Открытые

/**
 * @brief Настраивает вывод кнопки на вход.
 *
 * Внутренняя подтяжка к питанию включается, если вывод её имеет.
 * Кнопка, нажатая при запуске, нажатием не считается, пока её не отпустят.
 *
 * @param gpio Вывод кнопки.
 * @param name Имя кнопки для сообщений в консоли, например «Left».
 */
esp::button::button(gpio_num_t gpio, const char *name)
    : _gpio(gpio)
    , _name(name)
    , _down(false)
    , _count(0)
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << gpio;
    config.mode = GPIO_MODE_INPUT;
    // GPIO34–39 работают только на вход и внутренней подтяжки не имеют:
    // на платах к кнопкам на этих выводах ставят внешний резистор.
    config.pull_up_en = GPIO_IS_VALID_OUTPUT_GPIO(gpio)
        ? GPIO_PULLUP_ENABLE
        : GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s.", (int)gpio, esp_err_to_name(ret));

    // Кнопка, нажатая при запуске, — ещё не нажатие: сначала её нужно отпустить.
    _down = gpio_get_level(gpio) == 0;
}

/**
 * @brief Опрашивает кнопку.
 *
 * Метод вызывают каждые несколько миллисекунд. Изменение состояния
 * принимается, когда новый уровень держится DEBOUNCE_SAMPLES опросов
 * подряд; о каждом принятом нажатии и отпускании в консоль выводится
 * сообщение.
 *
 * @return @c true один раз на каждое нажатие — в момент, когда оно принято.
 */
bool esp::button::pressed()
{
    const bool down = gpio_get_level(_gpio) == 0;
    if (down == _down)
    {
        // Уровень совпадает с принятым состоянием: дребезг закончился
        // или его не было, счёт начинается заново.
        _count = 0;
        return false;
    }

    // Новый уровень держится ещё недостаточно долго.
    if (++_count < DEBOUNCE_SAMPLES)
        return false;

    // Новое состояние принято.
    _down = down;
    _count = 0;

    ESP_LOGI(TAG, "%s button (GPIO%d) %s.", _name, (int)_gpio, down ? "pressed" : "released");

    return down;
}
