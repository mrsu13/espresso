#pragma once

#include <driver/gpio.h>


namespace esp
{

/// Display backlight switched by a GPIO (on = high). Off after construction.
class backlight
{

public:

    explicit backlight(gpio_num_t gpio);

    void on();
    void off();
    inline bool is_on() const;

private:

    void set(bool on);

    gpio_num_t _gpio;
    bool _on;
};

}


// Implementation

bool esp::backlight::is_on() const
{
    return _on;
}
