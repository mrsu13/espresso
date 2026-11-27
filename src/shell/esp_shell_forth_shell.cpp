/**
 * @file esp_shell_forth_shell.h
 * @brief Объявление оболочки Форта esp::shell::forth_shell.
 */

/**
 * @file esp_shell_forth_shell.cpp
 * @brief Оболочка Форта: движок ff на терминале, редактор строки и история строк.
 */

#include "esp_shell_forth_shell.h"

#include "esp_board_test.h"
#include "esp_core_usage.h"
#include "esp_shell_terminal_words.h"
#include "esp_vt_controls.h"
#include "esp_vt_display.h"
#include "esp_vt_key_event.h"
#include "esp_vt_utf8.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <initializer_list>
#include <optional>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>


/**
 * @class esp::shell::forth_shell
 * @brief Оболочка Форта: движок ff на терминале (раздел 7 документа об устройстве).
 *
 * Работает в своей задаче. Создаёт движок ff, выводит его заставку и
 * повторяет: приглашение ff, строка из редактора строки, выполнение
 * строки ff_eval(). Приглашение показывает, что определение или
 * комментарий ещё не закрыты: «▶» — обычное, «:▶» — внутри определения,
 * «(▶» — внутри комментария; так определение пишется в несколько строк.
 * Приглашение всегда начинается с начала строки экрана: если вывод
 * кончился не переводом строки, оболочка его добавляет.
 *
 * Вывод Форта идёт в терминал, предупреждения ff — ярко-жёлтым. При
 * ошибке оболочка ставит знак @c ^ под словом, на котором она случилась,
 * и выводит сообщение ff красным (mark_error()). Выполненные строки
 * попадают в историю строк и в файл истории на SD-карте. Временные строки
 * ff оболочка освобождает после каждой строки, набранной вне определения,
 * если стек пуст (раздел 7.3).
 *
 * Ctrl+C и короткое нажатие K1 прерывают выполняющийся код Форта
 * (interrupt(), раздел 7.6): ff останавливается на ближайшем переходе или
 * вызове слова, а слова терминала, которые ждут, замечают прерывание не
 * позже чем через @c esp::shell::p::WAIT_STEP_MS. Клавиши, нажатые, пока
 * код выполнялся, терминал при этом сбрасывает, как терминал Unix. О
 * прерывании оболочка сообщает ярко-жёлтым. Пока код не выполняется,
 * Ctrl+C — обычная клавиша: редактор строки по ней отменяет строку.
 *
 * Долгий код Форта раз в @c esp::shell::p::YIELD_PERIOD_US отдаёт
 * процессор, чтобы не сработал сторожевой таймер задач (раздел 5.3).
 * Относительные пути файлов Форта отсчитываются от корня SD-карты. Память
 * слов и строк Форта ограничена @c esp::shell::p::MEMORY_LIMIT.
 *
 * Слова терминала (раздел 7.5) добавляет в словарь ff
 * esp::shell::terminal_words; им оболочка даёт терминал, вывод, чтение
 * строки (@c accept), смену видеорежима и проверку платы. Слово @c accept
 * набирает строку своим редактором строки со своей историей, начиная с
 * места курсора — после вопроса, который вывела программа.
 */


namespace esp::shell::p
{

/**
 * @brief Размер стека задачи оболочки, байты.
 *
 * Внутренний интерпретатор ff, вложенные @c evaluate и @c load, а ещё
 * проверка платы и файл истории работают на этом стеке; размер
 * предварительный, его уточнит замер (раздел 14 документа об
 * устройстве).
 */
constexpr int TASK_STACK = 16384;

/** @brief Приоритет задачи оболочки — ниже, чем у задач ввода. */
constexpr int TASK_PRIORITY = 3;

/**
 * @brief Предел памяти слов и строк Форта (ff_platform_t::mem_limit), байты.
 *
 * Сверх него ff даёт ошибку −8, а не отнимает память у экрана и задач;
 * предварительно, уточнит замер.
 */
constexpr std::size_t MEMORY_LIMIT = 64 * 1024;

/** @brief Как часто долгий код Форта отдаёт процессор задачам простоя, мкс. */
constexpr std::int64_t YIELD_PERIOD_US = 100000;

/** @brief Период опроса проверки платы, мс. */
constexpr int POLL_PERIOD_MS = 10;

/**
 * @brief Шаг ожидания слов, которые ждут ввода или времени, мс.
 *
 * Между шагами слово проверяет, не прервано ли выполнение: прерывание
 * доходит до него не позже чем через шаг.
 */
constexpr int WAIT_STEP_MS = 50;

/** @brief Код исключения «прерывание пользователем» по стандарту Forth: Ctrl+C или K1. */
constexpr std::int64_t USER_INTERRUPT = -28;

/** @brief Сколько байтов строк помнит история слова @c accept. */
constexpr int INPUT_HISTORY_BYTES = 1024;

/** @brief Сколько строк помнит история слова @c accept. */
constexpr int INPUT_HISTORY_LINES = 32;

/** @brief Цвет приглашения — ярко-жёлтый. */
constexpr std::string_view PROMPT_COLOUR = "\x1b[93m";

/** @brief Цвет предупреждений ff — ярко-жёлтый: он есть во всех палитрах. */
constexpr std::string_view WARNING_COLOUR = "\x1b[93m";

/** @brief Цвет ошибок — красный: он есть и в 4-цветной палитре. */
constexpr std::string_view ERROR_COLOUR = "\x1b[31m";

/** @brief Возврат к цвету по умолчанию. */
constexpr std::string_view NORMAL_COLOUR = "\x1b[0m";

/** @brief Тег журнала модуля. */
const char *const TAG = "shell";

/**
 * @brief Возвращает сообщение о прерывании.
 * @param ff Движок: по нему выбирается язык сообщения.
 * @return Сообщение.
 */
const char *interrupt_message(ff_t *ff)
{
    return FF_TR(ff, FF_TR_EN("Interrupted."), FF_TR_RU("Прервано."));
}

/**
 * @brief Находит, с какого байта начинается знак текста.
 * @param text Текст UTF-8.
 * @param chars Сколько знаков пропустить с начала.
 * @return Смещение, байты; за концом текста — его длина.
 */
std::size_t char_offset(std::string_view text, int chars)
{
    std::size_t index = 0;
    for (int i = 0; i < chars && index < text.size(); ++i)
        index = esp::vt::utf8::next(text, index);
    return index;
}

}


// Открытые

/**
 * @brief Создаёт оболочку; движок и задачу запускает start().
 * @param terminal Терминал; должен быть запущен и жить дольше оболочки.
 * @param sd_card Карта памяти для файла истории и файлов Форта; должна жить дольше оболочки.
 */
esp::shell::forth_shell::forth_shell(esp::vt::terminal &terminal, const esp::sd_card &sd_card)
    : _terminal{terminal}
    , _sd_card{sd_card}
    , _task{nullptr}
    , _ff{nullptr, &ff_free}
    , _history{esp::vt::HISTORY_BYTES, esp::vt::HISTORY_LINES}
    , _history_file{sd_card}
    , _editor{terminal, _history}
    , _input_history{esp::shell::p::INPUT_HISTORY_BYTES, esp::shell::p::INPUT_HISTORY_LINES}
    , _input_editor{terminal, _input_history}
    , _prompt{}
    , _prompt_length{0}
    , _prompt_width{0}
    , _source{}
    , _has_output{false}
    , _last_yield_us{0}
    , _is_running{false}
    , _is_interrupted{false}
    , _is_board_test_running{false}
    , _is_button_pressed{false}
{
}

/**
 * @brief Запускает задачу оболочки.
 */
void esp::shell::forth_shell::start()
{
    const int core = esp::core_usage::quiet_core();
    xTaskCreatePinnedToCore(&esp::shell::forth_shell::task, "shell", esp::shell::p::TASK_STACK,
                            this, esp::shell::p::TASK_PRIORITY, &_task,
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
bool esp::shell::forth_shell::button_pressed()
{
    if (!_is_board_test_running)
        return false;
    _is_button_pressed = true;
    return true;
}

/**
 * @brief Прерывает выполняющийся код Форта: Ctrl+C или короткое нажатие K1.
 *
 * Вызывается терминалом (esp::vt::terminal::interrupt()) — из задач ввода
 * по Ctrl+C и из главной задачи по K1; принятое прерывание терминал
 * дополняет сбросом непрочитанного ввода. Метод просит ff остановиться
 * (ff_request_abort()) и отмечает прерывание: ждущие слова (@c key,
 * @c ekey, @c accept, @c ms, @c tone, @c board-test) замечают его не
 * позже чем через @c esp::shell::p::WAIT_STEP_MS и сразу возвращаются.
 * Пока код не выполняется, прерывать нечего, и Ctrl+C остаётся обычной
 * клавишей — в редакторе строки она отменяет строку.
 *
 * @return @c true, если выполнялся код Форта и прерывание принято.
 */
bool esp::shell::forth_shell::interrupt()
{
    if (!_is_running)
        return false;
    _is_interrupted = true;
    ff_request_abort(_ff.get());
    return true;
}

/**
 * @brief Прерывает код Форта из ждущего слова, которое заметило прерывание.
 *
 * Бросает исключение −28, прерывание пользователем по стандарту Forth.
 * Его, как и прерывание по ff_request_abort(), ff не даёт перехватить
 * никакому @c catch, и ff_eval() возвращает @c FF_ERR_ABORTED. Так
 * прерывание действует, даже если слово — последнее в строке и ff больше
 * нигде не проверил бы просьбу остановиться.
 */
void esp::shell::forth_shell::throw_interrupt()
{
    ff_throwf(_ff.get(), esp::shell::p::USER_INTERRUPT, "%s",
              esp::shell::p::interrupt_message(_ff.get()));
}

/**
 * @brief Выводит текст на терминал и отмечает, что после строки был вывод.
 *
 * Через него выводят и слова Форта: от отметки зависит, где встанет знак
 * @c ^ под ошибкой (mark_error()).
 *
 * @param text Байты потока вывода.
 */
void esp::shell::forth_shell::write(std::string_view text)
{
    _terminal.write(text);
    _has_output = true;
}

/**
 * @brief Ждёт байт ввода — для слова @c key.
 *
 * Ждёт шагами по @c esp::shell::p::WAIT_STEP_MS, чтобы заметить прерывание.
 * Байт, прочитанный уже после прерывания, — из сброшенного ввода, и слову
 * его не отдают.
 *
 * @return Байт 0–255 или @c -1, если выполнение прервали.
 */
int esp::shell::forth_shell::wait_byte()
{
    while (!_is_interrupted)
    {
        const int byte = _terminal.read(esp::shell::p::WAIT_STEP_MS);
        if (byte >= 0 && !_is_interrupted)
            return byte;
    }
    return -1;
}

/**
 * @brief Ждёт событие клавиатуры — для слов @c ekey и @c accept.
 *
 * Ждёт шагами по @c esp::shell::p::WAIT_STEP_MS, чтобы заметить прерывание.
 * Событие, прочитанное уже после прерывания, — из сброшенного ввода, и
 * слову его не отдают.
 *
 * @return Событие или ничего, если выполнение прервали.
 */
std::optional<esp::vt::key_event> esp::shell::forth_shell::wait_key()
{
    while (!_is_interrupted)
    {
        const std::optional<esp::vt::key_event> key =
            _terminal.read_key(esp::shell::p::WAIT_STEP_MS);
        if (key && !_is_interrupted)
            return key;
    }
    return std::nullopt;
}

/**
 * @brief Ждёт заданное время — для слов @c ms и @c tone.
 *
 * Ждёт шагами не длиннее @c esp::shell::p::WAIT_STEP_MS и возвращается
 * раньше, если выполнение прервали (is_interrupted()). Пока ждёт, процессор
 * достаётся другим задачам.
 *
 * @param duration_ms Сколько ждать, мс.
 */
void esp::shell::forth_shell::wait(int duration_ms)
{
    const std::int64_t end_us = esp_timer_get_time() + std::int64_t{duration_ms} * 1000;
    while (!_is_interrupted)
    {
        const std::int64_t left_us = end_us - esp_timer_get_time();
        if (left_us <= 0)
            return;
        const std::int64_t step_ms =
            std::min<std::int64_t>(left_us / 1000 + 1, esp::shell::p::WAIT_STEP_MS);
        vTaskDelay(pdMS_TO_TICKS(static_cast<std::uint32_t>(step_ms)));
    }
}

/**
 * @brief Читает строку для слова @c accept.
 *
 * Строка набирается своим редактором строки со своей историей, с того
 * места, где стоит курсор: после вопроса, который вывела программа. Если
 * курсор ждёт переноса после последнего столбца, строка начинается со
 * следующей строки экрана.
 *
 * Если выполнение прервали (Ctrl+C, K1), строка отменяется, как
 * отменяет её Ctrl+C: набранное остаётся на экране с @c ^C, курсор
 * уходит на новую строку.
 *
 * @return Набранная строка или ничего, если выполнение прервали; строка
 *         действительна до следующего вызова.
 */
std::optional<std::string_view> esp::shell::forth_shell::read_line()
{
    if (_terminal.is_wrap_pending())
        write("\n");
    _has_output = true;
    _input_editor.start(std::string_view{}, _terminal.columns(), _terminal.cursor_column());
    esp::vt::line_edit::result result = esp::vt::line_edit::result::editing;
    while (result == esp::vt::line_edit::result::editing)
    {
        const std::optional<esp::vt::key_event> key = wait_key();
        if (!key)
        {
            _input_editor.feed(esp::vt::key_event{esp::vt::ETX});
            return std::nullopt;
        }
        result = _input_editor.feed(*key);
    }
    if (result != esp::vt::line_edit::result::accepted)
        return std::nullopt;
    _input_history.add(_input_editor.line());
    return _input_editor.line();
}

/**
 * @brief Включает видеорежим терминала — для слова @c video-mode.
 *
 * Экран создаётся заново, его текст пропадает (esp::vt::terminal::set_mode()).
 *
 * @param mode Номер видеорежима в таблице esp::vt::display.
 * @return @c false, если такого видеорежима нет.
 */
bool esp::shell::forth_shell::set_video_mode(int mode)
{
    if (mode < 0 || mode >= esp::vt::display::mode_count())
        return false;
    _terminal.set_mode(mode);
    _has_output = true;
    return true;
}

/**
 * @brief Проводит проверку платы и ждёт, пока она закончится.
 *
 * Терминал на это время приостанавливается (esp::vt::terminal::suspend())
 * и отдаёт проверке монитор, клавиатуру PS/2 и генератор звука; вывод, в
 * том числе журнал проверки, идёт в модель экрана и в UART0. Задача
 * оболочки опрашивает проверку каждые @c esp::shell::p::POLL_PERIOD_MS и
 * передаёт ей нажатия K1 (button_pressed()). Проверка заканчивается по Esc
 * на клавиатуре PS/2 или по одиночному ESC с терминала ПК, за которым
 * @c esp::vt::ESCAPE_TIME_OUT_MS ничего не пришло; прочий ввод с UART0 на
 * это время пропускается. Затем терминал включает свой видеорежим и
 * рисует экран заново.
 */
void esp::shell::forth_shell::board_test()
{
    write(FF_TR(_ff.get(), FF_TR_EN("Board test. Press Esc to return to the shell.\n"),
                FF_TR_RU("Проверка платы. Esc — возврат в оболочку.\n")));
    _is_button_pressed = false;
    _is_board_test_running = true;
    _terminal.suspend();
    {
        // Проверка — в куче, а не на стеке оболочки: один видеоконтроллер
        // занимает около килобайта.
        const std::unique_ptr<esp::board_test> test =
            std::make_unique<esp::board_test>(_terminal.sound());
        while (!test->is_done() && !_is_interrupted)
        {
            // Ожидание ввода с UART0 — заодно и пауза между опросами.
            const std::optional<esp::vt::key_event> key =
                _terminal.read_key(esp::shell::p::POLL_PERIOD_MS);
            if (key && *key == esp::vt::key_event{esp::vt::ESC})
                break;
            if (_is_button_pressed.exchange(false))
                test->button_pressed();
            test->poll();
        }
    }
    _terminal.resume();
    _is_board_test_running = false;
}

// Закрытые

/**
 * @brief Задача оболочки.
 * @param arg Оболочка.
 */
void esp::shell::forth_shell::task(void *arg)
{
    static_cast<esp::shell::forth_shell *>(arg)->run();
}

/**
 * @brief Вывод ff (ff_platform_t::vprintf): форматирует текст и выводит его на терминал.
 * @param context Оболочка.
 * @param format Формат @c printf.
 * @param args Аргументы формата.
 * @return Длина текста, как у @c vprintf.
 */
int esp::shell::forth_shell::print_callback(void *context, const char *format, va_list args)
{
    return static_cast<esp::shell::forth_shell *>(context)->print(format, args);
}

/**
 * @brief Сообщения ff (ff_platform_t::vtracef): предупреждения и трассировка.
 *
 * Сообщение выводится с начала строки экрана и кончается переводом
 * строки; предупреждение — ярко-жёлтым и со словом «Warning:» на языке
 * движка. Ошибки ff сюда не передаёт: их возвращает ff_eval(), и их
 * выводит report_error().
 *
 * @param context Оболочка.
 * @param error Код сообщения с признаком важности (@c FF_SEV_*).
 * @param format Формат @c printf.
 * @param args Аргументы формата.
 * @return Длина текста, как у @c vprintf.
 */
int esp::shell::forth_shell::trace_callback(void *context, ff_error_t error, const char *format,
                                            va_list args)
{
    esp::shell::forth_shell &shell = *static_cast<esp::shell::forth_shell *>(context);
    shell.start_line();
    // Предупреждение может прийти и из ff_new(), когда движка у оболочки
    // ещё нет: тогда — по-английски.
    const char *const language = shell._ff ? ff_lang(shell._ff.get()) : nullptr;
    const bool is_warning = FF_ERR_SEV(error) == FF_SEV_WARNING;
    if (is_warning)
    {
        shell.write(esp::shell::p::WARNING_COLOUR);
        shell.write(FF_TR_LANG(language, FF_TR_EN("Warning: "), FF_TR_RU("Предупреждение: ")));
    }
    const int length = shell.print(format, args);
    if (is_warning)
        shell.write(esp::shell::p::NORMAL_COLOUR);
    shell.write("\n");
    return length;
}

/**
 * @brief Сторожевой таймер ff (ff_platform_t::watchdog): отдаёт процессор долгому коду.
 *
 * ff вызывает его через каждые 65 536 команд. Раз в
 * @c esp::shell::p::YIELD_PERIOD_US задача оболочки отдаёт процессор на
 * один такт планировщика: иначе долгий цикл Форта отнимал бы ядро у задачи
 * простоя, и сработал бы сторожевой таймер задач ESP-IDF (раздел 5.3
 * документа об устройстве).
 *
 * @param context Оболочка.
 * @param opcodes Сколько команд выполнено с начала вызова ff_eval().
 * @return Всегда @c FF_WD_CONTINUE: прерывает выполнение не он.
 */
ff_watchdog_action_t esp::shell::forth_shell::watchdog_callback(void *context,
                                                                std::uint64_t opcodes)
{
    static_cast<void>(opcodes);
    esp::shell::forth_shell &shell = *static_cast<esp::shell::forth_shell *>(context);
    if (esp_timer_get_time() - shell._last_yield_us >= esp::shell::p::YIELD_PERIOD_US)
    {
        vTaskDelay(1);
        shell._last_yield_us = esp_timer_get_time();
    }
    // Просьба ff_request_abort() могла прийти между вызовами и пропасть:
    // отметка прерывания её повторяет.
    return shell._is_interrupted ? FF_WD_ABORT : FF_WD_CONTINUE;
}

/**
 * @brief Открывает файл для Форта (ff_platform_t::open_file).
 *
 * Относительный путь отсчитывается от корня SD-карты; без карты такой
 * файл не откроется, и Форт получит ошибку.
 *
 * @param context Оболочка.
 * @param path Путь, как его дала программа.
 * @param mode Режим @c fopen().
 * @return Открытый файл или @c nullptr, если открыть не удалось (@c errno — почему).
 */
std::FILE *esp::shell::forth_shell::open_file_callback(void *context, const char *path,
                                                       const char *mode)
{
    const esp::shell::forth_shell &shell = *static_cast<esp::shell::forth_shell *>(context);
    if (path[0] == '/')
        return std::fopen(path, mode);

    std::array<char, 256> full_path{};
    const int length = std::snprintf(full_path.data(), full_path.size(), "%s/%s",
                                     shell._sd_card.mount_point(), path);
    if (length < 0 || static_cast<std::size_t>(length) >= full_path.size())
    {
        errno = ENAMETOOLONG;
        return nullptr;
    }
    return std::fopen(full_path.data(), mode);
}

/**
 * @brief Цикл оболочки: приглашение, строка, выполнение. Не возвращает управление.
 *
 * Сначала читается история строк из файла на SD-карте и создаётся
 * движок; затем оболочка становится получателем прерывания терминала
 * (interrupt()). Выполненная строка попадает в историю и в файл,
 * отменённая (Ctrl+C) — никуда. Если на движок не хватило памяти,
 * оболочка сообщает об этом, и её задача кончается.
 */
void esp::shell::forth_shell::run()
{
    _history_file.load(_history);
    if (!create_engine())
    {
        write(esp::shell::p::ERROR_COLOUR);
        write("Not enough memory for the Forth engine.");
        write(esp::shell::p::NORMAL_COLOUR);
        write("\n");
        _task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    _terminal.set_interrupt_handler(this);

    write(ff_banner(_ff.get()));
    write("\n");
    std::array<char, 160> text{};
    std::snprintf(text.data(), text.size(),
                  FF_TR(_ff.get(),
                        FF_TR_EN("ff %s. \x1b[1mwords\x1b[0m lists the words, "
                                 "\x1b[1mman dup\x1b[0m describes dup.\n"),
                        FF_TR_RU("ff %s. \x1b[1mwords\x1b[0m выводит слова, "
                                 "\x1b[1mman dup\x1b[0m описывает dup.\n")),
                  ff_version());
    write(text.data());

    while (true)
    {
        start_line();
        update_prompt();
        _editor.start(std::string_view{_prompt.data(), _prompt_length}, _terminal.columns());
        esp::vt::line_edit::result result = esp::vt::line_edit::result::editing;
        while (result == esp::vt::line_edit::result::editing)
        {
            if (const std::optional<esp::vt::key_event> key = _terminal.read_key(-1))
                result = _editor.feed(*key);
        }
        if (result != esp::vt::line_edit::result::accepted)
            continue;
        const std::string_view line = _editor.line();
        if (_history.add(line))
            _history_file.append(line);
        evaluate(line);
    }
}

/**
 * @brief Создаёт движок ff и добавляет в его словарь слова оболочки.
 * @return @c false, если на движок не хватило памяти.
 */
bool esp::shell::forth_shell::create_engine()
{
    ff_platform_t platform{};
    platform.context = this;
    platform.vprintf = &esp::shell::forth_shell::print_callback;
    platform.vtracef = &esp::shell::forth_shell::trace_callback;
    platform.watchdog = &esp::shell::forth_shell::watchdog_callback;
    platform.mem_limit = esp::shell::p::MEMORY_LIMIT;
    platform.open_file = &esp::shell::forth_shell::open_file_callback;
    _ff.reset(ff_new(&platform));
    if (!_ff)
        return false;
    if (esp::shell::terminal_words::register_words(_ff.get()) != FF_OK)
        ESP_LOGW(esp::shell::p::TAG, "Shell words not registered: %s.", ff_strerror(_ff.get()));
    ESP_LOGD(esp::shell::p::TAG, "Forth engine created; free memory %zu kB.",
             heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    return true;
}

/**
 * @brief Выполняет строку и сообщает об ошибке, если она была.
 *
 * После строки вне определения, если стек пуст, освобождаются временные
 * строки ff: тогда ни на стеке, ни в незаконченном определении их адресов
 * нет (раздел 7.3 документа об устройстве).
 *
 * @param line Набранная строка.
 */
void esp::shell::forth_shell::evaluate(std::string_view line)
{
    std::copy(line.begin(), line.end(), _source.begin());
    _source[line.size()] = '\0';
    // Своя копия строки: слова, которые сами читают строку редактором,
    // меняют его строку, пока выполняется эта.
    const std::string_view source{_source.data(), line.size()};
    _has_output = false;
    _last_yield_us = esp_timer_get_time();
    _is_interrupted = false;
    _is_running = true;
    const ff_error_t error = ff_eval(_ff.get(), _source.data());
    _is_running = false;
    if (error != FF_OK)
        report_error(source, error);
    if (is_top_level() && ff_depth(_ff.get()) == 0)
        ff_release_strings(_ff.get());
}

/**
 * @brief Сообщает об ошибке: знак @c ^ под её местом и сообщение ff красным.
 *
 * Место отмечается, только если ошибка случилась в самой строке; ошибку в
 * загружаемом файле описывает сообщение. Прерывание (Ctrl+C, K1) — не
 * ошибка: о нём одно и то же ярко-жёлтое сообщение, чем бы ни был занят
 * код, — ждал ли он в слове терминала или считал.
 *
 * @param line Строка, на которой случилась ошибка.
 * @param error Код ошибки из ff_eval().
 */
void esp::shell::forth_shell::report_error(std::string_view line, ff_error_t error)
{
    const int position = ff_err_pos(_ff.get());
    if (ff_err_line(_ff.get()) == 1 && position >= 0
        && static_cast<std::size_t>(position) <= line.size())
        mark_error(line, static_cast<std::size_t>(position));
    start_line();
    // После ff_request_abort() у ff своё сообщение, с числом выполненных
    // операций, а у ждущих слов — своё; для пользователя это одно прерывание.
    if (_is_interrupted && error == FF_ERR_ABORTED)
    {
        write(esp::shell::p::WARNING_COLOUR);
        write(esp::shell::p::interrupt_message(_ff.get()));
    }
    else
    {
        write(esp::shell::p::ERROR_COLOUR);
        write(FF_TR(_ff.get(), FF_TR_EN("Error: "), FF_TR_RU("Ошибка: ")));
        write(ff_strerror(_ff.get()));
    }
    write(esp::shell::p::NORMAL_COLOUR);
    write("\n");
}

/**
 * @brief Ставит знак @c ^ под словом, на котором случилась ошибка.
 *
 * Если после строки ничего не выведено и она уместилась в одну строку
 * экрана, знак встаёт прямо под ней — с учётом приглашения. Иначе строка
 * выводится заново, и знак встаёт под ней: над курсором уже другой
 * вывод или конец перенесённой строки. Длинная строка выводится не вся, а
 * столько, сколько помещается в строку экрана вокруг ошибки.
 *
 * @param line Строка.
 * @param position Где в строке слово с ошибкой, байты.
 */
void esp::shell::forth_shell::mark_error(std::string_view line, std::size_t position)
{
    const int columns = _terminal.columns();
    const int length = esp::vt::utf8::length(line);
    int column = esp::vt::utf8::length(line.substr(0, position));

    if (!_has_output && _prompt_width + length <= columns)
        column += _prompt_width;
    else
    {
        // Окно строки шириной в строку экрана без одного столбца — так
        // строка не переносится, — и ошибка поближе к его середине.
        const int width = columns - 1;
        const int first = length <= width ? 0 : std::clamp(column - width / 2, 0, length - width);
        const std::size_t begin = esp::shell::p::char_offset(line, first);
        const std::size_t end = esp::shell::p::char_offset(line, first + width);
        start_line();
        write(line.substr(begin, end - begin));
        write("\n");
        column -= first;
    }

    std::array<char, 24> marker{};
    // CUF с параметром 0 сдвигает на один столбец, поэтому без сдвига — без CUF.
    if (column > 0)
        std::snprintf(marker.data(), marker.size(), "\x1b[%dC^\n", column);
    else
        std::snprintf(marker.data(), marker.size(), "^\n");
    write(marker.data());
}

/**
 * @brief Сообщает, ждёт ли ff новую строку вне определения и комментария.
 * @return @c true, если приглашение ff — обычное «▶», а не «:▶» и не «(▶».
 */
bool esp::shell::forth_shell::is_top_level() const
{
    const char first = ff_prompt(_ff.get())[0];
    return first != ':' && first != '(';
}

/**
 * @brief Собирает приглашение для редактора строки из приглашения ff.
 */
void esp::shell::forth_shell::update_prompt()
{
    const std::string_view prompt = ff_prompt(_ff.get());
    std::size_t length = 0;
    for (const std::string_view part : {esp::shell::p::PROMPT_COLOUR, prompt,
                                        esp::shell::p::NORMAL_COLOUR, std::string_view{" "}})
    {
        const std::size_t size = std::min(part.size(), _prompt.size() - length);
        std::copy_n(part.data(), size, _prompt.data() + length);
        length += size;
    }
    _prompt_length = length;
    _prompt_width = esp::vt::utf8::length(prompt) + 1;
}

/**
 * @brief Форматирует текст и выводит его на терминал.
 *
 * Короткий текст форматируется на стеке, длинный — во временном буфере в
 * куче: вывод @c man или @c words бывает длиннее строки.
 *
 * @param format Формат @c printf.
 * @param args Аргументы формата; не расходуются.
 * @return Длина текста, как у @c vprintf, или отрицательное число при ошибке формата.
 */
int esp::shell::forth_shell::print(const char *format, va_list args)
{
    std::array<char, 256> buffer{};
    va_list copy;
    va_copy(copy, args);
    const int length = std::vsnprintf(buffer.data(), buffer.size(), format, copy);
    va_end(copy);
    if (length < 0)
        return length;

    const std::size_t size = static_cast<std::size_t>(length);
    if (size < buffer.size())
    {
        write(std::string_view{buffer.data(), size});
        return length;
    }
    const std::unique_ptr<char[]> text = std::make_unique<char[]>(size + 1);
    va_copy(copy, args);
    std::vsnprintf(text.get(), size + 1, format, copy);
    va_end(copy);
    write(std::string_view{text.get(), size});
    return length;
}

/**
 * @brief Переводит строку, если курсор не в начале строки экрана.
 */
void esp::shell::forth_shell::start_line()
{
    if (_terminal.cursor_column() != 0)
        write("\n");
}
