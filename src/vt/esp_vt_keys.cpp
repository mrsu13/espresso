/**
 * @file esp_vt_keys.h
 * @brief Объявление перевода нажатий клавиш PS/2 в байты VT220, esp::vt::key_sequence().
 */

/**
 * @file esp_vt_keys.cpp
 * @brief Перевод нажатий клавиш PS/2 в байты, которые шлёт терминал VT220 или xterm.
 */

#include "esp_vt_keys.h"

#include "esp_vt_controls.h"

#include <cstddef>
#include <cstdio>


namespace esp::vt::p
{

/**
 * @brief Как особая клавиша записывается в последовательности VT220 и xterm.
 */
struct special_key
{
    /** @brief Виртуальная клавиша. */
    esp::ps2::virtual_key vk;

    /** @brief Последний байт: буква (@c A — стрелка вверх, @c P — F1) или @c ~ у клавиш с номером. */
    char final_byte;

    /** @brief Номер клавиши в @c ESC @c [номер~; 0 — у клавиш с буквой. */
    int number;

    /** @brief Без признаков записывается как @c ESC @c O буква (F1–F4), а не @c ESC @c [ буква. */
    bool is_ss3;
};

/**
 * @brief Особые клавиши и их последовательности.
 *
 * Клавиши дополнительной клавиатуры с выключенным NUM LOCK шлют то же, что
 * их двойники; с включённым у них есть знак ASCII, и до этой таблицы дело
 * не доходит.
 */
constexpr special_key SPECIAL_KEYS[] = {
    {esp::ps2::virtual_key::vk_up, 'A', 0, false},
    {esp::ps2::virtual_key::vk_kp_up, 'A', 0, false},
    {esp::ps2::virtual_key::vk_down, 'B', 0, false},
    {esp::ps2::virtual_key::vk_kp_down, 'B', 0, false},
    {esp::ps2::virtual_key::vk_right, 'C', 0, false},
    {esp::ps2::virtual_key::vk_kp_right, 'C', 0, false},
    {esp::ps2::virtual_key::vk_left, 'D', 0, false},
    {esp::ps2::virtual_key::vk_kp_left, 'D', 0, false},
    {esp::ps2::virtual_key::vk_home, 'H', 0, false},
    {esp::ps2::virtual_key::vk_kp_home, 'H', 0, false},
    {esp::ps2::virtual_key::vk_end, 'F', 0, false},
    {esp::ps2::virtual_key::vk_kp_end, 'F', 0, false},
    {esp::ps2::virtual_key::vk_insert, '~', 2, false},
    {esp::ps2::virtual_key::vk_kp_insert, '~', 2, false},
    {esp::ps2::virtual_key::vk_delete, '~', 3, false},
    {esp::ps2::virtual_key::vk_kp_delete, '~', 3, false},
    {esp::ps2::virtual_key::vk_pageup, '~', 5, false},
    {esp::ps2::virtual_key::vk_kp_pageup, '~', 5, false},
    {esp::ps2::virtual_key::vk_pagedown, '~', 6, false},
    {esp::ps2::virtual_key::vk_kp_pagedown, '~', 6, false},
    {esp::ps2::virtual_key::vk_f1, 'P', 0, true},
    {esp::ps2::virtual_key::vk_f2, 'Q', 0, true},
    {esp::ps2::virtual_key::vk_f3, 'R', 0, true},
    {esp::ps2::virtual_key::vk_f4, 'S', 0, true},
    {esp::ps2::virtual_key::vk_f5, '~', 15, false},
    {esp::ps2::virtual_key::vk_f6, '~', 17, false},
    {esp::ps2::virtual_key::vk_f7, '~', 18, false},
    {esp::ps2::virtual_key::vk_f8, '~', 19, false},
    {esp::ps2::virtual_key::vk_f9, '~', 20, false},
    {esp::ps2::virtual_key::vk_f10, '~', 21, false},
    {esp::ps2::virtual_key::vk_f11, '~', 23, false},
    {esp::ps2::virtual_key::vk_f12, '~', 24, false},
};

}


// Открытые

/**
 * @brief Переводит нажатие клавиши PS/2 в байты, которые шлёт терминал VT220 или xterm.
 *
 * Отпускание клавиши и клавиши без знака (Shift, NUM LOCK) байтов не дают.
 * Особые клавиши — стрелки, Home, End, Insert, Delete, PgUp, PgDn, F1–F12 —
 * дают последовательности: стрелка вверх — @c ESC @c [A, Delete —
 * @c ESC @c [3~, F1 — @c ESC @c OP. С Shift, Alt или Ctrl в них появляется
 * второй параметр: 1 плюс признаки (Shift — 1, Alt — 2, Ctrl — 4), так что
 * Ctrl+← — @c ESC @c [1;5D. Shift+Tab — @c ESC @c [Z. Backspace даёт DEL
 * (0x7F), Ctrl+Backspace — BS (0x08). Остальные клавиши дают свой знак
 * ASCII — драйвер клавиатуры уже учёл раскладку, Shift и Ctrl (Ctrl+A —
 * 0x01), — а с Alt перед знаком идёт ESC.
 *
 * @param item Событие клавиатуры.
 * @param bytes Куда записать байты.
 * @return Число записанных байтов, 0 — клавиша ничего не шлёт.
 */
int esp::vt::key_sequence(const esp::ps2::virtual_key_item &item,
                          std::array<char, esp::vt::MAX_KEY_SEQUENCE> &bytes)
{
    if (!item.down)
        return 0;

    const bool is_alt = item.lalt || item.ralt;
    const int modifiers = 1 + (item.shift ? 1 : 0) + (is_alt ? 2 : 0) + (item.ctrl ? 4 : 0);

    for (const esp::vt::p::special_key &key : esp::vt::p::SPECIAL_KEYS)
    {
        if (key.vk != item.vk)
            continue;

        int length = 0;
        if (key.final_byte == '~')
            length = modifiers > 1
                ? std::snprintf(bytes.data(), bytes.size(), "\x1b[%d;%d~", key.number, modifiers)
                : std::snprintf(bytes.data(), bytes.size(), "\x1b[%d~", key.number);
        else if (modifiers > 1)
            length =
                std::snprintf(bytes.data(), bytes.size(), "\x1b[1;%d%c", modifiers, key.final_byte);
        else
            length = std::snprintf(bytes.data(), bytes.size(), "\x1b%c%c", key.is_ss3 ? 'O' : '[',
                                   key.final_byte);
        return length;
    }

    if (item.vk == esp::ps2::virtual_key::vk_tab && item.shift)
        return std::snprintf(bytes.data(), bytes.size(), "\x1b[Z");

    int length = 0;
    if (is_alt)
        bytes[static_cast<std::size_t>(length++)] = static_cast<char>(esp::vt::ESC);

    if (item.vk == esp::ps2::virtual_key::vk_backspace)
        bytes[static_cast<std::size_t>(length++)] =
            static_cast<char>(item.ctrl ? esp::vt::BS : esp::vt::DEL);
    else if (item.ascii < 0x80)
        bytes[static_cast<std::size_t>(length++)] = static_cast<char>(item.ascii);
    else
        return 0;
    return length;
}
