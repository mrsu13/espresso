#pragma once

#include "esp_non_copyable.h"
#include "esp_sdcard.h"
#include "esp_shell_history_file.h"
#include "esp_vt_config.h"
#include "esp_vt_interrupt_handler.h"
#include "esp_vt_key_event.h"
#include "esp_vt_line_edit.h"
#include "esp_vt_line_history.h"
#include "esp_vt_terminal.h"

// ff.h есть и у FatFs из ESP-IDF, поэтому главный заголовок ff — по пути
// от src/3rdparty; остальные заголовки ff находятся по путям ff::ff.
#include <ff/src/ff.h>
#include <ff_platform.h>

#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string_view>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace esp::shell
{

class forth_shell : esp::non_copyable, public esp::vt::interrupt_handler
{

public:

    forth_shell(esp::vt::terminal &terminal, const esp::sd_card &sd_card);

    void start();
    bool button_pressed();
    bool interrupt() override;

    inline esp::vt::terminal &terminal();
    inline bool is_interrupted() const;
    void throw_interrupt();
    void write(std::string_view text);
    int wait_byte();
    std::optional<esp::vt::key_event> wait_key();
    void wait(int duration_ms);
    std::optional<std::string_view> read_line();
    bool set_video_mode(int mode);
    void board_test();
    void memstat();

private:

    /** @brief Размер приглашения: цвет, приглашение ff, пробел и конец цвета, байты. */
    static constexpr std::size_t PROMPT_CAPACITY = 32;

    static void task(void *arg);
    static int print_callback(void *context, const char *format, va_list args);
    static int trace_callback(void *context, ff_error_t error, const char *format, va_list args);
    static ff_watchdog_action_t watchdog_callback(void *context, std::uint64_t opcodes);
    static std::FILE *open_file_callback(void *context, const char *path, const char *mode);

    void run();
    bool create_engine();
    void evaluate(std::string_view line);
    void report_error(std::string_view line, ff_error_t error);
    void mark_error(std::string_view line, std::size_t position);
    bool is_top_level() const;
    void update_prompt();
    int print(const char *format, va_list args);
    std::size_t heap_reserve() const;
    void start_line();

    /** @brief Терминал, с которым работает оболочка. */
    esp::vt::terminal &_terminal;

    /** @brief Карта памяти: от её корня отсчитываются относительные пути файлов Форта. */
    const esp::sd_card &_sd_card;

    /** @brief Задача оболочки; @c nullptr, пока start() её не создал. */
    TaskHandle_t _task;

    /** @brief Движок Форта; создаётся в задаче оболочки (create_engine()). */
    std::unique_ptr<ff_t, void (*)(ff_t *)> _ff;

    /** @brief История строк: 8 КБ последних выполненных строк. */
    esp::vt::line_history _history;

    /** @brief Файл истории на SD-карте; без карты история живёт только в памяти. */
    esp::shell::history_file _history_file;

    /** @brief Редактор строки: через него набирается каждая строка. */
    esp::vt::line_edit _editor;

    /** @brief История строк, которые набирали для слова @c accept, — своя, не история оболочки. */
    esp::vt::line_history _input_history;

    /** @brief Редактор строки для слова @c accept. */
    esp::vt::line_edit _input_editor;

    /** @brief Приглашение для редактора строки: приглашение ff в цвете и пробел. */
    std::array<char, PROMPT_CAPACITY> _prompt;

    /** @brief Длина @c _prompt, байты. */
    std::size_t _prompt_length;

    /** @brief Ширина приглашения на экране, знакоместа: по ней ставится знак @c ^ под ошибкой. */
    int _prompt_width;

    /** @brief Строка для ff_eval() — набранная строка и нулевой байт после неё. */
    std::array<char, esp::vt::MAX_LINE_BYTES + 1> _source;

    /** @brief Выводил ли что-нибудь Форт после того, как строку набрали. */
    bool _has_output;

    /** @brief Когда задача оболочки последний раз отдавала процессор, мкс от запуска. */
    std::int64_t _last_yield_us;

    /**
     * @brief Выполняется ли код Форта: тогда Ctrl+C и K1 прерывают его (раздел 7.6).
     *
     * Его читают задачи ввода и главная задача — в interrupt().
     */
    std::atomic<bool> _is_running;

    /** @brief Прерван ли выполняющийся код: ждущие слова, получив это, сразу возвращаются. */
    std::atomic<bool> _is_interrupted;

    /** @brief Идёт ли проверка платы; его читает и главная задача — в button_pressed(). */
    std::atomic<bool> _is_board_test_running;

    /** @brief Нажата ли K1 во время проверки платы: нажатие ждёт, пока его заберёт board_test(). */
    std::atomic<bool> _is_button_pressed;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает терминал оболочки — для слов Форта.
 * @return Терминал.
 */
esp::vt::terminal &esp::shell::forth_shell::terminal()
{
    return _terminal;
}

/**
 * @brief Сообщает, прерван ли выполняющийся код Форта — для ждущих слов.
 * @return @c true, если с начала строки пришли Ctrl+C или нажатие K1.
 */
bool esp::shell::forth_shell::is_interrupted() const
{
    return _is_interrupted;
}
