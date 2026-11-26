/**
 * @file esp_vt_line_edit.h
 * @brief Объявление редактора строки esp::vt::line_edit.
 */

/**
 * @file esp_vt_line_edit.cpp
 * @brief Редактор строки в духе readline: клавиши Emacs, история, поиск, дополнение по Tab.
 */

#include "esp_vt_line_edit.h"

#include "esp_vt_controls.h"
#include "esp_vt_utf8.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>


/**
 * @class esp::vt::line_edit
 * @brief Редактор строки в духе readline (раздел 7.4 документа об устройстве).
 *
 * Редактор получает события клавиатуры (feed()) и всё, что видно, выводит
 * байтами в поток вывода — приглашение, эхо, перерисовку, движение
 * курсора, — поэтому одинаково работает на мониторе и в терминале ПК.
 * Клавиши — как в Emacs и bash: движение по знакам и словам, удаление,
 * вставка удалённого (Ctrl+Y), история (↑, ↓, Ctrl+P, Ctrl+N), поиск по
 * ней (Ctrl+R) и дополнение слова по Tab в стиле bash. Слово — знаки между
 * пробелами, как у Форта.
 *
 * Строка хранится в UTF-8, курсор и удаление идут по целым знакам. Длинная
 * строка переносится на следующие строки экрана. Переносит её сам
 * редактор — CR LF после последнего столбца, а не перенос терминала, —
 * поэтому и в окне терминала ПК шире экрана строка выглядит так же.
 * Приглашение должно начинаться с начала строки экрана: вывод перед ним
 * кончается переводом строки.
 *
 * Строки в историю добавляет вызывающий: так он может и записать их в файл.
 * Вывод копится в буфере и уходит к получателю в конце обработки клавиши.
 */


namespace esp::vt::p
{

/** @brief Начало приглашения поиска по истории, как в bash. */
constexpr std::string_view SEARCH_PROMPT = "(reverse-i-search)`";

/** @brief Начало приглашения поиска, когда строка не нашлась. */
constexpr std::string_view FAILED_SEARCH_PROMPT = "(failed reverse-i-search)`";

/** @brief Конец приглашения поиска. */
constexpr std::string_view SEARCH_PROMPT_END = "': ";

/** @brief Промежуток между столбцами списка вариантов дополнения, знакоместа. */
constexpr int COMPLETION_GAP = 2;

/**
 * @brief Возвращает значение события Ctrl+буква — управляющий знак 1–26.
 * @param letter Заглавная латинская буква.
 * @return Значение для сравнения с esp::vt::key_event::value().
 */
constexpr std::uint32_t ctrl_key(char letter)
{
    return static_cast<std::uint32_t>(letter - 'A' + 1);
}

/**
 * @brief Возвращает значение события знака с Alt.
 * @param code_point Код знака.
 * @return Значение для сравнения с esp::vt::key_event::value().
 */
constexpr std::uint32_t alt_key(std::uint32_t code_point)
{
    return esp::vt::key_event{code_point, esp::vt::key_event::ALT}.value();
}

/**
 * @brief Возвращает значение события особой клавиши.
 * @param key Особая клавиша.
 * @param modifiers Признаки esp::vt::key_event.
 * @return Значение для сравнения с esp::vt::key_event::value().
 */
constexpr std::uint32_t special_key_value(esp::vt::special_key key, std::uint32_t modifiers = 0)
{
    return esp::vt::key_event{key, modifiers}.value();
}

/**
 * @brief Переводит заглавную латинскую букву в строчную; остальное не меняет.
 * @param byte Байт текста.
 * @return Байт без учёта регистра латиницы.
 */
constexpr char fold_latin_case(char byte)
{
    return byte >= 'A' && byte <= 'Z' ? static_cast<char>(byte - 'A' + 'a') : byte;
}

/**
 * @brief Считает общее начало двух слов без учёта регистра латиницы.
 *
 * Общее начало не режет знак UTF-8 слова @p a пополам.
 *
 * @param a Первое слово.
 * @param b Второе слово.
 * @return Длина общего начала, байты.
 */
std::size_t common_prefix_length(std::string_view a, std::string_view b)
{
    const std::size_t size = std::min(a.size(), b.size());
    std::size_t length = 0;
    while (length < size
           && esp::vt::p::fold_latin_case(a[length]) == esp::vt::p::fold_latin_case(b[length]))
        ++length;
    while (length > 0 && length < a.size() && (static_cast<std::uint8_t>(a[length]) & 0xC0) == 0x80)
        --length;
    return length;
}

/**
 * @brief Сравнивает слова для списка вариантов: по алфавиту без учёта регистра латиницы.
 * @param a Первое слово.
 * @param b Второе слово.
 * @return @c true, если @p a идёт раньше @p b.
 */
bool is_word_before(std::string_view a, std::string_view b)
{
    const std::size_t size = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < size; ++i)
    {
        const std::uint8_t x = static_cast<std::uint8_t>(esp::vt::p::fold_latin_case(a[i]));
        const std::uint8_t y = static_cast<std::uint8_t>(esp::vt::p::fold_latin_case(b[i]));
        if (x != y)
            return x < y;
    }
    return a.size() < b.size();
}

}


// Открытые

/**
 * @brief Создаёт редактор; строку начинает start().
 * @param output Куда выводить; должен жить дольше редактора.
 * @param history История строк для ↑, ↓ и Ctrl+R; должна жить дольше редактора.
 */
esp::vt::line_edit::line_edit(esp::vt::writer &output, const esp::vt::line_history &history)
    : _output{output}
    , _history{history}
    , _words{}
    , _prompt{}
    , _columns{1}
    , _prompt_width{0}
    , _line{}
    , _length{0}
    , _cursor{0}
    , _draft{}
    , _draft_length{0}
    , _kill{}
    , _kill_length{0}
    , _query{}
    , _query_length{0}
    , _search_prompt{}
    , _search_prompt_length{0}
    , _history_age{-1}
    , _search_age{-1}
    , _is_searching{false}
    , _is_search_failed{false}
    , _was_tab{false}
    , _drawn_cursor{0}
    , _out{}
    , _out_length{0}
{
}

/**
 * @brief Задаёт слова для дополнения по Tab.
 * @param words Слова; должны жить, пока редактор ими пользуется.
 */
void esp::vt::line_edit::set_words(std::span<const std::string_view> words)
{
    _words = words;
}

/**
 * @brief Начинает новую строку: выводит приглашение.
 *
 * Курсор должен стоять в начале строки экрана.
 *
 * @param prompt Приглашение; может содержать последовательности CSI —
 *        например, цвет — и должно жить, пока набирается строка.
 * @param columns Ширина экрана в знакоместах (esp::vt::terminal::columns()).
 */
void esp::vt::line_edit::start(std::string_view prompt, int columns)
{
    assert(columns > 0);
    _prompt = prompt;
    _columns = columns;
    _length = 0;
    _cursor = 0;
    _history_age = -1;
    _is_searching = false;
    _was_tab = false;
    _drawn_cursor = 0;
    redraw();
    flush();
}

/**
 * @brief Обрабатывает событие клавиатуры.
 * @param key Событие от esp::vt::input_parser.
 * @return @c esp::vt::line_edit::result::accepted — строка готова (line()),
 *         @c esp::vt::line_edit::result::cancelled — отменена,
 *         @c esp::vt::line_edit::result::editing — набирается дальше.
 */
esp::vt::line_edit::result esp::vt::line_edit::feed(esp::vt::key_event key)
{
    const bool was_tab = _was_tab;
    _was_tab = false;
    const esp::vt::line_edit::result result = _is_searching ? edit_search(key) : edit(key, was_tab);
    flush();
    return result;
}


// Закрытые

/**
 * @brief Обрабатывает клавишу правки строки.
 * @param key Событие клавиатуры.
 * @param was_tab Был ли предыдущей клавишей Tab.
 * @return Чем кончилась обработка.
 */
esp::vt::line_edit::result esp::vt::line_edit::edit(esp::vt::key_event key, bool was_tab)
{
    const std::string_view text = line();
    switch (key.value())
    {
        case esp::vt::CR:
        case esp::vt::LF:
            return accept();

        case esp::vt::p::ctrl_key('C'):
            return cancel();

        case esp::vt::p::special_key_value(esp::vt::special_key::left):
        case esp::vt::p::ctrl_key('B'):
            move_to(esp::vt::utf8::previous(text, _cursor));
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::right):
        case esp::vt::p::ctrl_key('F'):
            move_to(esp::vt::utf8::next(text, _cursor));
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::left, esp::vt::key_event::CTRL):
        case esp::vt::p::alt_key('b'):
        case esp::vt::p::alt_key('B'):
            move_to(word_left(_cursor));
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::right, esp::vt::key_event::CTRL):
        case esp::vt::p::alt_key('f'):
        case esp::vt::p::alt_key('F'):
            move_to(word_right(_cursor));
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::home):
        case esp::vt::p::ctrl_key('A'):
            move_to(0);
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::end):
        case esp::vt::p::ctrl_key('E'):
            move_to(_length);
            break;

        case esp::vt::DEL:
        case esp::vt::BS:
            if (_cursor > 0)
                replace(esp::vt::utf8::previous(text, _cursor), _cursor, {});
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::del):
        case esp::vt::p::ctrl_key('D'):
            if (_cursor < _length)
                replace(_cursor, esp::vt::utf8::next(text, _cursor), {});
            break;

        case esp::vt::p::ctrl_key('W'):
        case esp::vt::p::alt_key(esp::vt::DEL):
            kill(word_left(_cursor), _cursor);
            break;

        case esp::vt::p::alt_key('d'):
        case esp::vt::p::alt_key('D'):
            kill(_cursor, word_right(_cursor));
            break;

        case esp::vt::p::ctrl_key('K'):
            kill(_cursor, _length);
            break;

        case esp::vt::p::ctrl_key('U'):
            kill(0, _cursor);
            break;

        case esp::vt::p::ctrl_key('Y'):
            replace(_cursor, _cursor, std::string_view{_kill.data(), _kill_length});
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::up):
        case esp::vt::p::ctrl_key('P'):
            recall(_history_age + 1);
            break;

        case esp::vt::p::special_key_value(esp::vt::special_key::down):
        case esp::vt::p::ctrl_key('N'):
            recall(_history_age - 1);
            break;

        case esp::vt::p::ctrl_key('R'):
            start_search();
            break;

        case esp::vt::HT:
            _was_tab = !complete(was_tab);
            break;

        case esp::vt::p::ctrl_key('L'):
            write("\x1b[H\x1b[2J");
            _drawn_cursor = 0;
            redraw();
            break;

        default:
            if (!key.is_special() && key.modifiers() == 0
                && esp::vt::utf8::is_printable(key.code()))
            {
                std::array<char, esp::vt::utf8::MAX_BYTES> bytes{};
                const int count = esp::vt::utf8::encode(key.code(), bytes);
                replace(_cursor, _cursor,
                        std::string_view{bytes.data(), static_cast<std::size_t>(count)});
            }
            break;
    }
    return esp::vt::line_edit::result::editing;
}

/**
 * @brief Обрабатывает клавишу во время поиска по истории.
 *
 * Знак дописывается в строку поиска, Ctrl+R ищет строку старше, Backspace
 * стирает последний знак поиска, Enter выполняет найденную строку, Ctrl+G
 * и Esc возвращают строку, какой она была до поиска, Ctrl+C отменяет её.
 * Любая другая клавиша кончает поиск с найденной строкой и правит её.
 *
 * @param key Событие клавиатуры.
 * @return Чем кончилась обработка.
 */
esp::vt::line_edit::result esp::vt::line_edit::edit_search(esp::vt::key_event key)
{
    switch (key.value())
    {
        case esp::vt::p::ctrl_key('R'):
            if (_query_length > 0)
                search(_search_age + 1);
            else
                bell();
            return esp::vt::line_edit::result::editing;

        case esp::vt::DEL:
        case esp::vt::BS:
            if (_query_length > 0)
            {
                _query_length = esp::vt::utf8::previous(query(), _query_length);
                if (_query_length > 0)
                    search(0);
                else
                {
                    _search_age = -1;
                    _is_search_failed = false;
                    update_search_prompt();
                    redraw();
                }
            }
            return esp::vt::line_edit::result::editing;

        case esp::vt::p::ctrl_key('G'):
        case esp::vt::ESC:
            stop_search(false);
            return esp::vt::line_edit::result::editing;

        case esp::vt::p::ctrl_key('C'):
            stop_search(false);
            return cancel();

        case esp::vt::CR:
        case esp::vt::LF:
            stop_search(true);
            return accept();

        default:
            break;
    }

    if (!key.is_special() && key.modifiers() == 0 && esp::vt::utf8::is_printable(key.code()))
    {
        std::array<char, esp::vt::utf8::MAX_BYTES> bytes{};
        const std::size_t count =
            static_cast<std::size_t>(esp::vt::utf8::encode(key.code(), bytes));
        if (_query_length + count > _query.size())
        {
            bell();
            return esp::vt::line_edit::result::editing;
        }
        std::copy_n(bytes.data(), count, _query.data() + _query_length);
        _query_length += count;
        search(std::max(_search_age, 0));
        return esp::vt::line_edit::result::editing;
    }

    stop_search(true);
    return edit(key, false);
}

/**
 * @brief Заканчивает строку: курсор уходит в начало строки экрана под ней.
 * @return @c esp::vt::line_edit::result::accepted.
 */
esp::vt::line_edit::result esp::vt::line_edit::accept()
{
    move_cursor(position(line(), _length));
    // Если строка кончилась ровно на краю экрана, курсор уже на новой строке.
    if (_drawn_cursor % _columns != 0 || _drawn_cursor == 0)
        write("\r\n");
    return esp::vt::line_edit::result::accepted;
}

/**
 * @brief Отменяет строку: за ней выводится @c ^C, курсор уходит на новую строку экрана.
 *
 * Отменённый текст остаётся на экране, как в bash, а line() возвращает пустую строку.
 *
 * @return @c esp::vt::line_edit::result::cancelled.
 */
esp::vt::line_edit::result esp::vt::line_edit::cancel()
{
    move_cursor(position(line(), _length));
    put("^C");
    if (_drawn_cursor % _columns != 0)
        write("\r\n");
    _length = 0;
    _cursor = 0;
    return esp::vt::line_edit::result::cancelled;
}

/**
 * @brief Заменяет часть строки текстом и ставит курсор за ним.
 *
 * Если строка стала бы длиннее @c esp::vt::MAX_LINE_BYTES, ничего не
 * меняется и звучит звонок.
 *
 * @param from Начало заменяемой части, байты.
 * @param to Её конец.
 * @param text Новый текст, не из самой строки; пустой — часть удаляется.
 */
void esp::vt::line_edit::replace(std::size_t from, std::size_t to, std::string_view text)
{
    if (from == to && text.empty())
        return;
    if (_length - (to - from) + text.size() > _line.size())
    {
        bell();
        return;
    }
    char *const data = _line.data();
    std::memmove(data + from + text.size(), data + to, _length - to);
    std::copy_n(text.data(), text.size(), data + from);
    _length = _length - (to - from) + text.size();
    _cursor = from + text.size();
    redraw_from(from);
}

/**
 * @brief Удаляет часть строки и запоминает её для Ctrl+Y.
 * @param from Начало части, байты.
 * @param to Её конец.
 */
void esp::vt::line_edit::kill(std::size_t from, std::size_t to)
{
    if (from == to)
        return;
    _kill_length = to - from;
    std::copy_n(_line.data() + from, _kill_length, _kill.data());
    replace(from, to, {});
}

/**
 * @brief Ставит курсор в строке.
 * @param index Начало знака или конец строки, байты.
 */
void esp::vt::line_edit::move_to(std::size_t index)
{
    _cursor = index;
    move_cursor(position(line(), index));
}

/**
 * @brief Находит начало слова слева: пробелы пропускаются, затем знаки слова.
 * @param index Откуда искать, байты.
 * @return Начало слова.
 */
std::size_t esp::vt::line_edit::word_left(std::size_t index) const
{
    while (index > 0 && _line[index - 1] == ' ')
        --index;
    return word_start(index);
}

/**
 * @brief Находит конец слова справа: пробелы пропускаются, затем знаки слова.
 * @param index Откуда искать, байты.
 * @return Конец слова.
 */
std::size_t esp::vt::line_edit::word_right(std::size_t index) const
{
    while (index < _length && _line[index] == ' ')
        ++index;
    while (index < _length && _line[index] != ' ')
        ++index;
    return index;
}

/**
 * @brief Находит начало слова, в котором стоит место: знаки слева до пробела.
 * @param index Место в строке, байты.
 * @return Начало слова; если слева пробел — само место.
 */
std::size_t esp::vt::line_edit::word_start(std::size_t index) const
{
    while (index > 0 && _line[index - 1] != ' ')
        --index;
    return index;
}

/**
 * @brief Заменяет всю строку и выводит её заново.
 * @param text Новая строка, не из самой строки.
 * @param cursor Курсор в новой строке, байты.
 */
void esp::vt::line_edit::set_line(std::string_view text, std::size_t cursor)
{
    _length = std::min(text.size(), _line.size());
    std::copy_n(text.data(), _length, _line.data());
    _cursor = std::min(cursor, _length);
    redraw_from(0);
}

/**
 * @brief Запоминает набираемую строку, если показ уходит в историю впервые.
 */
void esp::vt::line_edit::save_draft()
{
    if (_history_age >= 0)
        return;
    std::copy_n(_line.data(), _length, _draft.data());
    _draft_length = _length;
}

/**
 * @brief Показывает строку истории вместо набираемой.
 *
 * Набираемая строка запоминается при первом шаге в историю, и ↓ после
 * самой новой строки истории возвращает её. Правка показанной строки
 * истории при следующем шаге пропадает: в истории строка остаётся, какой
 * была. Шаг за пределы истории подаёт звонок.
 *
 * @param age Номер строки истории; @c -1 — набираемая строка.
 */
void esp::vt::line_edit::recall(int age)
{
    if (age < -1 || age >= _history.size())
    {
        bell();
        return;
    }
    save_draft();
    _history_age = age;
    const std::string_view text =
        age < 0 ? std::string_view{_draft.data(), _draft_length} : _history.line(age);
    set_line(text, text.size());
}

/**
 * @brief Дополняет слово перед курсором по списку слов, как bash.
 *
 * Варианты — слова, которые начинаются с набранной части слова, без учёта
 * регистра латиницы. Единственный вариант заменяет набранное целиком, и за
 * ним ставится пробел; у нескольких набранное дополняется до общего
 * начала. Если дополнять нечего, звучит звонок; следующий Tab сразу за
 * таким выводит список вариантов — как в readline.
 *
 * @param was_tab Был ли предыдущей клавишей Tab, который строку не изменил.
 * @return @c true, если строка изменилась.
 */
bool esp::vt::line_edit::complete(bool was_tab)
{
    const std::size_t start = word_start(_cursor);
    const std::string_view prefix = line().substr(start, _cursor - start);

    std::vector<std::string_view> matches;
    for (const std::string_view word : _words)
    {
        if (esp::vt::p::common_prefix_length(word, prefix) == prefix.size())
            matches.push_back(word);
    }
    if (matches.empty())
    {
        bell();
        return false;
    }

    if (matches.size() == 1)
    {
        replace(start, _cursor, matches[0]);
        if (_cursor == _length || _line[_cursor] != ' ')
            replace(_cursor, _cursor, " ");
        return true;
    }

    std::size_t common = matches[0].size();
    for (const std::string_view word : matches)
        common = std::min(common, esp::vt::p::common_prefix_length(matches[0], word));
    if (common > prefix.size())
    {
        replace(start, _cursor, matches[0].substr(0, common));
        return true;
    }
    if (was_tab)
        list(matches);
    else
        bell();
    return false;
}

/**
 * @brief Выводит варианты дополнения столбцами под строкой и выводит строку заново.
 * @param matches Варианты; сортируются по алфавиту без учёта регистра латиницы.
 */
void esp::vt::line_edit::list(std::span<std::string_view> matches)
{
    std::sort(matches.begin(), matches.end(), esp::vt::p::is_word_before);
    int width = 0;
    for (const std::string_view word : matches)
        width = std::max(width, esp::vt::utf8::length(word));
    width += esp::vt::p::COMPLETION_GAP;
    const std::size_t per_row = static_cast<std::size_t>(std::max(1, _columns / width));

    move_cursor(position(line(), _length));
    if (_drawn_cursor % _columns != 0 || _drawn_cursor == 0)
        write("\r\n");
    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        write(matches[i]);
        if ((i + 1) % per_row == 0 || i + 1 == matches.size())
            write("\r\n");
        else
        {
            for (int column = esp::vt::utf8::length(matches[i]); column < width; ++column)
                write(" ");
        }
    }
    _drawn_cursor = 0;
    redraw();
}

/**
 * @brief Начинает поиск по истории (Ctrl+R).
 */
void esp::vt::line_edit::start_search()
{
    save_draft();
    _is_searching = true;
    _is_search_failed = false;
    _query_length = 0;
    _search_age = -1;
    update_search_prompt();
    redraw();
}

/**
 * @brief Ищет строку истории со строкой поиска и показывает её.
 *
 * Если строки нет, остаётся прежняя найденная, приглашение сообщает о
 * неудаче и звучит звонок.
 *
 * @param age С какой строки истории искать — к более старым.
 */
void esp::vt::line_edit::search(int age)
{
    const int found = _history.find(query(), age);
    if (found >= 0)
    {
        _search_age = found;
        _is_search_failed = false;
    }
    else
    {
        _is_search_failed = true;
        bell();
    }
    update_search_prompt();
    redraw();
}

/**
 * @brief Кончает поиск по истории.
 * @param is_kept @c true — найденная строка становится набираемой, курсор
 *        — на найденном тексте; @c false — строка остаётся, какой была до поиска.
 */
void esp::vt::line_edit::stop_search(bool is_kept)
{
    if (is_kept && _search_age >= 0)
    {
        const std::size_t cursor = shown_cursor();
        const std::string_view match = _history.line(_search_age);
        _length = std::min(match.size(), _line.size());
        std::copy_n(match.data(), _length, _line.data());
        _cursor = std::min(cursor, _length);
        _history_age = _search_age;
    }
    _is_searching = false;
    redraw();
}

/**
 * @brief Собирает приглашение поиска из строки поиска.
 */
void esp::vt::line_edit::update_search_prompt()
{
    std::size_t length = 0;
    for (const std::string_view part :
         {_is_search_failed ? esp::vt::p::FAILED_SEARCH_PROMPT : esp::vt::p::SEARCH_PROMPT, query(),
          esp::vt::p::SEARCH_PROMPT_END})
    {
        std::copy_n(part.data(), part.size(), _search_prompt.data() + length);
        length += part.size();
    }
    _search_prompt_length = length;
}

/**
 * @brief Возвращает строку поиска.
 * @return Строка UTF-8.
 */
std::string_view esp::vt::line_edit::query() const
{
    return std::string_view{_query.data(), _query_length};
}

/**
 * @brief Возвращает показанное приглашение: обычное или приглашение поиска.
 * @return Приглашение.
 */
std::string_view esp::vt::line_edit::shown_prompt() const
{
    return _is_searching ? std::string_view{_search_prompt.data(), _search_prompt_length} : _prompt;
}

/**
 * @brief Возвращает показанный текст: набираемую строку или найденную строку истории.
 * @return Текст UTF-8.
 */
std::string_view esp::vt::line_edit::shown_text() const
{
    return _is_searching && _search_age >= 0 ? _history.line(_search_age) : line();
}

/**
 * @brief Возвращает курсор в показанном тексте: при поиске — на найденном тексте.
 * @return Место в тексте, байты.
 */
std::size_t esp::vt::line_edit::shown_cursor() const
{
    if (!_is_searching || _search_age < 0)
        return _cursor;
    const std::string_view match = _history.line(_search_age);
    const std::size_t index = match.find(query());
    return index == std::string_view::npos ? match.size() : index;
}

/**
 * @brief Выводит заново приглашение и текст, начиная с первой строки экрана.
 *
 * Остаток прежнего вывода стирается до конца экрана.
 */
void esp::vt::line_edit::redraw()
{
    move_cursor(0);
    put(shown_prompt());
    _prompt_width = _drawn_cursor;
    const std::string_view text = shown_text();
    put(text);
    write("\x1b[J");
    move_cursor(position(text, shown_cursor()));
}

/**
 * @brief Выводит заново строку с заданного места до конца.
 *
 * Остаток прежнего вывода стирается до конца экрана, затем курсор встаёт
 * на место.
 *
 * @param index С какого места строки, байты.
 */
void esp::vt::line_edit::redraw_from(std::size_t index)
{
    const std::string_view text = line();
    move_cursor(position(text, index));
    put(text.substr(index));
    write("\x1b[J");
    move_cursor(position(text, _cursor));
}

/**
 * @brief Возвращает знакоместо места в тексте, считая от начала приглашения.
 * @param text Показанный текст.
 * @param index Место в тексте, байты.
 * @return Номер знакоместа.
 */
int esp::vt::line_edit::position(std::string_view text, std::size_t index) const
{
    return _prompt_width + esp::vt::utf8::length(text.substr(0, index));
}

/**
 * @brief Передвигает курсор экрана последовательностями CUU, CUD, CUF, CUB.
 * @param target Знакоместо, считая от начала приглашения; его строка уже выведена.
 */
void esp::vt::line_edit::move_cursor(int target)
{
    const int row = _drawn_cursor / _columns;
    const int target_row = target / _columns;
    if (target_row < row)
        write_csi(row - target_row, 'A');
    else if (target_row > row)
        write_csi(target_row - row, 'B');

    const int column = _drawn_cursor % _columns;
    const int target_column = target % _columns;
    if (target_column == 0 && column != 0)
        write("\r");
    else if (target_column < column)
        write_csi(column - target_column, 'D');
    else if (target_column > column)
        write_csi(target_column - column, 'C');
    _drawn_cursor = target;
}

/**
 * @brief Выводит текст с места курсора и переносит его на следующую строку экрана.
 *
 * После последнего столбца выводится CR LF. Последовательности CSI —
 * например, цвет в приглашении — места не занимают.
 *
 * @param text Текст UTF-8.
 */
void esp::vt::line_edit::put(std::string_view text)
{
    std::size_t index = 0;
    while (index < text.size())
    {
        const std::uint8_t byte = static_cast<std::uint8_t>(text[index]);
        if (byte == esp::vt::ESC)
        {
            std::size_t end = index + 1;
            if (end < text.size() && text[end] == '[')
            {
                ++end;
                while (end < text.size() && (text[end] < 0x40 || text[end] > 0x7E))
                    ++end;
            }
            end = std::min(end + 1, text.size());
            write(text.substr(index, end - index));
            index = end;
            continue;
        }

        const std::size_t next = esp::vt::utf8::next(text, index);
        write(text.substr(index, next - index));
        index = next;
        if (byte < 0x20)
            continue;
        ++_drawn_cursor;
        if (_drawn_cursor % _columns == 0)
            write("\r\n");
    }
}

/**
 * @brief Выводит последовательность CSI с числом: <tt>ESC [число буква</tt>.
 * @param count Число.
 * @param final_byte Последний байт.
 */
void esp::vt::line_edit::write_csi(int count, char final_byte)
{
    std::array<char, 16> bytes{'\x1b', '['};
    const std::to_chars_result result =
        std::to_chars(bytes.data() + 2, bytes.data() + bytes.size() - 1, count);
    *result.ptr = final_byte;
    write(std::string_view{bytes.data(), static_cast<std::size_t>(result.ptr + 1 - bytes.data())});
}

/**
 * @brief Подаёт звонок: дополнять нечего, строка полна или поиск ничего не нашёл.
 */
void esp::vt::line_edit::bell()
{
    write("\a");
}

/**
 * @brief Добавляет байты в буфер вывода.
 * @param bytes Байты потока вывода.
 */
void esp::vt::line_edit::write(std::string_view bytes)
{
    if (_out_length + bytes.size() > _out.size())
        flush();
    if (bytes.size() > _out.size())
    {
        _output.write(bytes);
        return;
    }
    std::copy_n(bytes.data(), bytes.size(), _out.data() + _out_length);
    _out_length += bytes.size();
}

/**
 * @brief Отдаёт накопленный вывод получателю.
 */
void esp::vt::line_edit::flush()
{
    if (_out_length == 0)
        return;
    _output.write(std::string_view{_out.data(), _out_length});
    _out_length = 0;
}
