#pragma once

#include "esp_button.h"
#include "esp_sdcard.h"
#include "esp_shell_test_shell.h"
#include "esp_vt_terminal.h"


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

    /** @brief Терминал: монитор VGA, клавиатура PS/2 и UART0. */
    esp::vt::terminal _terminal;

    /** @brief Временная тестовая оболочка терминала, пока нет Форта (этап 1). */
    esp::shell::test_shell _shell;
};

}
