#pragma once

#include "esp_button.h"
#include "esp_sdcard.h"
#include "esp_shell_forth_shell.h"
#include "esp_vt_terminal.h"

#include <cstdint>


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

    /** @brief Когда нажали K1, мкс от запуска: по нему нажатие становится долгим. */
    std::int64_t _press_start_us;

    /**
     * @brief Ждёт ли нажатие K1, пока станет ясно, короткое оно или долгое.
     *
     * Нажатие, которое досталось проверке платы, и долгое нажатие, уже
     * выполненное, не ждут.
     */
    bool _is_press_pending;
};

}
