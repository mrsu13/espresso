/**
 * @file esp_shell_terminal_words.h
 * @brief Объявление слов терминала esp::shell::terminal_words.
 */

/**
 * @file esp_shell_terminal_words.cpp
 * @brief Слова Форта для терминала: ввод, экран, звук, видеорежим, проверка платы.
 */

#include "esp_shell_terminal_words.h"

#include "esp_shell_forth_shell.h"
#include "esp_vt_display.h"
#include "esp_vt_key_event.h"
#include "esp_vt_special_key.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdio>
#include <optional>
#include <string_view>


/**
 * @class esp::shell::terminal_words
 * @brief Слова Форта для терминала (раздел 7.5 документа об устройстве).
 *
 * Ввод: @c key и @c key? — байты ввода; @c ekey, @c ekey?, @c ekey>char,
 * @c ekey>fkey и номера клавиш @c k-up … @c k-f12 с разрядами
 * @c k-shift-mask, @c k-ctrl-mask, @c k-alt-mask — события клавиатуры,
 * как в стандарте Forth 2012 (расширение Facility); @c accept — строка
 * через редактор строки. Экран: @c page, @c at-xy, цвета @c fg и @c bg —
 * последовательностями ANSI в потоке вывода, поэтому они действуют и в
 * терминале ПК; @c video-mode. Звук: @c beep и @c tone. И проверка платы
 * @c board-test.
 *
 * Номер события — esp::vt::key_event::value(): код знака или номер особой
 * клавиши с признаками. Для @c ekey>char знак — только знак без
 * клавиш-модификаторов; остальное — особые клавиши и знаки с Alt или
 * Shift+Tab — для @c ekey>fkey. Так каждое событие — что-то одно.
 *
 * Слова работают через оболочку esp::shell::forth_shell — контекст движка
 * (ff_context()): она даёт терминал, вывод, чтение строки, смену
 * видеорежима и проверку платы. Неверный аргумент — ошибка −24 (неверный
 * числовой аргумент), как в стандарте. Слова, которые ждут, — @c key,
 * @c ekey, @c accept, @c ms, @c tone, @c board-test — замечают
 * прерывание (Ctrl+C, K1) и бросают исключение −28, прерывание
 * пользователем (esp::shell::forth_shell::throw_interrupt()): так
 * прерывание действует, даже если слово — последнее в строке и ff больше
 * нигде не проверил бы просьбу остановиться.
 */


namespace esp::shell::p
{

/** @brief Код ошибки «неверный числовой аргумент» по стандарту Forth. */
constexpr std::int64_t INVALID_ARGUMENT = -24;

/** @brief Наибольшая частота звука @c tone, Гц. */
constexpr std::int64_t MAX_FREQUENCY = 20000;

/**
 * @brief Возвращает номер особой клавиши для @c ekey.
 * @param key Особая клавиша.
 * @return Номер.
 */
constexpr std::uint32_t key_id(esp::vt::special_key key)
{
    return static_cast<std::uint32_t>(key);
}

/** @brief Наибольший номер знака: события меньше него без признаков — знаки. */
constexpr std::int64_t FIRST_SPECIAL_KEY = 0x110000;

}


const ff_native_word_t esp::shell::terminal_words::WORDS[] = {
    // Ввод.
    FF_NATIVE("key", &esp::shell::terminal_words::key,
              FF_TR_EN("( -- c )  Read a byte\n"
                       "Waits for a byte of input and leaves it. A Cyrillic letter is two "
                       "bytes, a special key a whole sequence; *ekey* reads them as one event."),
              FF_TR_RU("( -- c )  Прочитать байт\n"
                       "Ждёт байт ввода и кладёт его. Буква кириллицы — два байта, особая "
                       "клавиша — целая последовательность; *ekey* читает их одним событием.")),
    FF_NATIVE("key?", &esp::shell::terminal_words::key_question,
              FF_TR_EN("( -- f )  Input waiting?\nTrue if a byte of input waits to be read."),
              FF_TR_RU("( -- f )  Есть ли ввод\nИстина, если байт ввода ждёт, когда его прочтут.")),
    FF_NATIVE("ekey", &esp::shell::terminal_words::ekey,
              FF_TR_EN("( -- x )  Read a keyboard event\n"
                       "Waits for a key and leaves its event: a character's code, or a special "
                       "key's id (*k-up* … *k-f12*) with the *k-shift-mask*, *k-ctrl-mask* and "
                       "*k-alt-mask* bits."),
              FF_TR_RU("( -- x )  Прочитать событие клавиатуры\n"
                       "Ждёт клавишу и кладёт её событие: код знака или номер особой клавиши "
                       "(*k-up* … *k-f12*) с разрядами *k-shift-mask*, *k-ctrl-mask* и "
                       "*k-alt-mask*.")),
    FF_NATIVE("ekey?", &esp::shell::terminal_words::ekey_question,
              FF_TR_EN("( -- f )  Keyboard event waiting?\n"
                       "True if input waits to be read: *ekey* won't wait long."),
              FF_TR_RU("( -- f )  Есть ли событие клавиатуры\n"
                       "Истина, если ввод ждёт, когда его прочтут: *ekey* долго ждать не "
                       "будет.")),
    FF_NATIVE("ekey>char", &esp::shell::terminal_words::ekey_to_char,
              FF_TR_EN("( x -- c f )  Event to a character\n"
                       "If the event *x* is a character without modifier keys, leaves its code "
                       "and true; otherwise *x* and false."),
              FF_TR_RU("( x -- c f )  Событие в знак\n"
                       "Если событие *x* — знак без клавиш-модификаторов, кладёт его код и "
                       "истину, иначе *x* и ложь.")),
    FF_NATIVE("ekey>fkey", &esp::shell::terminal_words::ekey_to_fkey,
              FF_TR_EN("( x -- u f )  Event to a key id\n"
                       "If the event *x* is a special key or a character with a modifier key "
                       "(Alt+X, Shift+Tab), leaves its id and true; otherwise *x* and false."),
              FF_TR_RU("( x -- u f )  Событие в номер клавиши\n"
                       "Если событие *x* — особая клавиша или знак с клавишей-модификатором "
                       "(Alt+X, Shift+Tab), кладёт его номер и истину, иначе *x* и ложь.")),
    FF_NATIVE("accept", &esp::shell::terminal_words::accept,
              FF_TR_EN("( -- s )  Read a line\n"
                       "Reads a line with the line editor, from where the cursor is, and leaves "
                       "it. The lines have their own history: ↑ and ↓ show earlier answers. "
                       "Ctrl+C, like the K1 button, interrupts the program."),
              FF_TR_RU("( -- s )  Прочитать строку\n"
                       "Читает строку редактором строки с того места, где стоит курсор, и "
                       "кладёт её. У строк своя история: ↑ и ↓ показывают прежние ответы. "
                       "Ctrl+C, как и кнопка K1, прерывает программу.")),

    // Номера особых клавиш и признаки — для ekey>fkey.
    FF_NATIVE(
        "k-up",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::up)>,
        FF_TR_EN("( -- u )  Up arrow key id"), FF_TR_RU("( -- u )  Номер клавиши ↑")),
    FF_NATIVE(
        "k-down",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::down)>,
        FF_TR_EN("( -- u )  Down arrow key id"), FF_TR_RU("( -- u )  Номер клавиши ↓")),
    FF_NATIVE(
        "k-right",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::right)>,
        FF_TR_EN("( -- u )  Right arrow key id"), FF_TR_RU("( -- u )  Номер клавиши →")),
    FF_NATIVE(
        "k-left",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::left)>,
        FF_TR_EN("( -- u )  Left arrow key id"), FF_TR_RU("( -- u )  Номер клавиши ←")),
    FF_NATIVE(
        "k-home",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::home)>,
        FF_TR_EN("( -- u )  Home key id"), FF_TR_RU("( -- u )  Номер клавиши Home")),
    FF_NATIVE(
        "k-end",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::end)>,
        FF_TR_EN("( -- u )  End key id"), FF_TR_RU("( -- u )  Номер клавиши End")),
    FF_NATIVE(
        "k-insert",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::insert)>,
        FF_TR_EN("( -- u )  Insert key id"), FF_TR_RU("( -- u )  Номер клавиши Insert")),
    FF_NATIVE(
        "k-delete",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::del)>,
        FF_TR_EN("( -- u )  Delete key id"), FF_TR_RU("( -- u )  Номер клавиши Delete")),
    FF_NATIVE(
        "k-prior",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::page_up)>,
        FF_TR_EN("( -- u )  PgUp key id"), FF_TR_RU("( -- u )  Номер клавиши PgUp")),
    FF_NATIVE("k-next",
              &esp::shell::terminal_words::constant<esp::shell::p::key_id(
                  esp::vt::special_key::page_down)>,
              FF_TR_EN("( -- u )  PgDn key id"), FF_TR_RU("( -- u )  Номер клавиши PgDn")),
    FF_NATIVE(
        "k-f1",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f1)>,
        FF_TR_EN("( -- u )  F1 key id"), FF_TR_RU("( -- u )  Номер клавиши F1")),
    FF_NATIVE(
        "k-f2",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f2)>,
        FF_TR_EN("( -- u )  F2 key id"), FF_TR_RU("( -- u )  Номер клавиши F2")),
    FF_NATIVE(
        "k-f3",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f3)>,
        FF_TR_EN("( -- u )  F3 key id"), FF_TR_RU("( -- u )  Номер клавиши F3")),
    FF_NATIVE(
        "k-f4",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f4)>,
        FF_TR_EN("( -- u )  F4 key id"), FF_TR_RU("( -- u )  Номер клавиши F4")),
    FF_NATIVE(
        "k-f5",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f5)>,
        FF_TR_EN("( -- u )  F5 key id"), FF_TR_RU("( -- u )  Номер клавиши F5")),
    FF_NATIVE(
        "k-f6",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f6)>,
        FF_TR_EN("( -- u )  F6 key id"), FF_TR_RU("( -- u )  Номер клавиши F6")),
    FF_NATIVE(
        "k-f7",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f7)>,
        FF_TR_EN("( -- u )  F7 key id"), FF_TR_RU("( -- u )  Номер клавиши F7")),
    FF_NATIVE(
        "k-f8",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f8)>,
        FF_TR_EN("( -- u )  F8 key id"), FF_TR_RU("( -- u )  Номер клавиши F8")),
    FF_NATIVE(
        "k-f9",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f9)>,
        FF_TR_EN("( -- u )  F9 key id"), FF_TR_RU("( -- u )  Номер клавиши F9")),
    FF_NATIVE(
        "k-f10",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f10)>,
        FF_TR_EN("( -- u )  F10 key id"), FF_TR_RU("( -- u )  Номер клавиши F10")),
    FF_NATIVE(
        "k-f11",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f11)>,
        FF_TR_EN("( -- u )  F11 key id"), FF_TR_RU("( -- u )  Номер клавиши F11")),
    FF_NATIVE(
        "k-f12",
        &esp::shell::terminal_words::constant<esp::shell::p::key_id(esp::vt::special_key::f12)>,
        FF_TR_EN("( -- u )  F12 key id"), FF_TR_RU("( -- u )  Номер клавиши F12")),
    FF_NATIVE("k-shift-mask", &esp::shell::terminal_words::constant<esp::vt::key_event::SHIFT>,
              FF_TR_EN("( -- u )  Shift bit\n"
                       "The bit *ekey* sets in a key id when Shift is held."),
              FF_TR_RU("( -- u )  Разряд Shift\n"
                       "Разряд, который *ekey* ставит в номер клавиши, если нажат Shift.")),
    FF_NATIVE("k-ctrl-mask", &esp::shell::terminal_words::constant<esp::vt::key_event::CTRL>,
              FF_TR_EN("( -- u )  Ctrl bit\n"
                       "The bit *ekey* sets in a key id when Ctrl is held."),
              FF_TR_RU("( -- u )  Разряд Ctrl\n"
                       "Разряд, который *ekey* ставит в номер клавиши, если нажат Ctrl.")),
    FF_NATIVE("k-alt-mask", &esp::shell::terminal_words::constant<esp::vt::key_event::ALT>,
              FF_TR_EN("( -- u )  Alt bit\n"
                       "The bit *ekey* sets in a key id when Alt is held."),
              FF_TR_RU("( -- u )  Разряд Alt\n"
                       "Разряд, который *ekey* ставит в номер клавиши, если нажат Alt.")),

    // Экран.
    FF_NATIVE("page", &esp::shell::terminal_words::page,
              FF_TR_EN("( -- )  Clear the screen\n"
                       "Clears the screen and puts the cursor at the top left."),
              FF_TR_RU("( -- )  Очистить экран\n"
                       "Очищает экран и ставит курсор в левый верхний угол.")),
    FF_NATIVE("at-xy", &esp::shell::terminal_words::at_xy,
              FF_TR_EN("( x y -- )  Move the cursor\n"
                       "Puts the cursor in column *x*, row *y*, both counted from 0."),
              FF_TR_RU("( x y -- )  Поставить курсор\n"
                       "Ставит курсор в столбец *x*, строку *y*, считая с 0.")),
    FF_NATIVE("fg", &esp::shell::terminal_words::foreground,
              FF_TR_EN("( n -- )  Text colour\n"
                       "Sets the colour of the text that follows: an ANSI colour number, 0–15. "
                       "A video mode with fewer colours shows the nearest one it has."),
              FF_TR_RU("( n -- )  Цвет текста\n"
                       "Задаёт цвет следующего текста — номер цвета ANSI, 0–15. Видеорежим с "
                       "меньшим числом цветов показывает ближайший свой.")),
    FF_NATIVE("bg", &esp::shell::terminal_words::background,
              FF_TR_EN("( n -- )  Background colour\n"
                       "Sets the background colour of the text that follows: an ANSI colour "
                       "number, 0–15."),
              FF_TR_RU("( n -- )  Цвет фона\n"
                       "Задаёт цвет фона следующего текста — номер цвета ANSI, 0–15.")),
    FF_NATIVE("video-mode", &esp::shell::terminal_words::video_mode,
              FF_TR_EN("( n -- )  Switch the video mode\n"
                       "Switches to video mode *n*: 0–3 are 640×480 with 16, 8, 4 and 2 "
                       "colours, 4 and 5 are 800×600 with 4 and 2, 6 and 7 are 1024×768 with 2. "
                       "The screen is cleared."),
              FF_TR_RU("( n -- )  Сменить видеорежим\n"
                       "Включает видеорежим *n*: 0–3 — 640×480 с 16, 8, 4 и 2 цветами, 4 и 5 — "
                       "800×600 с 4 и 2, 6 и 7 — 1024×768 с 2. Экран очищается.")),

    // Время и звук.
    FF_NATIVE("ms", &esp::shell::terminal_words::ms,
              FF_TR_EN("( n -- )  Pause\nWaits *n* milliseconds."),
              FF_TR_RU("( n -- )  Пауза\nЖдёт *n* миллисекунд.")),
    FF_NATIVE("beep", &esp::shell::terminal_words::beep,
              FF_TR_EN("( -- )  Short sound\nRings the bell."),
              FF_TR_RU("( -- )  Короткий звук\nПодаёт звонок.")),
    FF_NATIVE("tone", &esp::shell::terminal_words::tone,
              FF_TR_EN("( frequency ms -- )  Sound\n"
                       "Plays a square wave of *frequency* hertz for *ms* milliseconds and "
                       "waits until it ends."),
              FF_TR_RU("( частота мс -- )  Звук\n"
                       "Подаёт прямоугольную волну частотой *частота* герц длиной *мс* "
                       "миллисекунд и ждёт, пока она отзвучит.")),

    // Плата.
    FF_NATIVE("board-test", &esp::shell::terminal_words::board_test,
              FF_TR_EN("( -- )  Board test\n"
                       "Checks the VGA output, the PS/2 keyboard and mouse and the sound. "
                       "*Esc* returns to the shell."),
              FF_TR_RU("( -- )  Проверка платы\n"
                       "Проверяет вывод на VGA, клавиатуру и мышь PS/2 и звук. "
                       "*Esc* возвращает в оболочку.")),
    FF_NATIVE_END,
};


// Открытые

/**
 * @brief Добавляет слова терминала в словарь движка.
 *
 * Контекст движка (ff_platform_t::context) должен быть оболочкой
 * esp::shell::forth_shell.
 *
 * @param ff Движок.
 * @return @c FF_OK или ошибка ff_register().
 */
ff_error_t esp::shell::terminal_words::register_words(ff_t *ff)
{
    return ff_register(ff, WORDS);
}


// Закрытые

/**
 * @brief Слово-постоянная: кладёт число @p VALUE — номер клавиши или признак.
 * @tparam VALUE Число.
 * @param ff Движок.
 */
template<std::uint32_t VALUE>
void esp::shell::terminal_words::constant(ff_t *ff)
{
    ff_push_int(ff, VALUE);
}

/**
 * @brief Слово @c key ( -- c ): ждёт байт ввода.
 * @param ff Движок.
 */
void esp::shell::terminal_words::key(ff_t *ff)
{
    const int byte = shell(ff).wait_byte();
    if (byte < 0)
    {
        shell(ff).throw_interrupt();
        return;
    }
    ff_push_int(ff, byte);
}

/**
 * @brief Слово @c key? ( -- f ): есть ли байт ввода.
 * @param ff Движок.
 */
void esp::shell::terminal_words::key_question(ff_t *ff)
{
    ff_push_int(ff, shell(ff).terminal().is_input_ready() ? -1 : 0);
}

/**
 * @brief Слово @c ekey ( -- x ): ждёт событие клавиатуры.
 * @param ff Движок.
 */
void esp::shell::terminal_words::ekey(ff_t *ff)
{
    const std::optional<esp::vt::key_event> event = shell(ff).wait_key();
    if (!event)
    {
        shell(ff).throw_interrupt();
        return;
    }
    ff_push_int(ff, event->value());
}

/**
 * @brief Слово @c ekey? ( -- f ): есть ли ввод для события клавиатуры.
 * @param ff Движок.
 */
void esp::shell::terminal_words::ekey_question(ff_t *ff)
{
    ff_push_int(ff, shell(ff).terminal().is_input_ready() ? -1 : 0);
}

/**
 * @brief Слово @c ekey>char ( x -- c f ): знак ли событие.
 *
 * Знак — событие меньше 0x110000 без признаков клавиш-модификаторов.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::ekey_to_char(ff_t *ff)
{
    std::int64_t event = 0;
    if (!ff_pop_int(ff, &event))
        return;
    const bool is_character = event >= 0 && event < esp::shell::p::FIRST_SPECIAL_KEY;
    if (ff_push_int(ff, event))
        ff_push_int(ff, is_character ? -1 : 0);
}

/**
 * @brief Слово @c ekey>fkey ( x -- u f ): особая ли клавиша событие.
 *
 * Особая — всё, что не знак для @c ekey>char: особые клавиши и знаки с
 * клавишами-модификаторами.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::ekey_to_fkey(ff_t *ff)
{
    std::int64_t event = 0;
    if (!ff_pop_int(ff, &event))
        return;
    const bool is_character = event >= 0 && event < esp::shell::p::FIRST_SPECIAL_KEY;
    if (ff_push_int(ff, event))
        ff_push_int(ff, is_character ? 0 : -1);
}

/**
 * @brief Слово @c accept ( -- s ): читает строку редактором строки.
 *
 * Строка попадает во временную область строк ff (ff_push_str()).
 * Прерывание (Ctrl+C, K1) отменяет строку и кончает слово исключением −28.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::accept(ff_t *ff)
{
    const std::optional<std::string_view> line = shell(ff).read_line();
    if (!line && shell(ff).is_interrupted())
    {
        shell(ff).throw_interrupt();
        return;
    }
    const std::string_view text = line ? *line : std::string_view{""};
    ff_push_str(ff, text.data(), text.size());
}

/**
 * @brief Слово @c page ( -- ): очищает экран.
 * @param ff Движок.
 */
void esp::shell::terminal_words::page(ff_t *ff)
{
    shell(ff).write("\x1b[H\x1b[2J");
}

/**
 * @brief Слово @c at-xy ( x y -- ): ставит курсор.
 *
 * За пределы экрана курсор не уходит: координаты урезает экран.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::at_xy(ff_t *ff)
{
    std::int64_t row = 0;
    std::int64_t column = 0;
    if (!ff_pop_int(ff, &row) || !ff_pop_int(ff, &column))
        return;
    // Последовательности CUP номера строк и столбцов — с 1.
    std::array<char, 32> text{};
    std::snprintf(text.data(), text.size(), "\x1b[%d;%dH",
                  static_cast<int>(std::clamp<std::int64_t>(row, 0, 9998)) + 1,
                  static_cast<int>(std::clamp<std::int64_t>(column, 0, 9998)) + 1);
    shell(ff).write(text.data());
}

/**
 * @brief Слово @c fg ( n -- ): цвет текста.
 * @param ff Движок.
 */
void esp::shell::terminal_words::foreground(ff_t *ff)
{
    set_colour(ff, 30, 90);
}

/**
 * @brief Слово @c bg ( n -- ): цвет фона.
 * @param ff Движок.
 */
void esp::shell::terminal_words::background(ff_t *ff)
{
    set_colour(ff, 40, 100);
}

/**
 * @brief Слово @c ms ( n -- ): пауза.
 *
 * Пауза отдаёт процессор другим задачам; прерывание (Ctrl+C, K1) кончает
 * её сразу — исключение −28.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::ms(ff_t *ff)
{
    std::int64_t duration = 0;
    if (!ff_pop_int(ff, &duration))
        return;
    if (duration < 0)
    {
        ff_throwf(ff, esp::shell::p::INVALID_ARGUMENT,
                  FF_TR(ff, FF_TR_EN("Pause %" PRId64 " ms is less than zero."),
                        FF_TR_RU("Пауза %" PRId64 " мс меньше нуля.")),
                  duration);
        return;
    }
    shell(ff).wait(static_cast<int>(std::min<std::int64_t>(duration, INT32_MAX)));
    if (shell(ff).is_interrupted())
        shell(ff).throw_interrupt();
}

/**
 * @brief Слово @c beep ( -- ): звонок.
 * @param ff Движок.
 */
void esp::shell::terminal_words::beep(ff_t *ff)
{
    shell(ff).write("\a");
}

/**
 * @brief Слово @c tone ( частота мс -- ): звук заданной частоты.
 *
 * Слово ждёт, пока звук отзвучит: так ноты мелодии идут одна за другой.
 * Прерывание (Ctrl+C, K1) обрывает звук и кончает слово исключением −28.
 *
 * @param ff Движок.
 */
void esp::shell::terminal_words::tone(ff_t *ff)
{
    std::int64_t duration = 0;
    std::int64_t frequency = 0;
    if (!ff_pop_int(ff, &duration) || !ff_pop_int(ff, &frequency))
        return;
    if (frequency < 1 || frequency > esp::shell::p::MAX_FREQUENCY || duration < 0)
    {
        ff_throwf(ff, esp::shell::p::INVALID_ARGUMENT,
                  FF_TR(ff,
                        FF_TR_EN("Sound %" PRId64 " Hz, %" PRId64
                                 " ms: the frequency must be 1-20000 Hz, the length not "
                                 "less than zero."),
                        FF_TR_RU("Звук %" PRId64 " Гц, %" PRId64
                                 " мс: частота должна быть 1–20000 Гц, длина — не меньше нуля.")),
                  frequency, duration);
        return;
    }
    const int length = static_cast<int>(std::min<std::int64_t>(duration, INT32_MAX));
    shell(ff).terminal().tone(static_cast<int>(frequency), length);
    shell(ff).wait(length);
    if (shell(ff).is_interrupted())
    {
        // Прерванный звук не доигрывает.
        shell(ff).terminal().stop_sound();
        shell(ff).throw_interrupt();
    }
}

/**
 * @brief Слово @c video-mode ( n -- ): включает видеорежим.
 * @param ff Движок.
 */
void esp::shell::terminal_words::video_mode(ff_t *ff)
{
    std::int64_t mode = 0;
    if (!ff_pop_int(ff, &mode))
        return;
    const int count = esp::vt::display::mode_count();
    if (mode < 0 || mode >= count || !shell(ff).set_video_mode(static_cast<int>(mode)))
        ff_throwf(ff, esp::shell::p::INVALID_ARGUMENT,
                  FF_TR(ff, FF_TR_EN("There is no video mode %" PRId64 "; they are 0-%d."),
                        FF_TR_RU("Видеорежима %" PRId64 " нет; есть 0–%d.")),
                  mode, count - 1);
}

/**
 * @brief Слово @c board-test ( -- ): проверка платы.
 * @param ff Движок.
 */
void esp::shell::terminal_words::board_test(ff_t *ff)
{
    shell(ff).board_test();
    if (shell(ff).is_interrupted())
        shell(ff).throw_interrupt();
}

/**
 * @brief Задаёт цвет последовательностью SGR.
 *
 * Цвета 0–7 — SGR @p base + n, яркие 8–15 — SGR @p bright_base + n − 8.
 *
 * @param ff Движок; на стеке — номер цвета ANSI.
 * @param base Номер SGR для цвета 0: 30 — текст, 40 — фон.
 * @param bright_base Номер SGR для цвета 8: 90 — текст, 100 — фон.
 */
void esp::shell::terminal_words::set_colour(ff_t *ff, int base, int bright_base)
{
    std::int64_t colour = 0;
    if (!ff_pop_int(ff, &colour))
        return;
    if (colour < 0 || colour > 15)
    {
        ff_throwf(ff, esp::shell::p::INVALID_ARGUMENT,
                  FF_TR(ff, FF_TR_EN("Colour %" PRId64 " is not 0-15."),
                        FF_TR_RU("Цвет %" PRId64 " — не 0–15.")),
                  colour);
        return;
    }
    const int value = static_cast<int>(colour);
    std::array<char, 16> text{};
    std::snprintf(text.data(), text.size(), "\x1b[%dm",
                  value < 8 ? base + value : bright_base + value - 8);
    shell(ff).write(text.data());
}

/**
 * @brief Возвращает оболочку — контекст движка.
 * @param ff Движок.
 * @return Оболочка.
 */
esp::shell::forth_shell &esp::shell::terminal_words::shell(ff_t *ff)
{
    return *static_cast<esp::shell::forth_shell *>(ff_context(ff));
}
