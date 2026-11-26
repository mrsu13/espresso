#pragma once


/**
 * @file
 * @brief Событие клавиатуры esp::vt::key_event.
 */


#include "esp_vt_special_key.h"

#include <cstdint>


namespace esp::vt
{

/**
 * @brief Событие клавиатуры: знак или особая клавиша с признаками Shift, Alt и Ctrl.
 *
 * Хранится одним 32-разрядным числом — тем, что вернёт слово Форта
 * @c ekey (раздел 7.5 документа об устройстве): в младших разрядах — код
 * знака Unicode или номер особой клавиши (esp::vt::special_key), в
 * старших — признаки @c SHIFT, @c CTRL и @c ALT. Управляющие знаки — тоже
 * знаки: Enter — CR, Tab — HT, Backspace — DEL, Esc — ESC, Ctrl+A — 0x01.
 * Alt+знак — знак с признаком @c ALT, Shift+Tab — HT с признаком @c SHIFT.
 */
class key_event
{

public:

    /** @brief Признак Shift; значение — как у @c k-shift-mask в gforth. */
    static constexpr std::uint32_t SHIFT = 0x10000000;

    /** @brief Признак Ctrl; значение — как у @c k-ctrl-mask в gforth. */
    static constexpr std::uint32_t CTRL = 0x20000000;

    /** @brief Признак Alt; значение — как у @c k-alt-mask в gforth. */
    static constexpr std::uint32_t ALT = 0x40000000;

    constexpr explicit key_event(std::uint32_t code_point, std::uint32_t modifiers = 0);
    constexpr explicit key_event(esp::vt::special_key key, std::uint32_t modifiers = 0);

    constexpr std::uint32_t value() const;
    constexpr std::uint32_t code() const;
    constexpr std::uint32_t modifiers() const;
    constexpr bool is_special() const;
    constexpr esp::vt::special_key special() const;

    constexpr bool operator==(const esp::vt::key_event &other) const = default;

private:

    /** @brief Разряды кода знака или номера особой клавиши; признаки — выше. */
    static constexpr std::uint32_t CODE_MASK = 0x001FFFFF;

    /** @brief Первый номер особой клавиши: сразу за последним кодом Unicode U+10FFFF. */
    static constexpr std::uint32_t FIRST_SPECIAL = 0x110000;

    /** @brief Значение события, как у @c ekey: код или номер и признаки. */
    std::uint32_t _value;
};

}


// Реализация

// Открытые

/**
 * @brief Создаёт событие знака.
 * @param code_point Код знака Unicode, до U+10FFFF; управляющие знаки — тоже знаки.
 * @param modifiers Признаки @c SHIFT, @c CTRL и @c ALT, сложенные вместе; 0 — без признаков.
 */
constexpr esp::vt::key_event::key_event(std::uint32_t code_point, std::uint32_t modifiers)
    : _value{(code_point & CODE_MASK) | modifiers}
{
}

/**
 * @brief Создаёт событие особой клавиши.
 * @param key Особая клавиша.
 * @param modifiers Признаки @c SHIFT, @c CTRL и @c ALT, сложенные вместе; 0 — без признаков.
 */
constexpr esp::vt::key_event::key_event(esp::vt::special_key key, std::uint32_t modifiers)
    : _value{(static_cast<std::uint32_t>(key) & CODE_MASK) | modifiers}
{
}

/**
 * @brief Возвращает значение события — то, что вернёт слово Форта @c ekey.
 * @return Код знака или номер особой клавиши вместе с признаками.
 */
constexpr std::uint32_t esp::vt::key_event::value() const
{
    return _value;
}

/**
 * @brief Возвращает код знака или номер особой клавиши без признаков.
 * @return Код знака Unicode или номер из esp::vt::special_key.
 */
constexpr std::uint32_t esp::vt::key_event::code() const
{
    return _value & CODE_MASK;
}

/**
 * @brief Возвращает признаки клавиш-модификаторов.
 * @return Сумма @c SHIFT, @c CTRL и @c ALT; 0 — без признаков.
 */
constexpr std::uint32_t esp::vt::key_event::modifiers() const
{
    return _value & ~CODE_MASK;
}

/**
 * @brief Сообщает, особая ли это клавиша.
 * @return @c true — особая клавиша, @c false — знак.
 */
constexpr bool esp::vt::key_event::is_special() const
{
    return code() >= FIRST_SPECIAL;
}

/**
 * @brief Возвращает особую клавишу.
 * @return Особая клавиша; для знака значение смысла не имеет.
 */
constexpr esp::vt::special_key esp::vt::key_event::special() const
{
    return static_cast<esp::vt::special_key>(code());
}

/**
 * @fn bool esp::vt::key_event::operator==(const esp::vt::key_event &other) const
 * @brief Сравнивает события: равны ли код и признаки.
 * @param other Другое событие.
 * @return @c true, если события одинаковы.
 */
