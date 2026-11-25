/**
 * @file esp_vt_parser.h
 * @brief Объявление разборщика потока вывода esp::vt::parser.
 */

/**
 * @file esp_vt_parser.cpp
 * @brief Разборщик потока вывода по автомату Пола Уильямса.
 */

#include "esp_vt_parser.h"

#include "esp_vt_config.h"
#include "esp_vt_controls.h"

#include <algorithm>
#include <cstddef>


/**
 * @class esp::vt::parser
 * @brief Разбирает поток байтов, который выводят на терминал.
 *
 * Разбор идёт по автомату Пола Уильямса для терминалов DEC VT500
 * (vt100.net/emu/dec_ansi_parser): поток делится на знаки, управляющие байты
 * C0 и последовательности CSI и ESC, и о каждом найденном разборщик сообщает
 * исполнителю esp::vt::interpreter. Знаки собираются из байтов UTF-8;
 * испорченная запись UTF-8 становится знаком замены
 * @c esp::vt::REPLACEMENT_CHARACTER.
 *
 * Строки OSC (заголовок окна и подобное), DCS, SOS, PM и APC разборщик
 * проходит до конца и отбрасывает, ничего не накапливая: экрану они не
 * нужны, а проход до конца не даёт их содержимому попасть на экран.
 * Восьмибитные управляющие байты C1 не поддерживаются: поток — UTF-8, и
 * байты 0x80–0xFF принадлежат знакам.
 *
 * Разбор идёт байт за байтом, а состояние сохраняется между вызовами
 * feed(), поэтому последовательность может прийти по частям.
 */


// Открытые

/**
 * @brief Создаёт разборщик в начальном состоянии.
 * @param interpreter Исполнитель разобранного; должен жить дольше разборщика.
 */
esp::vt::parser::parser(esp::vt::interpreter &interpreter)
    : _interpreter{interpreter}
    , _state{esp::vt::parser::state::ground}
    , _command{}
    , _param{-1}
    , _is_param_open{false}
    , _code_point{0}
    , _min_code_point{0}
    , _utf8_remaining{0}
{
}

/**
 * @brief Разбирает очередную часть потока.
 *
 * Исполнитель вызывается по ходу разбора. Последовательность или знак UTF-8,
 * не законченные в @p bytes, продолжатся при следующем вызове.
 *
 * @param bytes Байты потока вывода.
 */
void esp::vt::parser::feed(std::string_view bytes)
{
    for (const char byte : bytes)
        step(static_cast<std::uint8_t>(byte));
}

/**
 * @brief Возвращает разборщик в начальное состояние.
 *
 * Начатые последовательность и знак UTF-8 отбрасываются без вызова
 * исполнителя.
 */
void esp::vt::parser::reset()
{
    start_sequence(esp::vt::parser::state::ground);
    _code_point = 0;
    _utf8_remaining = 0;
}


// Закрытые

/**
 * @brief Разбирает один байт.
 *
 * Сначала проверяются переходы, общие для всех состояний: CAN и SUB
 * прерывают последовательность, ESC начинает новую (внутри строки — первый
 * байт её конца). Остальное разбирает обработчик текущего состояния.
 *
 * @param byte Байт потока.
 */
void esp::vt::parser::step(std::uint8_t byte)
{
    if (byte == esp::vt::CAN || byte == esp::vt::SUB)
    {
        flush_utf8();
        _state = esp::vt::parser::state::ground;
        _interpreter.execute(byte);
        return;
    }

    if (byte == esp::vt::ESC)
    {
        if (_state == esp::vt::parser::state::osc_string
            || _state == esp::vt::parser::state::ignored_string)
        {
            _state = esp::vt::parser::state::string_escape;
            return;
        }
        flush_utf8();
        start_sequence(esp::vt::parser::state::escape);
        return;
    }

    switch (_state)
    {
        case esp::vt::parser::state::ground:
            step_ground(byte);
            break;

        case esp::vt::parser::state::escape:
        case esp::vt::parser::state::escape_intermediate:
            step_escape(byte);
            break;

        case esp::vt::parser::state::csi_entry:
        case esp::vt::parser::state::csi_param:
        case esp::vt::parser::state::csi_intermediate:
        case esp::vt::parser::state::csi_ignore:
            step_csi(byte);
            break;

        case esp::vt::parser::state::osc_string:
        case esp::vt::parser::state::ignored_string:
        case esp::vt::parser::state::string_escape:
            step_string(byte);
            break;
    }
}

/**
 * @brief Разбирает байт обычного текста.
 *
 * Управляющий байт обрывает начатый знак UTF-8 и уходит исполнителю; DEL
 * пропускается; остальные байты собираются в знаки.
 *
 * @param byte Байт потока.
 */
void esp::vt::parser::step_ground(std::uint8_t byte)
{
    if (byte < 0x20)
    {
        flush_utf8();
        _interpreter.execute(byte);
    }
    else if (byte != esp::vt::DEL)
        step_utf8(byte);
}

/**
 * @brief Разбирает байт после ESC.
 *
 * @c [ начинает CSI, @c ] — строку OSC, @c P, @c X, @c ^ и @c _ — строки
 * DCS, SOS, PM и APC; байты 0x20–0x2F копятся как промежуточные, байт
 * 0x30–0x7E завершает последовательность ESC. Байт 0x80 и выше
 * последовательности не принадлежит: она отбрасывается, а байт разбирается
 * как текст.
 *
 * @param byte Байт потока.
 */
void esp::vt::parser::step_escape(std::uint8_t byte)
{
    if (byte < 0x20)
    {
        _interpreter.execute(byte);
        return;
    }
    if (byte == esp::vt::DEL)
        return;
    if (byte >= 0x80)
    {
        _state = esp::vt::parser::state::ground;
        step_ground(byte);
        return;
    }
    if (byte <= 0x2F)
    {
        collect_intermediate(byte);
        _state = esp::vt::parser::state::escape_intermediate;
        return;
    }

    if (_state == esp::vt::parser::state::escape)
    {
        if (byte == '[')
        {
            start_sequence(esp::vt::parser::state::csi_entry);
            return;
        }
        if (byte == ']')
        {
            _state = esp::vt::parser::state::osc_string;
            return;
        }
        if (byte == 'P' || byte == 'X' || byte == '^' || byte == '_')
        {
            _state = esp::vt::parser::state::ignored_string;
            return;
        }
    }
    dispatch_esc(byte);
}

/**
 * @brief Разбирает байт последовательности CSI.
 *
 * Цифры и @c ';' собирают параметры; знак частной последовательности
 * (@c ?, @c <, @c =, @c >) допустим только первым; байты 0x20–0x2F копятся
 * как промежуточные; байт 0x40–0x7E завершает последовательность. Что не
 * укладывается в эти правила — например, @c ':' в параметрах или знак
 * частной последовательности в середине, — портит последовательность: она
 * пропускается до последнего байта и исполнителю не передаётся.
 *
 * @param byte Байт потока.
 */
void esp::vt::parser::step_csi(std::uint8_t byte)
{
    if (byte < 0x20)
    {
        _interpreter.execute(byte);
        return;
    }
    if (byte == esp::vt::DEL)
        return;
    if (byte >= 0x80)
    {
        _state = esp::vt::parser::state::ground;
        step_ground(byte);
        return;
    }
    if (byte >= 0x40)
    {
        if (_state == esp::vt::parser::state::csi_ignore)
            _state = esp::vt::parser::state::ground;
        else
            dispatch_csi(byte);
        return;
    }
    if (_state == esp::vt::parser::state::csi_ignore)
        return;

    if (byte <= 0x2F)
    {
        if (_is_param_open)
            commit_param();
        collect_intermediate(byte);
        _state = esp::vt::parser::state::csi_intermediate;
        return;
    }

    // Байты 0x30–0x3F: цифры, ':', ';' и знаки частной последовательности.
    if (_state == esp::vt::parser::state::csi_intermediate)
    {
        _state = esp::vt::parser::state::csi_ignore;
        return;
    }
    if (byte >= '0' && byte <= '9')
    {
        collect_digit(byte);
        _state = esp::vt::parser::state::csi_param;
        return;
    }
    if (byte == ';')
    {
        commit_param();
        _is_param_open = true;
        _state = esp::vt::parser::state::csi_param;
        return;
    }
    if (byte >= '<' && _state == esp::vt::parser::state::csi_entry)
    {
        _command.private_marker = byte;
        _state = esp::vt::parser::state::csi_param;
        return;
    }
    _state = esp::vt::parser::state::csi_ignore;
}

/**
 * @brief Разбирает байт строки OSC, DCS, SOS, PM или APC.
 *
 * Байты строки отбрасываются. Строку завершает @c ESC @c \\, строку OSC ещё
 * и BEL. ESC с другим байтом обрывает строку и начинает новую
 * последовательность с этого байта.
 *
 * @param byte Байт потока.
 */
void esp::vt::parser::step_string(std::uint8_t byte)
{
    if (_state == esp::vt::parser::state::string_escape)
    {
        if (byte == '\\')
        {
            _state = esp::vt::parser::state::ground;
            return;
        }
        start_sequence(esp::vt::parser::state::escape);
        step_escape(byte);
        return;
    }
    if (_state == esp::vt::parser::state::osc_string && byte == esp::vt::BEL)
        _state = esp::vt::parser::state::ground;
}

/**
 * @brief Собирает знак из байтов UTF-8.
 *
 * Готовый знак уходит исполнителю. Вместо испорченной записи выводится
 * знак замены: так заменяются продолжающий байт без начала, недопустимый
 * первый байт (0xC0, 0xC1, 0xF5–0xFF), избыточная запись, половинка
 * суррогатной пары и код больше U+10FFFF. Если продолжение не пришло,
 * выводится знак замены, а пришедший байт разбирается заново как начало.
 *
 * @param byte Байт 0x20–0xFF, кроме DEL.
 */
void esp::vt::parser::step_utf8(std::uint8_t byte)
{
    if (_utf8_remaining == 0)
    {
        if (byte < 0x80)
            _interpreter.print(byte);
        else if (byte >= 0xC2 && byte <= 0xDF)
        {
            _code_point = byte & 0x1F;
            _min_code_point = 0x80;
            _utf8_remaining = 1;
        }
        else if (byte >= 0xE0 && byte <= 0xEF)
        {
            _code_point = byte & 0x0F;
            _min_code_point = 0x800;
            _utf8_remaining = 2;
        }
        else if (byte >= 0xF0 && byte <= 0xF4)
        {
            _code_point = byte & 0x07;
            _min_code_point = 0x10000;
            _utf8_remaining = 3;
        }
        else
            _interpreter.print(esp::vt::REPLACEMENT_CHARACTER);
        return;
    }

    if ((byte & 0xC0) != 0x80)
    {
        _utf8_remaining = 0;
        _interpreter.print(esp::vt::REPLACEMENT_CHARACTER);
        step_utf8(byte);
        return;
    }

    _code_point = (_code_point << 6) | (byte & 0x3F);
    --_utf8_remaining;
    if (_utf8_remaining > 0)
        return;

    const bool is_valid = _code_point >= _min_code_point && _code_point <= 0x10FFFF
        && (_code_point < 0xD800 || _code_point > 0xDFFF);
    _interpreter.print(is_valid ? _code_point : esp::vt::REPLACEMENT_CHARACTER);
}

/**
 * @brief Обрывает начатый знак UTF-8: если он был, выводится знак замены.
 */
void esp::vt::parser::flush_utf8()
{
    if (_utf8_remaining == 0)
        return;
    _utf8_remaining = 0;
    _interpreter.print(esp::vt::REPLACEMENT_CHARACTER);
}

/**
 * @brief Начинает новую последовательность.
 * @param state Состояние, в которое переходит автомат.
 */
void esp::vt::parser::start_sequence(esp::vt::parser::state state)
{
    _command.clear();
    _param = -1;
    _is_param_open = false;
    _state = state;
}

/**
 * @brief Запоминает промежуточный байт; сверх @c esp::vt::MAX_INTERMEDIATES они отбрасываются.
 * @param byte Промежуточный байт 0x20–0x2F.
 */
void esp::vt::parser::collect_intermediate(std::uint8_t byte)
{
    if (_command.intermediate_count >= esp::vt::MAX_INTERMEDIATES)
        return;
    _command.intermediates[static_cast<std::size_t>(_command.intermediate_count)] = byte;
    ++_command.intermediate_count;
}

/**
 * @brief Добавляет цифру к собираемому параметру, не давая ему превысить @c esp::vt::MAX_PARAM_VALUE.
 * @param byte Цифра @c '0'–@c '9'.
 */
void esp::vt::parser::collect_digit(std::uint8_t byte)
{
    if (_param < 0)
        _param = 0;
    if (_param < esp::vt::MAX_PARAM_VALUE)
        _param = std::min(_param * 10 + (byte - '0'), esp::vt::MAX_PARAM_VALUE);
    _is_param_open = true;
}

/**
 * @brief Записывает собранный параметр в последовательность и начинает следующий.
 *
 * Параметры сверх @c esp::vt::MAX_PARAMS отбрасываются.
 */
void esp::vt::parser::commit_param()
{
    if (_command.param_count < esp::vt::MAX_PARAMS)
    {
        _command.params[static_cast<std::size_t>(_command.param_count)] = _param;
        ++_command.param_count;
    }
    _param = -1;
    _is_param_open = false;
}

/**
 * @brief Завершает последовательность CSI и передаёт её исполнителю.
 * @param final_byte Последний байт 0x40–0x7E.
 */
void esp::vt::parser::dispatch_csi(std::uint8_t final_byte)
{
    if (_is_param_open)
        commit_param();
    _command.final_byte = final_byte;
    _state = esp::vt::parser::state::ground;
    _interpreter.csi(_command);
}

/**
 * @brief Завершает последовательность ESC и передаёт её исполнителю.
 * @param final_byte Последний байт 0x30–0x7E.
 */
void esp::vt::parser::dispatch_esc(std::uint8_t final_byte)
{
    _command.final_byte = final_byte;
    _state = esp::vt::parser::state::ground;
    _interpreter.esc(_command);
}
