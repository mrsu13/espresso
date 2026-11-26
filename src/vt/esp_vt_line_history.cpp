/**
 * @file esp_vt_line_history.h
 * @brief Объявление истории строк esp::vt::line_history.
 */

/**
 * @file esp_vt_line_history.cpp
 * @brief История строк редактора строки в кольцевом буфере.
 */

#include "esp_vt_line_history.h"

#include "esp_vt_config.h"
#include "esp_vt_utf8.h"

#include <algorithm>
#include <cassert>


/**
 * @class esp::vt::line_history
 * @brief История строк редактора строки: последние выполненные строки в кольцевом буфере.
 *
 * Строки хранятся в буфере постоянного размера, а их начала и длины — в
 * кольцевом списке. Когда места для новой строки нет — не хватает байтов
 * или записей, — уходят самые старые. Вся память выделяется в
 * конструкторе, и добавление строки ничего не выделяет.
 *
 * В историю попадают только строки, которые редактор строки может
 * показать и править: правильный UTF-8 из печатных знаков, не длиннее
 * @c esp::vt::MAX_LINE_BYTES. Пустые строки, строки из одних пробелов и
 * повтор самой новой строки не добавляются.
 *
 * Строки нумеруются от самой новой: строка 0 выполнена последней.
 */


// Открытые

/**
 * @brief Создаёт пустую историю и выделяет её память.
 * @param byte_capacity Сколько байтов строк вмещает история, от 1 до 65536.
 * @param line_capacity Сколько строк она помнит, больше 0.
 */
esp::vt::line_history::line_history(int byte_capacity, int line_capacity)
    : _text{}
    , _entries{}
    , _next{0}
    , _first{0}
    , _count{0}
{
    assert(byte_capacity > 0 && byte_capacity <= 65536 && line_capacity > 0);
    _text.resize(static_cast<std::size_t>(byte_capacity));
    _entries.resize(static_cast<std::size_t>(line_capacity));
}

/**
 * @brief Добавляет строку как самую новую.
 *
 * Если места нет, сначала уходят самые старые строки.
 *
 * @param text Строка UTF-8 без перевода строки.
 * @return @c true, если строка добавлена; @c false — пустая, из одних
 *         пробелов, повтор самой новой, слишком длинная или с
 *         управляющими знаками.
 */
bool esp::vt::line_history::add(std::string_view text)
{
    if (text.find_first_not_of(' ') == std::string_view::npos
        || text.size() > static_cast<std::size_t>(esp::vt::MAX_LINE_BYTES)
        || text.size() > _text.size() || !esp::vt::utf8::is_printable_line(text))
        return false;
    if (_count > 0 && line(0) == text)
        return false;

    const std::size_t start = reserve(text.size());
    std::copy(text.begin(), text.end(), _text.begin() + static_cast<std::ptrdiff_t>(start));
    esp::vt::line_history::entry &entry = _entries[(_first + _count) % _entries.size()];
    entry.start = static_cast<std::uint16_t>(start);
    entry.length = static_cast<std::uint16_t>(text.size());
    _next = start + text.size();
    ++_count;
    return true;
}

/**
 * @brief Забывает все строки.
 */
void esp::vt::line_history::clear()
{
    _next = 0;
    _first = 0;
    _count = 0;
}

/**
 * @brief Возвращает строку истории.
 * @param age Строка, от 0 (самая новая) до size() − 1.
 * @return Строка; действительна, пока в историю не добавят новую.
 */
std::string_view esp::vt::line_history::line(int age) const
{
    assert(age >= 0 && static_cast<std::size_t>(age) < _count);
    const esp::vt::line_history::entry &entry =
        _entries[(_first + _count - 1 - static_cast<std::size_t>(age)) % _entries.size()];
    return std::string_view{_text.data() + entry.start, entry.length};
}

/**
 * @brief Ищет строку, в которой есть текст, — от заданной к более старым.
 * @param text Искомый текст; пустой есть в любой строке.
 * @param age С какой строки искать; меньше 0 — с самой новой.
 * @return Номер найденной строки или @c -1, если такой нет.
 */
int esp::vt::line_history::find(std::string_view text, int age) const
{
    for (int i = std::max(age, 0); i < size(); ++i)
    {
        if (line(i).find(text) != std::string_view::npos)
            return i;
    }
    return -1;
}


// Закрытые

/**
 * @brief Освобождает место для новой строки подряд и возвращает его начало.
 *
 * Пока строки лежат подряд от самой старой до @c _next, место ищется за
 * ними, а если до конца буфера не хватает — в начале буфера, перед самой
 * старой. Когда строки уже перешли через конец буфера, свободно только
 * место между самой новой и самой старой. Если места нет, уходят самые
 * старые строки.
 *
 * @param length Длина новой строки, от 1 до размера буфера.
 * @return Номер байта в @c _text, с которого писать строку.
 */
std::size_t esp::vt::line_history::reserve(std::size_t length)
{
    while (true)
    {
        if (_count == _entries.size())
        {
            drop_oldest();
            continue;
        }
        if (_count == 0)
            return 0;

        const std::size_t oldest = _entries[_first].start;
        if (oldest < _next)
        {
            if (_next + length <= _text.size())
                return _next;
            if (length <= oldest)
                return 0;
        }
        else if (_next + length <= oldest)
            return _next;
        drop_oldest();
    }
}

/**
 * @brief Забывает самую старую строку.
 */
void esp::vt::line_history::drop_oldest()
{
    _first = (_first + 1) % _entries.size();
    --_count;
}
