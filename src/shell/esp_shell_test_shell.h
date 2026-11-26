#pragma once

#include "esp_non_copyable.h"
#include "esp_sdcard.h"
#include "esp_shell_history_file.h"
#include "esp_vt_line_edit.h"
#include "esp_vt_line_history.h"
#include "esp_vt_terminal.h"

#include <atomic>
#include <string_view>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace esp::shell
{

class test_shell : esp::non_copyable
{

public:

    test_shell(esp::vt::terminal &terminal, const esp::sd_card &sd_card);

    void start();
    bool button_pressed();

private:

    static void task(void *arg);

    void run();
    void execute(std::string_view line);
    void help();
    void colors();
    void demo();
    void mode(std::string_view argument);
    void lines(std::string_view argument);
    void history();
    void board_test();

    /** @brief Терминал, с которым работает оболочка. */
    esp::vt::terminal &_terminal;

    /** @brief Задача оболочки; @c nullptr, пока start() её не создал. */
    TaskHandle_t _task;

    /** @brief История строк: 8 КБ последних выполненных строк. */
    esp::vt::line_history _history;

    /** @brief Файл истории на SD-карте; без карты история живёт только в памяти. */
    esp::shell::history_file _history_file;

    /** @brief Редактор строки: через него набирается каждая команда. */
    esp::vt::line_edit _editor;

    /** @brief Идёт ли проверка платы; его читает и главная задача — в button_pressed(). */
    std::atomic<bool> _is_board_test_running;

    /** @brief Нажата ли K1 во время проверки платы: нажатие ждёт, пока его заберёт board_test(). */
    std::atomic<bool> _is_button_pressed;
};

}
