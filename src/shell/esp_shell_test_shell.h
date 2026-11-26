#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_terminal.h"

#include <array>
#include <atomic>
#include <string_view>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace esp::shell
{

class test_shell : esp::non_copyable
{

public:

    explicit test_shell(esp::vt::terminal &terminal);

    void start();
    bool button_pressed();

private:

    /** @brief Наибольшая длина строки ввода, байты. */
    static constexpr int LINE_CAPACITY = 120;

    static void task(void *arg);

    void run();
    void read_line();
    void skip_sequence();
    void execute(std::string_view line);
    void help();
    void colors();
    void demo();
    void mode(std::string_view argument);
    void lines(std::string_view argument);
    void board_test();

    /** @brief Терминал, с которым работает оболочка. */
    esp::vt::terminal &_terminal;

    /** @brief Задача оболочки; @c nullptr, пока start() её не создал. */
    TaskHandle_t _task;

    /** @brief Набираемая строка. */
    std::array<char, LINE_CAPACITY> _line;

    /** @brief Длина набираемой строки. */
    int _length;

    /** @brief Предыдущий байт ввода: LF сразу после CR второй строкой не считается. */
    int _last_input;

    /** @brief Идёт ли проверка платы; его читает и главная задача — в button_pressed(). */
    std::atomic<bool> _is_board_test_running;

    /** @brief Нажата ли K1 во время проверки платы: нажатие ждёт, пока его заберёт board_test(). */
    std::atomic<bool> _is_button_pressed;
};

}
