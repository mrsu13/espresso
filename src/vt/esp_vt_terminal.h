#pragma once

#include "esp_non_copyable.h"
#include "esp_ps2_virtual_key_item.h"
#include "esp_sound_sound_generator.h"
#include "esp_vt_config.h"
#include "esp_vt_display.h"
#include "esp_vt_input_parser.h"
#include "esp_vt_interrupt_handler.h"
#include "esp_vt_key_event.h"
#include "esp_vt_parser.h"
#include "esp_vt_renderer.h"
#include "esp_vt_screen.h"
#include "esp_vt_writer.h"

#include <array>
#include <atomic>
#include <cstdarg>
#include <optional>
#include <string_view>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>


namespace esp::vt
{

class terminal : esp::non_copyable, public esp::vt::writer
{

public:

    terminal();
    ~terminal() override;

    void begin();
    void write(std::string_view text) override;
    int read(int time_out_ms);
    std::optional<esp::vt::key_event> read_key(int time_out_ms);
    bool is_input_ready();

    void set_mode(int mode);
    inline int mode() const;
    int columns();
    int rows();
    int cursor_column();
    bool is_wrap_pending();
    void tone(int frequency, int duration_ms);
    void stop_sound();
    void check_devices();
    void set_interrupt_handler(esp::vt::interrupt_handler *handler);
    bool interrupt();

    void suspend();
    void resume();
    inline esp::sound::sound_generator &sound();

private:

    static void keyboard_task(void *arg);
    static void uart_task(void *arg);
    static void blink(TimerHandle_t timer);
    static int log_vprintf(const char *format, va_list args);

    void start_screen();
    void put(std::string_view text);
    void feed(std::string_view bytes);
    void render();
    void show_current_lines();
    void on_key(const esp::ps2::virtual_key_item &item);
    void push_input(std::string_view bytes);
    void flush_input();

    /** @brief Вывод на VGA: видеоконтроллер, холст и шрифт. */
    esp::vt::display _display;

    /**
     * @brief Видеорежим терминала: номер в таблице esp::vt::display; @c -1 — до begin().
     *
     * Пока терминал приостановлен, вывод выключен, а номер остаётся: в этот
     * видеорежим вывод вернёт resume().
     */
    int _mode;

    /** @brief Экран; создаётся заново при смене видеорежима, потому что меняется его размер. */
    std::optional<esp::vt::screen> _screen;

    /** @brief Разборщик потока вывода; его исполнитель — @c _screen. */
    std::optional<esp::vt::parser> _parser;

    /** @brief Рисование @c _screen на холсте @c _display. */
    std::optional<esp::vt::renderer> _renderer;

    /** @brief Разборщик ввода для read_key(); им пользуется одна задача — та, что читает события. */
    esp::vt::input_parser _input_parser;

    /**
     * @brief Сбрасывали ли ввод с последнего чтения read_key() (flush_input()).
     *
     * Тогда read_key() начинает разбор заново: начатое событие собрать уже
     * не из чего. Ввод сбрасывает одна задача, а разбирает другая, поэтому
     * признак атомарный.
     */
    std::atomic<bool> _is_input_flushed;

    /** @brief Генератор звука для звонка BEL; пока терминал приостановлен, им пользуется другая программа. */
    esp::sound::sound_generator _sound;

    /**
     * @brief Мьютекс терминала.
     *
     * Экран меняют и рисуют несколько задач — та, что выводит, задачи
     * ввода и таймер мигания курсора, — поэтому всё это идёт под мьютексом.
     */
    SemaphoreHandle_t _lock;

    /** @brief Очередь байтов ввода с клавиатуры PS/2 и UART0; её читает read(). */
    QueueHandle_t _input;

    /** @brief Таймер FreeRTOS, который мигает курсором. */
    TimerHandle_t _blink_timer;

    /** @brief Задача, которая переводит нажатия клавиш PS/2 в байты ввода. */
    TaskHandle_t _keyboard_task;

    /** @brief Двоичный семафор: его отдаёт задача клавиатуры, когда после suspend() отпустила клавиатуру. */
    SemaphoreHandle_t _keyboard_released;

    /**
     * @brief Получатель прерывания: ему задачи ввода передают Ctrl+C; @c nullptr — некому.
     *
     * Его задаёт и читают разные задачи, поэтому указатель атомарный.
     */
    std::atomic<esp::vt::interrupt_handler *> _interrupt_handler;

    /** @brief Задача, которая принимает байты ввода с UART0. */
    TaskHandle_t _uart_task;

    /** @brief Прежний вывод журнала ESP-IDF; через него пишут строки, которые не могут пройти через терминал. */
    vprintf_like_t _previous_log;

    /** @brief Буфер для строки журнала ESP-IDF; используется под мьютексом. */
    std::array<char, esp::vt::LOG_LINE_MAX> _log_line;

    /**
     * @brief Приостановлен ли терминал (suspend()).
     *
     * Меняется под мьютексом, а задача клавиатуры читает его и без мьютекса,
     * поэтому он атомарный.
     */
    std::atomic<bool> _is_suspended;

    /** @brief Фаза мигания курсора: @c true — курсор показан. */
    bool _is_cursor_on;

    /** @brief Последний выведенный байт: LF после CR новой пары CR LF не требует. */
    char _last_byte;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает номер видеорежима терминала.
 *
 * Пока терминал приостановлен, это видеорежим, в который вывод вернёт resume().
 *
 * @return Номер в таблице esp::vt::display.
 */
int esp::vt::terminal::mode() const
{
    return _mode;
}

/**
 * @brief Возвращает генератор звука терминала.
 *
 * Генератор нельзя использовать из двух задач сразу, а терминал подаёт им
 * звонок BEL. Поэтому другая программа пользуется генератором, только пока
 * терминал приостановлен (suspend()).
 *
 * @return Генератор звука.
 */
esp::sound::sound_generator &esp::vt::terminal::sound()
{
    return _sound;
}
