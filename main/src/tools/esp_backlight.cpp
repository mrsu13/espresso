/**
 * @file esp_backlight.h
 * @brief Объявление класса подсветки дисплея esp::backlight.
 */

/**
 * @file esp_backlight.cpp
 * @brief Подсветка дисплея, управляемая выводом.
 */

#include "esp_backlight.h"

#include "esp_err.h"
#include "esp_log.h"


/**
 * @class esp::backlight
 * @brief Подсветка дисплея, которую включает высокий уровень на выводе.
 *
 * После создания объекта подсветка выключена. О каждом изменении её
 * состояния в консоль выводится сообщение.
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "backlight";

}


// Открытые

/**
 * @brief Настраивает вывод подсветки на выход и выключает подсветку.
 * @param gpio Вывод управления подсветкой.
 */
esp::backlight::backlight(gpio_num_t gpio)
    : _gpio(gpio)
    , _on(false)
{
    // Уровень задаётся до перевода вывода в режим выхода, чтобы подсветка
    // не вспыхнула на мгновение.
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

/**
 * @brief Включает подсветку.
 */
void esp::backlight::on()
{
    set(true);
}

/**
 * @brief Выключает подсветку.
 */
void esp::backlight::off()
{
    set(false);
}


// Закрытые

/**
 * @brief Устанавливает состояние подсветки.
 *
 * Повторная установка того же состояния ничего не делает и сообщения
 * не выводит.
 *
 * @param on @c true — включить подсветку, @c false — выключить.
 */
void esp::backlight::set(bool on)
{
    if (on == _on)
        return;

    gpio_set_level(_gpio, on ? 1 : 0);
    _on = on;

    ESP_LOGI(TAG, "Backlight %s.", on ? "on" : "off");
}
