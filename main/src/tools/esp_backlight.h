#pragma once

#include <driver/gpio.h>


namespace esp
{

class backlight
{

public:

    explicit backlight(gpio_num_t gpio);

    void on();
    void off();
    inline bool is_on() const;

private:

    void set(bool on);

    /** @brief Вывод управления подсветкой. */
    gpio_num_t _gpio;

    /** @brief Текущее состояние подсветки: @c true — включена. */
    bool _on;
};

}


// Реализация

/**
 * @brief Проверяет, включена ли подсветка.
 * @return @c true, если подсветка включена.
 */
bool esp::backlight::is_on() const
{
    return _on;
}
