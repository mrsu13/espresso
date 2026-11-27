#pragma once

#include "esp_button.h"
#include "esp_sdcard.h"
#include "esp_shell_forth_shell.h"
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

    /** @brief Карта памяти: монтируется при запуске, если вставлена; на ней лежит файл истории строк. */
    sd_card _sd_card;

    /** @brief Кнопка K1 (GPIO36), единственная пользовательская кнопка платы. */
    button _button;

    /** @brief Терминал: монитор VGA, клавиатура PS/2 и UART0. */
    esp::vt::terminal _terminal;

    /** @brief Оболочка Форта на терминале. */
    esp::shell::forth_shell _shell;
};

}
