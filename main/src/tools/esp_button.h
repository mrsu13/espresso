#pragma once

#include <driver/gpio.h>


namespace esp
{

/// Push button between a GPIO and ground (pressed = low), polled and debounced.
class button
{

public:

    explicit button(gpio_num_t gpio);

    /// Samples the pin; call it every few milliseconds.
    /// Returns true once per press, when the low level has held
    /// for DEBOUNCE_SAMPLES samples in a row.
    bool pressed();

    inline gpio_num_t gpio() const;

private:

    static constexpr int DEBOUNCE_SAMPLES = 3;

    gpio_num_t _gpio;
    bool _down; ///< Debounced state: true while the button is held.
    int _count; ///< Samples in a row that disagree with _down.
};

}


// Implementation

gpio_num_t esp::button::gpio() const
{
    return _gpio;
}
