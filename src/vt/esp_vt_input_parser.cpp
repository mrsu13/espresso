/**
 * @file esp_vt_input_parser.h
 * @brief Объявление разборщика ввода esp::vt::input_parser.
 */

/**
 * @file esp_vt_input_parser.cpp
 * @brief Разборщик ввода: байты от клавиатуры и терминала ПК — в события клавиатуры.
 */

#include "esp_vt_input_parser.h"

#include "esp_vt_controls.h"

#include <algorithm>
#include <cstddef>


/**
 * @class esp::vt::input_parser
 * @brief Разборщик потока ввода: собирает из байтов события клавиатуры esp::vt::key_event.
 *
 * Поток ввода — байты, какие шлёт терминал VT220 или xterm (раздел 6.3
 * документа об устройстве): знаки UTF-8, управляющие знаки и
 * последовательности особых клавиш. Разборщик понимает и варианты других
 * терминалов ПК: Home и End как <tt>ESC [1~</tt> и <tt>ESC [4~</tt> или
 * как <tt>ESC OH</tt> и <tt>ESC OF</tt>, F1–F4 как <tt>ESC [11~</tt> …
 * <tt>ESC [14~</tt>.
 *
 * Один байт даёт не больше одного события. ESC без продолжения — клавиша
 * Esc, но узнать это можно только по времени, поэтому тот, кто читает
 * ввод, ждёт продолжения не дольше @c esp::vt::ESCAPE_TIME_OUT_MS и тогда
 * вызывает time_out() (см. is_pending()). ESC перед знаком — признак Alt.
 * LF сразу после CR пропускается: на Enter терминалы ПК шлют то CR, то
 * CR LF. Испорченная запись UTF-8 отбрасывается, незнакомые
 * последовательности пропускаются целиком.
 */


namespace esp::vt::p
{

/** @brief Особая клавиша, последовательность которой кончается буквой: <tt>ESC [A</tt>, <tt>ESC OP</tt>. */
struct letter_key
{
    /** @brief Последний байт последовательности. */
    char letter;

    /** @brief Клавиша. */
    esp::vt::special_key key;
};

/** @brief Особые клавиши с буквой в конце; у CSI и SS3 буквы одни и те же. */
constexpr letter_key LETTER_KEYS[] = {
    {'A', esp::vt::special_key::up},    {'B', esp::vt::special_key::down},
    {'C', esp::vt::special_key::right}, {'D', esp::vt::special_key::left},
    {'H', esp::vt::special_key::home},  {'F', esp::vt::special_key::end},
    {'P', esp::vt::special_key::f1},    {'Q', esp::vt::special_key::f2},
    {'R', esp::vt::special_key::f3},    {'S', esp::vt::special_key::f4},
};

/** @brief Особая клавиша с номером: <tt>ESC [номер~</tt>. */
struct tilde_key
{
    /** @brief Номер клавиши — первый параметр последовательности. */
    int number;

    /** @brief Клавиша. */
    esp::vt::special_key key;
};

/** @brief Особые клавиши с номером, в том числе номера Home, End и F1–F4 у других терминалов. */
constexpr tilde_key TILDE_KEYS[] = {
    {1, esp::vt::special_key::home},    {2, esp::vt::special_key::insert},
    {3, esp::vt::special_key::del},     {4, esp::vt::special_key::end},
    {5, esp::vt::special_key::page_up}, {6, esp::vt::special_key::page_down},
    {7, esp::vt::special_key::home},    {8, esp::vt::special_key::end},
    {11, esp::vt::special_key::f1},     {12, esp::vt::special_key::f2},
    {13, esp::vt::special_key::f3},     {14, esp::vt::special_key::f4},
    {15, esp::vt::special_key::f5},     {17, esp::vt::special_key::f6},
    {18, esp::vt::special_key::f7},     {19, esp::vt::special_key::f8},
    {20, esp::vt::special_key::f9},     {21, esp::vt::special_key::f10},
    {23, esp::vt::special_key::f11},    {24, esp::vt::special_key::f12},
};

/** @brief Наибольшее значение параметра: больших номеров у клавиш нет, а переполнения не будет. */
constexpr int MAX_KEY_PARAM = 9999;

}


// Открытые

/**
 * @brief Создаёт разборщик в начальном состоянии.
 */
esp::vt::input_parser::input_parser()
    : _state{esp::vt::input_parser::state::ground}
    , _modifiers{0}
    , _code_point{0}
    , _min_code_point{0}
    , _utf8_remaining{0}
    , _params{-1, -1}
    , _param_index{0}
    , _is_ignored{false}
    , _is_after_cr{false}
{
}

/**
 * @brief Разбирает очередной байт ввода.
 * @param byte Байт.
 * @return Событие, если байт его закончил; иначе ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::feed(std::uint8_t byte)
{
    switch (_state)
    {
        case esp::vt::input_parser::state::escape:
            return step_escape(byte);

        case esp::vt::input_parser::state::csi:
            return step_csi(byte);

        case esp::vt::input_parser::state::ss3:
            return step_ss3(byte);

        case esp::vt::input_parser::state::ground:
            break;
    }
    return step_ground(byte);
}

/**
 * @brief Заканчивает разбор, когда продолжения не дождались.
 *
 * ESC без продолжения — клавиша Esc, <tt>ESC [</tt> и <tt>ESC O</tt> —
 * Alt+[ и Alt+O. Начатые последовательность с параметрами и знак UTF-8
 * отбрасываются.
 *
 * @return Событие или ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::time_out()
{
    const esp::vt::input_parser::state state = _state;
    const bool is_bare_csi = _params[0] < 0 && _param_index == 0 && !_is_ignored;
    start_ground();
    switch (state)
    {
        case esp::vt::input_parser::state::escape:
            return emit(esp::vt::key_event{esp::vt::ESC});

        case esp::vt::input_parser::state::csi:
            if (is_bare_csi)
                return emit(esp::vt::key_event{'[', esp::vt::key_event::ALT});
            return std::nullopt;

        case esp::vt::input_parser::state::ss3:
            return emit(esp::vt::key_event{'O', esp::vt::key_event::ALT});

        case esp::vt::input_parser::state::ground:
            break;
    }
    return std::nullopt;
}

/**
 * @brief Возвращает разборщик в начальное состояние; начатое отбрасывается.
 */
void esp::vt::input_parser::reset()
{
    start_ground();
    _is_after_cr = false;
}


// Закрытые

/**
 * @brief Разбирает байт вне последовательности: знак UTF-8, управляющий знак или ESC.
 *
 * Если продолжающий байт UTF-8 не пришёл, начатый знак отбрасывается, а
 * пришедший байт разбирается заново.
 *
 * @param byte Байт.
 * @return Событие или ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::step_ground(std::uint8_t byte)
{
    if (_utf8_remaining > 0)
    {
        if ((byte & 0xC0) == 0x80)
        {
            _code_point = (_code_point << 6) | (byte & 0x3F);
            if (--_utf8_remaining > 0)
                return std::nullopt;
            const std::uint32_t modifiers = _modifiers;
            _modifiers = 0;
            const bool is_valid = _code_point >= _min_code_point && _code_point <= 0x10FFFF
                && (_code_point < 0xD800 || _code_point > 0xDFFF);
            if (!is_valid)
                return std::nullopt;
            return emit(esp::vt::key_event{_code_point, modifiers});
        }
        _utf8_remaining = 0;
        _modifiers = 0;
    }

    if (byte == esp::vt::ESC)
    {
        _state = esp::vt::input_parser::state::escape;
        return std::nullopt;
    }
    if (byte < 0x80)
    {
        const std::uint32_t modifiers = _modifiers;
        _modifiers = 0;
        return emit(esp::vt::key_event{byte, modifiers});
    }

    if (byte >= 0xC2 && byte <= 0xDF)
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
        _modifiers = 0;
    return std::nullopt;
}

/**
 * @brief Разбирает байт после ESC.
 *
 * @c [ и @c O начинают последовательности CSI и SS3, второй ESC делает
 * первый клавишей Esc и сам ждёт продолжения, остальное — знак с Alt.
 *
 * @param byte Байт.
 * @return Событие или ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::step_escape(std::uint8_t byte)
{
    _state = esp::vt::input_parser::state::ground;
    if (byte == '[' || byte == 'O')
    {
        _state =
            byte == '[' ? esp::vt::input_parser::state::csi : esp::vt::input_parser::state::ss3;
        _params = {-1, -1};
        _param_index = 0;
        _is_ignored = false;
        return std::nullopt;
    }
    if (byte == esp::vt::ESC)
    {
        _state = esp::vt::input_parser::state::escape;
        return emit(esp::vt::key_event{esp::vt::ESC});
    }
    if (byte < 0x80)
        return emit(esp::vt::key_event{byte, esp::vt::key_event::ALT});
    _modifiers = esp::vt::key_event::ALT;
    return step_ground(byte);
}

/**
 * @brief Разбирает байт последовательности CSI.
 *
 * Байт, которого в последовательности быть не может, — управляющий знак,
 * DEL или байт старше 0x7F — обрывает её и разбирается заново.
 *
 * @param byte Байт.
 * @return Событие, если последовательность закончилась и знакома; иначе ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::step_csi(std::uint8_t byte)
{
    if (byte >= '0' && byte <= '9')
    {
        if (_param_index < static_cast<int>(_params.size()))
        {
            int &param = _params[static_cast<std::size_t>(_param_index)];
            param = std::min(std::max(param, 0) * 10 + (byte - '0'), esp::vt::p::MAX_KEY_PARAM);
        }
        return std::nullopt;
    }
    if (byte == ';')
    {
        ++_param_index;
        return std::nullopt;
    }
    if ((byte >= 0x20 && byte <= 0x2F) || (byte >= 0x3A && byte <= 0x3F))
    {
        _is_ignored = true;
        return std::nullopt;
    }

    _state = esp::vt::input_parser::state::ground;
    if (byte >= 0x40 && byte <= 0x7E)
    {
        if (_is_ignored)
            return std::nullopt;
        const std::optional<esp::vt::key_event> key = csi_key(byte);
        return key ? emit(*key) : std::nullopt;
    }
    return step_ground(byte);
}

/**
 * @brief Разбирает байт последовательности SS3: цифры признаков или последнюю букву.
 * @param byte Байт.
 * @return Событие, если последовательность закончилась и знакома; иначе ничего.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::step_ss3(std::uint8_t byte)
{
    if (byte >= '0' && byte <= '9')
    {
        _params[1] =
            std::min(std::max(_params[1], 0) * 10 + (byte - '0'), esp::vt::p::MAX_KEY_PARAM);
        return std::nullopt;
    }

    _state = esp::vt::input_parser::state::ground;
    for (const esp::vt::p::letter_key &key : esp::vt::p::LETTER_KEYS)
    {
        if (key.letter == static_cast<char>(byte))
            return emit(esp::vt::key_event{key.key, modifiers()});
    }
    if (byte >= 0x40 && byte <= 0x7E)
        return std::nullopt;
    return step_ground(byte);
}

/**
 * @brief Находит клавишу законченной последовательности CSI.
 * @param final_byte Последний байт последовательности.
 * @return Событие или ничего, если последовательность незнакома.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::csi_key(std::uint8_t final_byte) const
{
    if (final_byte == 'Z')
        return esp::vt::key_event{esp::vt::HT, esp::vt::key_event::SHIFT | modifiers()};

    if (final_byte == '~')
    {
        for (const esp::vt::p::tilde_key &key : esp::vt::p::TILDE_KEYS)
        {
            if (key.number == _params[0])
                return esp::vt::key_event{key.key, modifiers()};
        }
        return std::nullopt;
    }

    for (const esp::vt::p::letter_key &key : esp::vt::p::LETTER_KEYS)
    {
        if (key.letter == static_cast<char>(final_byte))
            return esp::vt::key_event{key.key, modifiers()};
    }
    return std::nullopt;
}

/**
 * @brief Возвращает признаки из второго параметра последовательности.
 *
 * Параметр — 1 плюс признаки: Shift — 1, Alt — 2, Ctrl — 4; Meta (8)
 * пропускается.
 *
 * @return Сумма признаков esp::vt::key_event.
 */
std::uint32_t esp::vt::input_parser::modifiers() const
{
    const int flags = _params[1] > 1 ? _params[1] - 1 : 0;
    return ((flags & 1) ? esp::vt::key_event::SHIFT : 0)
        | ((flags & 2) ? esp::vt::key_event::ALT : 0)
        | ((flags & 4) ? esp::vt::key_event::CTRL : 0);
}

/**
 * @brief Отдаёт событие; LF сразу после CR пропускается.
 * @param key Событие.
 * @return Событие или ничего, если это LF после CR.
 */
std::optional<esp::vt::key_event> esp::vt::input_parser::emit(esp::vt::key_event key)
{
    const bool is_lf_after_cr = _is_after_cr && key == esp::vt::key_event{esp::vt::LF};
    _is_after_cr = key == esp::vt::key_event{esp::vt::CR};
    if (is_lf_after_cr)
        return std::nullopt;
    return key;
}

/**
 * @brief Возвращается к разбору знаков: начатые последовательность и знак UTF-8 отбрасываются.
 */
void esp::vt::input_parser::start_ground()
{
    _state = esp::vt::input_parser::state::ground;
    _utf8_remaining = 0;
    _modifiers = 0;
}
