#pragma once

#include <driver/gpio.h>


namespace esp
{

/// Push button between a GPIO and ground (pressed = low), polled and debounced.
class button
{

public:

    /// name is shown in the console messages, e.g. "Left".
    button(gpio_num_t gpio, const char *name);

    /// Samples the pin; call it every few milliseconds.
    /// Returns true once per press, when the low level has held
    /// for DEBOUNCE_SAMPLES samples in a row.
    bool pressed();

    inline gpio_num_t gpio() const;
    inline const char *name() const;

private:

    static constexpr int DEBOUNCE_SAMPLES = 3;

    gpio_num_t _gpio;
    const char *_name;
    bool _down; ///< Debounced state: true while the button is held.
    int _count; ///< Samples in a row that disagree with _down.
};

}


// Implementation

gpio_num_t esp::button::gpio() const
{
    return _gpio;
}

const char *esp::button::name() const
{
    return _name;
}
