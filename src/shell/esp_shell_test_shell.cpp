/**
 * @file esp_shell_test_shell.h
 * @brief Объявление временной тестовой оболочки esp::shell::test_shell.
 */

/**
 * @file esp_shell_test_shell.cpp
 * @brief Временная тестовая оболочка терминала: эхо и несколько команд проверки.
 */

#include "esp_shell_test_shell.h"

#include "esp_board_test.h"
#include "esp_core_usage.h"
#include "esp_vt_controls.h"
#include "esp_vt_display.h"

#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <memory>

#include <esp_log.h>


/**
 * @class esp::shell::test_shell
 * @brief Временная тестовая оболочка терминала, пока нет Форта.
 *
 * Работает в своей задаче: выводит приглашение, читает строку и
 * выполняет команду, а незнакомую строку выводит обратно. Строка
 * набирается просто — знаки, Backspace, Enter, Ctrl+C; последовательности
 * особых клавиш пропускаются. Команды проверяют терминал: цвета и признаки
 * (@c colors), альтернативный экран и область прокрутки (@c vt),
 * видеорежимы (@c mode), прокрутку и историю экрана (@c lines), журнал
 * ESP-IDF (@c log) и звонок (@c bell); @c board-test запускает проверку
 * платы esp::board_test. Потом команды заменит Форт (раздел 13 документа
 * об устройстве).
 */


namespace esp::shell::p
{

/** @brief Приглашение: ярко-жёлтый треугольник. */
constexpr std::string_view PROMPT = "\x1b[93m▶\x1b[0m ";

/** @brief Размер стека задачи оболочки. */
constexpr int TASK_STACK = 6144;

/** @brief Приоритет задачи оболочки — ниже, чем у задач ввода. */
constexpr int TASK_PRIORITY = 3;

/** @brief Сколько ждать продолжения последовательности клавиши, мс. */
constexpr int SEQUENCE_TIME_OUT_MS = 50;

/** @brief Период опроса проверки платы, мс. */
constexpr int POLL_PERIOD_MS = 10;

/** @brief Тег журнала модуля. */
const char *const TAG = "shell";

/**
 * @brief Отрезает пробелы в начале и в конце.
 * @param text Строка.
 * @return Строка без пробелов по краям.
 */
std::string_view trim(std::string_view text)
{
    while (!text.empty() && text.front() == ' ')
        text.remove_prefix(1);
    while (!text.empty() && text.back() == ' ')
        text.remove_suffix(1);
    return text;
}

/**
 * @brief Читает неотрицательное число.
 * @param text Строка с числом.
 * @param fallback Что вернуть, если строка пуста.
 * @return Число; @c -1, если строка — не число.
 */
int number(std::string_view text, int fallback)
{
    if (text.empty())
        return fallback;
    int value = 0;
    const std::from_chars_result result =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < 0)
        return -1;
    return value;
}

}


// Открытые

/**
 * @brief Создаёт оболочку; задачу запускает start().
 * @param terminal Терминал; должен быть запущен и жить дольше оболочки.
 */
esp::shell::test_shell::test_shell(esp::vt::terminal &terminal)
    : _terminal{terminal}
    , _task{nullptr}
    , _line{}
    , _length{0}
    , _last_input{-1}
    , _is_board_test_running{false}
    , _is_button_pressed{false}
{
}

/**
 * @brief Запускает задачу оболочки.
 */
void esp::shell::test_shell::start()
{
    const int core = esp::core_usage::quiet_core();
    xTaskCreatePinnedToCore(&esp::shell::test_shell::task, "shell", esp::shell::p::TASK_STACK, this,
                            esp::shell::p::TASK_PRIORITY, &_task,
                            core >= 0 ? core : tskNO_AFFINITY);
    assert(_task);
}

/**
 * @brief Передаёт нажатие кнопки K1 проверке платы, если она идёт.
 *
 * Проверка рисует из задачи оболочки, а метод вызывает главная задача,
 * поэтому он только отмечает нажатие; проверке его передаёт board_test().
 *
 * @return @c true, если нажатие досталось проверке платы.
 */
bool esp::shell::test_shell::button_pressed()
{
    if (!_is_board_test_running)
        return false;
    _is_button_pressed = true;
    return true;
}


// Закрытые

/**
 * @brief Задача оболочки.
 * @param arg Оболочка.
 */
void esp::shell::test_shell::task(void *arg)
{
    static_cast<esp::shell::test_shell *>(arg)->run();
}

/**
 * @brief Цикл оболочки: приглашение, строка, команда. Не возвращает управление.
 */
void esp::shell::test_shell::run()
{
    _terminal.write("Test shell. Type \x1b[1mhelp\x1b[0m for the commands.\n");
    while (true)
    {
        _terminal.write(esp::shell::p::PROMPT);
        read_line();
        execute(
            esp::shell::p::trim(std::string_view{_line.data(), static_cast<std::size_t>(_length)}));
    }
}

/**
 * @brief Читает строку в @c _line.
 *
 * Знаки ASCII выводятся обратно, Backspace стирает последний, Enter
 * завершает строку, Ctrl+C её отменяет. Последовательности особых клавиш
 * пропускаются.
 */
void esp::shell::test_shell::read_line()
{
    _length = 0;
    while (true)
    {
        const int byte = _terminal.read(-1);
        const int last_input = _last_input;
        _last_input = byte;

        if (byte == esp::vt::CR || byte == esp::vt::LF)
        {
            if (byte == esp::vt::LF && last_input == esp::vt::CR)
                continue;
            _terminal.write("\n");
            return;
        }
        if (byte == esp::vt::DEL || byte == esp::vt::BS)
        {
            if (_length > 0)
            {
                --_length;
                _terminal.write("\b \b");
            }
        }
        else if (byte == 0x03)
        {
            _terminal.write("^C\n");
            _length = 0;
            return;
        }
        else if (byte == esp::vt::ESC)
            skip_sequence();
        else if (byte >= 0x20 && byte < 0x7F && _length < LINE_CAPACITY)
        {
            const char text = static_cast<char>(byte);
            _line[static_cast<std::size_t>(_length++)] = text;
            _terminal.write(std::string_view{&text, 1});
        }
    }
}

/**
 * @brief Пропускает остаток последовательности особой клавиши после ESC.
 *
 * После @c [ или @c O байты пропускаются до последнего байта 0x40–0x7E;
 * если продолжения нет дольше @c esp::shell::p::SEQUENCE_TIME_OUT_MS, это
 * была клавиша Esc.
 */
void esp::shell::test_shell::skip_sequence()
{
    const int introducer = _terminal.read(esp::shell::p::SEQUENCE_TIME_OUT_MS);
    if (introducer != '[' && introducer != 'O')
        return;
    for (int i = 0; i < 16; ++i)
    {
        const int byte = _terminal.read(esp::shell::p::SEQUENCE_TIME_OUT_MS);
        if (byte < 0 || (byte >= 0x40 && byte <= 0x7E))
            return;
    }
}

/**
 * @brief Выполняет команду.
 * @param line Строка без пробелов по краям: команда и её параметр.
 */
void esp::shell::test_shell::execute(std::string_view line)
{
    if (line.empty())
        return;
    const std::size_t space = line.find(' ');
    const std::string_view command = line.substr(0, space);
    const std::string_view argument = space == std::string_view::npos
        ? std::string_view{}
        : esp::shell::p::trim(line.substr(space));

    if (command == "help")
        help();
    else if (command == "clear")
        _terminal.write("\x1b[H\x1b[2J");
    else if (command == "colors")
        colors();
    else if (command == "vt")
        demo();
    else if (command == "mode")
        mode(argument);
    else if (command == "lines")
        lines(argument);
    else if (command == "bell")
        _terminal.write("\a");
    else if (command == "log")
        ESP_LOGW(esp::shell::p::TAG, "A test warning from the shell.");
    else if (command == "board-test")
        board_test();
    else
    {
        _terminal.write(line);
        _terminal.write("\n");
    }
}

/**
 * @brief Выводит список команд.
 */
void esp::shell::test_shell::help()
{
    _terminal.write("Commands:\n"
                    "  help        this list\n"
                    "  clear       clear the screen\n"
                    "  colors      colours and text attributes\n"
                    "  vt          alternate screen and scrolling region demo\n"
                    "  mode [n]    list the video modes or switch to mode n\n"
                    "  lines [n]   print n numbered lines (50 by default)\n"
                    "  log         print a test warning through the ESP-IDF log\n"
                    "  bell        ring the bell\n"
                    "  board-test  check VGA, keyboard, mouse, sound (Esc quits)\n"
                    "Anything else is echoed. Shift+PgUp and Shift+PgDn scroll back.\n");
}

/**
 * @brief Показывает 16 цветов текста и фона и признаки вида текста.
 */
void esp::shell::test_shell::colors()
{
    char text[64];
    for (int bright = 0; bright < 2; ++bright)
    {
        _terminal.write(bright ? "Bright: " : "Normal: ");
        for (int color = 0; color < 8; ++color)
        {
            std::snprintf(text, sizeof(text), "\x1b[%dm%2d \x1b[0m", (bright ? 90 : 30) + color,
                          bright * 8 + color);
            _terminal.write(text);
        }
        _terminal.write(" ");
        for (int color = 0; color < 8; ++color)
        {
            std::snprintf(text, sizeof(text), "\x1b[%dm  \x1b[0m", (bright ? 100 : 40) + color);
            _terminal.write(text);
        }
        _terminal.write("\n");
    }
    _terminal.write("Attributes: \x1b[1mbold\x1b[0m \x1b[2mdim\x1b[0m \x1b[3mitalic\x1b[0m "
                    "\x1b[4munderline\x1b[0m \x1b[7minverse\x1b[0m [\x1b[8mhidden\x1b[0m]\n");
}

/**
 * @brief Показывает альтернативный экран, область прокрутки и вставку и удаление строк.
 *
 * Сверху и снизу — полосы чёрным по светло-серому, между ними — область
 * прокрутки.
 * Клавиши: Enter добавляет строку в область, @c i вставляет строку вверху
 * области, @c d удаляет её, @c q возвращает на основной экран.
 */
void esp::shell::test_shell::demo()
{
    const int rows = _terminal.rows();
    char text[192];
    // Полосы — чёрным по светло-серому, а не инверсией: стирание до конца
    // строки заливает её цветом фона, а признак инверсии не переносит.
    std::snprintf(text, sizeof(text),
                  "\x1b[?1049h\x1b[2J\x1b[1;1H\x1b[30;47m Alternate screen demo\x1b[K\x1b[0m"
                  "\x1b[%d;1H\x1b[30;47m Enter: add, i: insert, d: delete, q: quit\x1b[K\x1b[0m"
                  "\x1b[2;%dr\x1b[2;1H",
                  rows, rows - 1);
    _terminal.write(text);

    int count = 0;
    while (true)
    {
        const int key = _terminal.read(-1);
        if (key == 'q')
            break;
        if (key == esp::vt::CR)
        {
            std::snprintf(text, sizeof(text), "\x1b[%d;1H\nLine %d", rows - 1, ++count);
            _terminal.write(text);
        }
        else if (key == 'i')
            _terminal.write("\x1b[2;1H\x1b[L\x1b[1mInserted line\x1b[0m");
        else if (key == 'd')
            _terminal.write("\x1b[2;1H\x1b[M");
    }
    _terminal.write("\x1b[r\x1b[?1049l");
}

/**
 * @brief Показывает видеорежимы или переключает видеорежим.
 * @param argument Пусто — показать список, номер — включить видеорежим.
 */
void esp::shell::test_shell::mode(std::string_view argument)
{
    char text[96];
    if (argument.empty())
    {
        for (int index = 0; index < esp::vt::display::mode_count(); ++index)
        {
            std::snprintf(text, sizeof(text), "%c %d  %s\n", index == _terminal.mode() ? '*' : ' ',
                          index, esp::vt::display::mode_label(index));
            _terminal.write(text);
        }
        return;
    }

    const int index = esp::shell::p::number(argument, -1);
    if (index < 0 || index >= esp::vt::display::mode_count())
    {
        _terminal.write("\x1b[91mNo such mode.\x1b[0m\n");
        return;
    }
    _terminal.set_mode(index);
    std::snprintf(text, sizeof(text), "Mode %d: %s, %dx%d characters.\n", index,
                  esp::vt::display::mode_label(index), _terminal.columns(), _terminal.rows());
    _terminal.write(text);
}

/**
 * @brief Выводит пронумерованные строки.
 * @param argument Число строк; пусто — 50.
 */
void esp::shell::test_shell::lines(std::string_view argument)
{
    const int count = esp::shell::p::number(argument, 50);
    if (count < 0)
    {
        _terminal.write("\x1b[91mNot a number.\x1b[0m\n");
        return;
    }
    char text[48];
    for (int line = 1; line <= count; ++line)
    {
        std::snprintf(text, sizeof(text), "Line %d of %d\n", line, count);
        _terminal.write(text);
    }
}

/**
 * @brief Запускает проверку платы и ждёт, пока она закончится.
 *
 * Терминал на это время приостанавливается (esp::vt::terminal::suspend())
 * и отдаёт проверке монитор, клавиатуру PS/2 и генератор звука; вывод, в
 * том числе журнал проверки, идёт в модель экрана и в UART0. Задача
 * оболочки опрашивает проверку каждые @c esp::shell::p::POLL_PERIOD_MS и
 * передаёт ей нажатия K1 (button_pressed()). Проверка заканчивается по Esc
 * на клавиатуре PS/2 или по одиночному ESC с терминала ПК, за которым
 * @c esp::shell::p::SEQUENCE_TIME_OUT_MS ничего не пришло; прочий ввод с
 * UART0 на это время пропускается. Затем терминал включает свой
 * видеорежим и рисует экран заново.
 */
void esp::shell::test_shell::board_test()
{
    _terminal.write("Board test. Press Esc to return to the shell.\n");
    _is_button_pressed = false;
    _is_board_test_running = true;
    _terminal.suspend();
    {
        // Проверка — в куче, а не на стеке оболочки: один видеоконтроллер
        // занимает около килобайта.
        const std::unique_ptr<esp::board_test> test =
            std::make_unique<esp::board_test>(_terminal.sound());
        while (!test->is_done())
        {
            // Ожидание ввода с UART0 — заодно и пауза между опросами.
            const int byte = _terminal.read(esp::shell::p::POLL_PERIOD_MS);
            if (byte == esp::vt::ESC && _terminal.read(esp::shell::p::SEQUENCE_TIME_OUT_MS) < 0)
                break;
            if (_is_button_pressed.exchange(false))
                test->button_pressed();
            test->poll();
        }
    }
    _terminal.resume();
    _is_board_test_running = false;
}
