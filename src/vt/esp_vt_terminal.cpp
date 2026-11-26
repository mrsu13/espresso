/**
 * @file esp_vt_terminal.h
 * @brief Объявление терминала esp::vt::terminal.
 */

/**
 * @file esp_vt_terminal.cpp
 * @brief Терминал: экран на VGA, ввод с клавиатуры PS/2 и UART0, копия вывода в UART0.
 */

#include "esp_vt_terminal.h"

#include "esp_core_usage.h"
#include "esp_ps2_keyboard.h"
#include "esp_ps2_keyboard_layouts.h"
#include "esp_ps2_ps2_controller.h"
#include "esp_ps2_virtual_key.h"
#include "esp_sound_square_waveform_generator.h"
#include "esp_vt_keys.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>

#include <driver/uart.h>
#include <driver/uart_vfs.h>
#include <esp_err.h>


/**
 * @class esp::vt::terminal
 * @brief Терминал: экран на мониторе VGA, ввод с клавиатуры PS/2 и UART0, копия вывода в UART0.
 *
 * Всё, что выводят программы, — поток байтов UTF-8 с последовательностями
 * VT220 (раздел 6.2 документа об устройстве). write() заменяет в нём
 * одиночный LF на CR LF, передаёт его разборщику, а тот — экрану, и
 * дословно повторяет его в UART0, так что терминал ПК показывает то же,
 * что монитор. Затем рисуются изменения экрана.
 *
 * Ввод — один поток байтов, как от терминала VT220: нажатия клавиш PS/2
 * задача ввода переводит в те же байты, что шлёт терминал ПК
 * (esp::vt::key_sequence()), а байты с UART0 принимает другая задача; обе
 * кладут их в одну очередь, которую читает read(), а read_key() собирает из
 * этих байтов события клавиатуры esp::vt::key_event. Shift+PgUp и Shift+PgDn
 * на основном экране листают историю экрана и в ввод не попадают; любая
 * другая клавиша возвращает показ к текущим строкам.
 *
 * Курсор мигает по таймеру FreeRTOS. Звонок BEL подаёт генератор звука.
 * Журнал ESP-IDF после begin() идёт через терминал, поэтому его строки не
 * сбивают экран терминала ПК.
 *
 * suspend() отдаёт монитор, клавиатуру PS/2 и генератор звука другой
 * программе — проверке платы: видеоконтроллер терминала останавливается и
 * освобождает буфер кадра, а задача клавиатуры перестаёт брать клавиши.
 * Вывод тем временем идёт в модель экрана и в UART0, ввод с UART0 — в
 * очередь ввода. resume() включает видеорежим терминала снова и рисует
 * экран целиком.
 */


namespace esp::vt::p
{

/**
 * @brief Держит мьютекс, пока жив объект.
 */
class guard : esp::non_copyable
{

public:

    explicit guard(SemaphoreHandle_t mutex);
    ~guard();

private:

    /** @brief Захваченный мьютекс. */
    SemaphoreHandle_t _mutex;
};

/** @brief Последовательный порт терминала — UART0, тот же, что у консоли ESP-IDF. */
constexpr uart_port_t UART_PORT = UART_NUM_0;

/** @brief Терминал, через который идёт журнал ESP-IDF; @c nullptr — журнал идёт мимо. */
esp::vt::terminal *log_terminal = nullptr;

/** @brief Тег журнала модуля. */
const char *const TAG = "vt";

}


// Открытые

/**
 * @brief Создаёт терминал; вывод, ввод и задачи запускает begin().
 */
esp::vt::terminal::terminal()
    : _display{}
    , _mode{-1}
    , _screen{}
    , _parser{}
    , _renderer{}
    , _input_parser{}
    , _sound{}
    , _lock{nullptr}
    , _input{nullptr}
    , _blink_timer{nullptr}
    , _keyboard_task{nullptr}
    , _keyboard_released{nullptr}
    , _uart_task{nullptr}
    , _previous_log{nullptr}
    , _log_line{}
    , _is_suspended{false}
    , _is_cursor_on{true}
    , _last_byte{0}
{
}

/**
 * @brief Останавливает терминал: журнал возвращается к прежнему выводу, задачи и таймер удаляются.
 *
 * Приложение живёт, пока работает плата, и на деле терминал не уничтожается.
 */
esp::vt::terminal::~terminal()
{
    if (esp::vt::p::log_terminal == this)
    {
        esp_log_set_vprintf(_previous_log);
        esp::vt::p::log_terminal = nullptr;
    }
    if (_blink_timer)
        xTimerDelete(_blink_timer, portMAX_DELAY);
    if (_keyboard_task)
        vTaskDelete(_keyboard_task);
    if (_uart_task)
    {
        vTaskDelete(_uart_task);
        uart_driver_delete(esp::vt::p::UART_PORT);
    }
    if (_keyboard_released)
        vSemaphoreDelete(_keyboard_released);
    if (_input)
        vQueueDelete(_input);
    if (_lock)
        vSemaphoreDelete(_lock);
}

/**
 * @brief Запускает терминал.
 *
 * Включает видеорежим @c esp::vt::DEFAULT_MODE, контроллер PS/2 (клавиатура
 * на порту 0, раскладка US), драйвер UART0, задачи ввода, мигание курсора
 * и вывод журнала ESP-IDF через терминал. Без чего терминал работать не
 * может, проверяется @c ESP_ERROR_CHECK.
 */
void esp::vt::terminal::begin()
{
    _lock = xSemaphoreCreateMutex();
    _input = xQueueCreate(esp::vt::INPUT_QUEUE_LENGTH, 1);
    _keyboard_released = xSemaphoreCreateBinary();
    assert(_lock && _input && _keyboard_released);

    set_mode(esp::vt::DEFAULT_MODE);

    esp::ps2::ps2_controller::begin(esp::ps2::ps2_preset::keyboard_port_0_mouse_port_1);
    esp::ps2::ps2_controller::keyboard()->set_layout(&esp::ps2::US_LAYOUT);

    // С драйвером UART принимает байты по прерыванию, а консоль ESP-IDF
    // пишет через тот же драйвер, а не в обход него.
    ESP_ERROR_CHECK(uart_driver_install(esp::vt::p::UART_PORT, esp::vt::UART_RX_BUFFER,
                                        esp::vt::UART_TX_BUFFER, 0, nullptr, 0));
    uart_vfs_dev_use_driver(esp::vt::p::UART_PORT);

    const int core = esp::core_usage::quiet_core();
    const BaseType_t affinity = core >= 0 ? core : tskNO_AFFINITY;
    xTaskCreatePinnedToCore(&esp::vt::terminal::keyboard_task, "vt_keyboard",
                            esp::vt::KEYBOARD_TASK_STACK, this, esp::vt::INPUT_TASK_PRIORITY,
                            &_keyboard_task, affinity);
    xTaskCreatePinnedToCore(&esp::vt::terminal::uart_task, "vt_uart", esp::vt::UART_TASK_STACK,
                            this, esp::vt::INPUT_TASK_PRIORITY, &_uart_task, affinity);
    assert(_keyboard_task && _uart_task);

    _blink_timer = xTimerCreate("vt_blink", pdMS_TO_TICKS(esp::vt::BLINK_PERIOD_MS), pdTRUE, this,
                                &esp::vt::terminal::blink);
    assert(_blink_timer);
    xTimerStart(_blink_timer, portMAX_DELAY);

    esp::vt::p::log_terminal = this;
    _previous_log = esp_log_set_vprintf(&esp::vt::terminal::log_vprintf);
}

/**
 * @brief Выводит текст на монитор и в UART0.
 *
 * Одиночный LF заменяется на CR LF, как в терминале Unix: модель экрана,
 * как VT220, по LF только переводит строку.
 *
 * @param text Байты UTF-8 с последовательностями VT220.
 */
void esp::vt::terminal::write(std::string_view text)
{
    const esp::vt::p::guard guard{_lock};
    put(text);
}

/**
 * @brief Читает байт ввода.
 * @param time_out_ms Сколько ждать, мс; меньше 0 — сколько потребуется.
 * @return Байт 0–255 или @c -1, если время вышло.
 */
int esp::vt::terminal::read(int time_out_ms)
{
    std::uint8_t byte = 0;
    const TickType_t ticks = time_out_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(time_out_ms);
    return xQueueReceive(_input, &byte, ticks) == pdTRUE ? byte : -1;
}

/**
 * @brief Читает событие клавиатуры: знак или особую клавишу.
 *
 * Байты ввода собирает разборщик esp::vt::input_parser, так что знак UTF-8
 * или последовательность особой клавиши — одно событие (раздел 6.3
 * документа об устройстве). Продолжения начатой последовательности метод
 * ждёт не дольше @c esp::vt::ESCAPE_TIME_OUT_MS: ESC без продолжения —
 * клавиша Esc. Поэтому ждать он может и дольше @p time_out_ms. События
 * читает одна задача; байты read() между вызовами read_key() читать можно.
 *
 * @param time_out_ms Сколько ждать первого байта, мс; меньше 0 — сколько потребуется.
 * @return Событие или ничего, если время вышло.
 */
std::optional<esp::vt::key_event> esp::vt::terminal::read_key(int time_out_ms)
{
    while (true)
    {
        const bool is_pending = _input_parser.is_pending();
        const int byte = read(is_pending ? esp::vt::ESCAPE_TIME_OUT_MS : time_out_ms);
        if (byte < 0)
        {
            if (!is_pending)
                return std::nullopt;
            // Продолжения нет; если и события нет, ждать дальше.
            if (const std::optional<esp::vt::key_event> key = _input_parser.time_out())
                return key;
            continue;
        }
        if (const std::optional<esp::vt::key_event> key =
                _input_parser.feed(static_cast<std::uint8_t>(byte)))
            return key;
    }
}

/**
 * @brief Включает видеорежим.
 *
 * Экран создаётся заново под новый размер: прежний текст и история экрана
 * пропадают. Пока терминал приостановлен, видеорежим не меняется.
 *
 * @param mode Номер видеорежима в таблице esp::vt::display.
 */
void esp::vt::terminal::set_mode(int mode)
{
    const esp::vt::p::guard guard{_lock};
    assert(!_is_suspended);
    // Сначала освобождается память прежнего экрана, затем буфер кадра:
    // так новому видеорежиму её достаётся больше.
    _renderer.reset();
    _parser.reset();
    _screen.reset();
    _display.set_mode(mode);
    _mode = mode;
    start_screen();
}

/**
 * @brief Возвращает ширину экрана.
 * @return Число столбцов.
 */
int esp::vt::terminal::columns()
{
    const esp::vt::p::guard guard{_lock};
    return _screen->columns();
}

/**
 * @brief Возвращает высоту экрана.
 * @return Число строк.
 */
int esp::vt::terminal::rows()
{
    const esp::vt::p::guard guard{_lock};
    return _screen->rows();
}

/**
 * @brief Заново ищет клавиатуру PS/2, если её не было.
 *
 * Сброс клавиатуры занимает до двух секунд.
 *
 * @return @c true, если клавиатура есть.
 */
bool esp::vt::terminal::check_keyboard()
{
    esp::ps2::keyboard *const keyboard = esp::ps2::ps2_controller::keyboard();
    if (keyboard->is_keyboard_available())
        return true;
    const bool is_found = keyboard->reset();
    // Сброс ставит раскладку US; здесь она та же, но задаётся явно.
    keyboard->set_layout(&esp::ps2::US_LAYOUT);
    ESP_LOGI(esp::vt::p::TAG, "Keyboard %s.", is_found ? "found" : "not found");
    return is_found;
}

/**
 * @brief Приостанавливает терминал: отдаёт монитор, клавиатуру PS/2 и генератор звука другой программе.
 *
 * Видеоконтроллер терминала останавливается и уничтожается, освобождая
 * буфер кадра: двум буферам кадра памяти не хватит. Мигание курсора
 * останавливается, задача клавиатуры перестаёт брать клавиши из очереди
 * клавиатуры, и их читает другая программа; метод возвращает управление,
 * когда задача клавиатуры остановилась, — не позже чем через
 * @c esp::vt::KEYBOARD_WAIT_MS. Генератором звука другая программа
 * пользуется через sound(): терминал звонок не подаёт.
 *
 * Модель экрана, история экрана и задачи остаются: вывод идёт в модель
 * экрана и в UART0, ввод с UART0 — в очередь ввода. Экран рисует заново
 * resume().
 */
void esp::vt::terminal::suspend()
{
    {
        const esp::vt::p::guard guard{_lock};
        assert(!_is_suspended);
        _is_suspended = true;
        xTimerStop(_blink_timer, portMAX_DELAY);
        // Рисование ссылается на холст, а холст уничтожается вместе с
        // видеоконтроллером.
        _renderer.reset();
        _display.stop();
    }
    // Ждать без мьютекса: задаче клавиатуры он может понадобиться, чтобы
    // обработать уже взятую клавишу. Клавиши, взятые до её ответа, — ещё
    // для терминала.
    xSemaphoreTake(_keyboard_released, portMAX_DELAY);
}

/**
 * @brief Продолжает работу терминала после suspend().
 *
 * Включает видеорежим терминала и рисует экран целиком по модели экрана —
 * с тем, что было выведено за время приостановки. Если буфер кадра
 * поместился не целиком и строк стало меньше (видеоконтроллер уменьшает
 * высоту изображения, когда памяти не хватает), экран создаётся заново,
 * как при смене видеорежима. Затем снова мигает курсор, и задача
 * клавиатуры берёт клавиши.
 */
void esp::vt::terminal::resume()
{
    {
        const esp::vt::p::guard guard{_lock};
        assert(_is_suspended);
        _display.set_mode(_mode);
        _is_suspended = false;
        _is_cursor_on = true;
        start_screen();
        xTimerStart(_blink_timer, portMAX_DELAY);
    }
    xTaskNotifyGive(_keyboard_task);
}


// Закрытые

/**
 * @brief Задача клавиатуры: переводит нажатия клавиш PS/2 в байты ввода.
 *
 * Клавишу задача ждёт не дольше @c esp::vt::KEYBOARD_WAIT_MS, а затем
 * проверяет, не приостановлен ли терминал. Если приостановлен, она
 * сообщает об этом suspend() — клавиатура свободна — и ждёт уведомления
 * от resume(), не трогая клавиш: их читает программа, которой отдана
 * клавиатура. Задача останавливается в начале цикла, не держа ни мьютекса,
 * ни клавиши.
 *
 * @param arg Терминал.
 */
void esp::vt::terminal::keyboard_task(void *arg)
{
    esp::vt::terminal &terminal = *static_cast<esp::vt::terminal *>(arg);
    esp::ps2::keyboard *const keyboard = esp::ps2::ps2_controller::keyboard();
    esp::ps2::virtual_key_item item;
    while (true)
    {
        if (terminal._is_suspended)
        {
            xSemaphoreGive(terminal._keyboard_released);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }
        if (keyboard->get_next_virtual_key(&item, esp::vt::KEYBOARD_WAIT_MS) && item.down)
            terminal.on_key(item);
    }
}

/**
 * @brief Задача UART0: кладёт принятые байты в ввод.
 *
 * Байты принимаются по одному: ввод с клавиатуры ПК редкий, а пачка байтов
 * вставленного текста проходит тем же путём без задержек. Если очередь
 * ввода полна, задача ждёт места: байты тем временем копятся в буфере
 * драйвера (@c esp::vt::UART_RX_BUFFER), так что вставленный текст не
 * теряется, пока помещается в нём.
 *
 * @param arg Терминал.
 */
void esp::vt::terminal::uart_task(void *arg)
{
    esp::vt::terminal &terminal = *static_cast<esp::vt::terminal *>(arg);
    while (true)
    {
        char byte = 0;
        if (uart_read_bytes(esp::vt::p::UART_PORT, &byte, 1, portMAX_DELAY) != 1)
            continue;
        terminal.show_current_lines();
        xQueueSend(terminal._input, &byte, portMAX_DELAY);
    }
}

/**
 * @brief Мигает курсором: вызывается таймером FreeRTOS.
 *
 * Если терминал занят выводом, мигание пропускается до следующего раза:
 * задача таймеров FreeRTOS одна на всех и ждать не должна. Приостановленный
 * терминал не рисует: таймер мог сработать, пока suspend() его
 * останавливал.
 *
 * @param timer Таймер; его идентификатор — терминал.
 */
void esp::vt::terminal::blink(TimerHandle_t timer)
{
    esp::vt::terminal &terminal = *static_cast<esp::vt::terminal *>(pvTimerGetTimerID(timer));
    if (xSemaphoreTake(terminal._lock, 0) != pdTRUE)
        return;
    if (!terminal._is_suspended)
    {
        terminal._is_cursor_on = !terminal._is_cursor_on;
        terminal._renderer->draw(*terminal._screen, terminal._is_cursor_on);
    }
    xSemaphoreGive(terminal._lock);
}

/**
 * @brief Выводит строку журнала ESP-IDF через терминал.
 *
 * Если журнал пишет задача, которая уже держит мьютекс терминала, — то
 * есть изнутри вывода, — строка уходит прежним путём, прямо в UART0:
 * иначе она вклинилась бы в середину выводимой последовательности.
 * Слишком длинная строка обрезается, но конец строки у неё остаётся.
 *
 * @param format Формат @c printf.
 * @param args Аргументы формата.
 * @return Длина строки, как у @c vprintf.
 */
int esp::vt::terminal::log_vprintf(const char *format, va_list args)
{
    esp::vt::terminal *const terminal = esp::vt::p::log_terminal;
    if (!terminal || xSemaphoreGetMutexHolder(terminal->_lock) == xTaskGetCurrentTaskHandle())
        return terminal && terminal->_previous_log ? terminal->_previous_log(format, args)
                                                   : std::vprintf(format, args);

    const esp::vt::p::guard guard{terminal->_lock};
    std::array<char, esp::vt::LOG_LINE_MAX> &line = terminal->_log_line;
    const int length = std::vsnprintf(line.data(), line.size(), format, args);
    if (length < 0)
        return length;
    std::size_t size = std::min(static_cast<std::size_t>(length), line.size() - 1);
    if (static_cast<std::size_t>(length) >= line.size())
        line[size - 1] = '\n';
    terminal->put(std::string_view{line.data(), size});
    return length;
}

/**
 * @brief Готовит экран к рисованию на только что включённом видеорежиме; мьютекс уже захвачен.
 *
 * Экран и разборщик создаются заново, если их нет или размер экрана не
 * совпадает с размером видеорежима; иначе экран остаётся со всем текстом.
 * Рисование создаётся для нового холста и рисует экран целиком.
 */
void esp::vt::terminal::start_screen()
{
    if (!_screen || _screen->columns() != _display.columns() || _screen->rows() != _display.rows())
    {
        _parser.reset();
        _screen.reset();
        _screen.emplace(_display.columns(), _display.rows());
        _parser.emplace(*_screen);
        _last_byte = 0;
    }
    _renderer.emplace(_display.canvas(), _display.font(), _display.palette());
    render();
}

/**
 * @brief Выводит текст; мьютекс уже захвачен.
 *
 * Текст идёт кусками через небольшой буфер, в котором одиночный LF
 * заменяется на CR LF. Курсор на время вывода показывается.
 *
 * @param text Байты потока вывода.
 */
void esp::vt::terminal::put(std::string_view text)
{
    _is_cursor_on = true;
    std::array<char, 64> chunk;
    std::size_t length = 0;
    for (const char byte : text)
    {
        if (length + 2 > chunk.size())
        {
            feed(std::string_view{chunk.data(), length});
            length = 0;
        }
        if (byte == '\n' && _last_byte != '\r')
            chunk[length++] = '\r';
        chunk[length++] = byte;
        _last_byte = byte;
    }
    feed(std::string_view{chunk.data(), length});
    render();
}

/**
 * @brief Передаёт байты разборщику экрана и копирует их в UART0.
 * @param bytes Байты потока вывода, уже с CR LF.
 */
void esp::vt::terminal::feed(std::string_view bytes)
{
    if (bytes.empty())
        return;
    _parser->feed(bytes);
    uart_write_bytes(esp::vt::p::UART_PORT, bytes.data(), bytes.size());
}

/**
 * @brief Рисует изменения экрана и подаёт звонок, если пришёл BEL; мьютекс уже захвачен.
 *
 * Приостановленный терминал не рисует — resume() нарисует экран целиком, —
 * и звонок пропускает: генератором звука пользуется другая программа.
 */
void esp::vt::terminal::render()
{
    if (_is_suspended)
    {
        _screen->take_bell();
        return;
    }
    _renderer->draw(*_screen, _is_cursor_on);
    if (_screen->take_bell())
        _sound.play_sound(esp::sound::square_waveform_generator(), esp::vt::BELL_FREQUENCY,
                          esp::vt::BELL_DURATION_MS, esp::vt::BELL_VOLUME);
}

/**
 * @brief Возвращает показ к текущим строкам, если он был сдвинут в историю экрана.
 */
void esp::vt::terminal::show_current_lines()
{
    const esp::vt::p::guard guard{_lock};
    if (_screen->view_offset() == 0)
        return;
    _screen->reset_view();
    render();
}

/**
 * @brief Обрабатывает нажатие клавиши PS/2.
 *
 * Shift+PgUp и Shift+PgDn на основном экране листают историю на полэкрана;
 * остальные клавиши возвращают показ к текущим строкам и уходят в ввод.
 *
 * @param item Нажатие клавиши.
 */
void esp::vt::terminal::on_key(const esp::ps2::virtual_key_item &item)
{
    const bool is_page_up = item.vk == esp::ps2::virtual_key::vk_pageup;
    const bool is_page_down = item.vk == esp::ps2::virtual_key::vk_pagedown;
    if ((is_page_up || is_page_down) && item.shift && !item.ctrl && !item.lalt && !item.ralt)
    {
        const esp::vt::p::guard guard{_lock};
        if (!_screen->is_alternate())
        {
            const int step = std::max(_screen->rows() / 2, 1);
            _screen->scroll_view(is_page_up ? step : -step);
            render();
            return;
        }
    }

    show_current_lines();
    std::array<char, esp::vt::MAX_KEY_SEQUENCE> bytes{};
    const int length = esp::vt::key_sequence(item, bytes);
    push_input(std::string_view{bytes.data(), static_cast<std::size_t>(length)});
}

/**
 * @brief Кладёт байты нажатой клавиши PS/2 в ввод.
 *
 * Если программа не читает ввод и очередь полна, лишние байты отбрасываются:
 * задача клавиатуры не ждёт, а нажатие, которое некому прочесть, не
 * нужно.
 *
 * @param bytes Байты ввода.
 */
void esp::vt::terminal::push_input(std::string_view bytes)
{
    for (const char byte : bytes)
        xQueueSend(_input, &byte, 0);
}


/**
 * @brief Захватывает мьютекс, ожидая сколько потребуется.
 * @param mutex Мьютекс.
 */
esp::vt::p::guard::guard(SemaphoreHandle_t mutex)
    : _mutex{mutex}
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
}

/**
 * @brief Освобождает мьютекс.
 */
esp::vt::p::guard::~guard()
{
    xSemaphoreGive(_mutex);
}
