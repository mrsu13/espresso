#pragma once

#include "esp_button.h"
#include "esp_fabgl_test.h"
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

    /** @brief Карта памяти. Слот на плате есть, но карта пока не монтируется. */
    sd_card _sd_card;

    /** @brief Кнопка K1 (GPIO36), единственная пользовательская кнопка платы. */
    button _button;

    /** @brief Проверка FabGL: VGA, клавиатура и мышь PS/2, звук. */
    fabgl_test _test;
};

}
