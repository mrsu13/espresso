#pragma once

#include "esp_backlight.h"
#include "esp_button.h"
#include "esp_display.h"
#include "esp_sdcard.h"


namespace esp
{

class app
{

public:

    app();
    ~app();

    void dispatch();
    void exec();

private:

    void hello();
    void toggle_screen();

    /** @brief Карта памяти. На T-Display слота для неё нет, поэтому она не монтируется. */
    sd_card _sd_card;

    /** @brief Подсветка дисплея (GPIO4). */
    backlight _backlight;

    /** @brief Дисплей ST7789. */
    display _display;

    /** @brief Кнопки: левая (GPIO0) и правая (GPIO35). */
    button _buttons[2];
};

}
