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

    sd_card _sd_card;
    backlight _backlight;
    display _display;
    button _buttons[2];
};

}
