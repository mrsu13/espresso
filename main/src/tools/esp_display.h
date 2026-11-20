#pragma once

#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <cstdint>


namespace esp
{

/// ST7789V display of the TTGO T-Display: 135 x 240 pixels in portrait
/// orientation, on SPI through esp_lcd. The backlight is separate
/// (esp::backlight on GPIO4).
class display
{

public:

    static constexpr int WIDTH = 135;
    static constexpr int HEIGHT = 240;

    /// RGB565 colours.
    static constexpr std::uint16_t BLACK = 0x0000;
    static constexpr std::uint16_t WHITE = 0xFFFF;

    display();
    ~display();

    /// Fills the whole screen with an RGB565 colour; returns when the
    /// data has been sent.
    bool fill(std::uint16_t color);

    inline bool ready() const;

private:

    bool init();
    void release();

    esp_lcd_panel_io_handle_t _io;
    esp_lcd_panel_handle_t _panel;
    SemaphoreHandle_t _sent; ///< Given once per finished band transfer.
    std::uint16_t *_band; ///< DMA buffer for one band of lines.
    bool _bus;
};

}


// Implementation

bool esp::display::ready() const
{
    return _panel;
}
