// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_codepages.h
 * @brief Объявление кодовых страниц и перевода виртуальных клавиш в ASCII.
 */

/**
 * @file esp_ps2_codepages.cpp
 * @brief Кодовые страницы 437 и 1252 и перевод виртуальных клавиш в коды знаков.
 */

#include "esp_ps2_codepages.h"

#include "esp_ps2_ascii.h"


#pragma GCC optimize("O2")


namespace esp::ps2::p
{

/**
 * @brief Знаки кодовой страницы 437 (IBM PC), которых нет в ASCII.
 *
 * Знаки, которых нет и в странице 437, заменяются похожими буквами ASCII.
 */
const esp::ps2::virtual_key_ascii VK_TO_ASCII_437[] = {

    {virtual_key::vk_grave_a, 0x85}, // à
    {virtual_key::vk_grave_e, 0x8a}, // è
    {virtual_key::vk_acute_e, 0x82}, // é
    {virtual_key::vk_grave_i, 0x8d}, // ì
    {virtual_key::vk_grave_o, 0x95}, // ò
    {virtual_key::vk_grave_u, 0x97}, // ù
    {virtual_key::vk_cedilla_c, 0x87}, // ç
    {virtual_key::vk_eszett, 0xe1}, // ß
    {virtual_key::vk_umlaut_u, 0x81}, // ü
    {virtual_key::vk_umlaut_o, 0x94}, // ö
    {virtual_key::vk_umlaut_a, 0x84}, // ä
    {virtual_key::vk_acuteaccent, 0x27}, // ´ -> ' (нет в CP437)
    {virtual_key::vk_pound, 0x9c}, // £
    {virtual_key::vk_euro, 0xee}, // € -> ε (нет в CP437)
    {virtual_key::vk_degree, 0xf8}, // °
    {virtual_key::vk_negation, 0xaa}, // ¬
    {virtual_key::vk_square, 0xfd}, // ²
    {virtual_key::vk_mu, 0xe6}, // µ
    {virtual_key::vk_cedilla_C, 0x80}, // Ç
    {virtual_key::vk_tilde_n, 0xa4}, // ñ
    {virtual_key::vk_tilde_N, 0xa5}, // Ñ
    {virtual_key::vk_upper_a, 0xa6}, // ª
    {virtual_key::vk_acute_a, 0xa0}, // á
    {virtual_key::vk_acute_o, 0xa2}, // ó
    {virtual_key::vk_acute_u, 0xa3}, // ú
    {virtual_key::vk_umlaut_i, 0x8b}, // ï
    {virtual_key::vk_exclaim_inv, 0xad}, // ¡
    {virtual_key::vk_question_inv, 0xa8}, // ¿
    {virtual_key::vk_acute_A, 0x41}, // Á -> A (нет в CP437)
    {virtual_key::vk_acute_E, 0x90}, // É
    {virtual_key::vk_acute_I, 0x49}, // Í -> I (нет в CP437)
    {virtual_key::vk_acute_O, 0xe0}, // Ó -> O (нет в CP437)
    {virtual_key::vk_acute_U, 0x55}, // Ú -> U (нет в CP437)
    {virtual_key::vk_grave_A, 0x41}, // À -> A (нет в CP437)
    {virtual_key::vk_grave_E, 0x45}, // È -> E (нет в CP437)
    {virtual_key::vk_grave_I, 0x49}, // Ì -> I (нет в CP437)
    {virtual_key::vk_grave_O, 0x4f}, // Ò -> O (нет в CP437)
    {virtual_key::vk_grave_U, 0x55}, // Ù -> U (нет в CP437)
    {virtual_key::vk_interpunct, 0xfa}, // ·
    {virtual_key::vk_umlaut_e, 0x89}, // ë
    {virtual_key::vk_umlaut_A, 0x8e}, // Ä
    {virtual_key::vk_umlaut_E, 0x45}, // Ë -> E (нет в CP437)
    {virtual_key::vk_umlaut_I, 0x49}, // Ï -> I (нет в CP437)
    {virtual_key::vk_umlaut_O, 0x99}, // Ö
    {virtual_key::vk_umlaut_U, 0x9a}, // Ü
    {virtual_key::vk_caret_a, 0x83}, // â
    {virtual_key::vk_caret_e, 0x88}, // ê
    {virtual_key::vk_caret_i, 0x8c}, // î
    {virtual_key::vk_caret_o, 0x93}, // ô
    {virtual_key::vk_caret_u, 0x96}, // û
    {virtual_key::vk_caret_A, 0x41}, // Â -> A (нет в CP437)
    {virtual_key::vk_caret_E, 0x45}, // Ê -> E (нет в CP437)
    {virtual_key::vk_caret_I, 0x49}, // Î -> I (нет в CP437)
    {virtual_key::vk_caret_O, 0x4f}, // Ô -> O (нет в CP437)
    {virtual_key::vk_caret_U, 0x55}, // Û -> U (нет в CP437)
    {virtual_key::vk_tilde_a, 0x61}, // ã -> a (нет в CP437)
    {virtual_key::vk_tilde_A, 0x41}, // Ã -> A (нет в CP437)
    {virtual_key::vk_tilde_o, 0x6f}, // õ -> o (нет в CP437)
    {virtual_key::vk_tilde_O, 0x4f}, // Õ -> O (нет в CP437)
    {virtual_key::vk_grave_y, 0x79}, // ỳ -> y (нет в CP437)
    {virtual_key::vk_grave_Y, 0x59}, // Ỳ -> Y (нет в CP437)
    {virtual_key::vk_acute_y, 0x79}, // ý -> y (нет в CP437)
    {virtual_key::vk_acute_Y, 0x59}, // Ý -> Y (нет в CP437)
    {virtual_key::vk_caret_y, 0x79}, // ŷ -> y (нет в CP437)
    {virtual_key::vk_caret_Y, 0x59}, // Ŷ -> Y (нет в CP437)
    {virtual_key::vk_umlaut_y, 0x98}, // ÿ
    {virtual_key::vk_umlaut_Y, 0x59}, // Ÿ -> Y (нет в CP437)

    {virtual_key::vk_none, 0x00}, // Конец таблицы.
};

/**
 * @brief Знаки кодовой страницы 1252 (Windows, западноевропейская), которых нет в ASCII.
 */
const esp::ps2::virtual_key_ascii VK_TO_ASCII_1252[] = {
    {virtual_key::vk_grave_a, 0xe0}, // à
    {virtual_key::vk_grave_e, 0xe8}, // è
    {virtual_key::vk_acute_e, 0xe9}, // é
    {virtual_key::vk_grave_i, 0xec}, // ì
    {virtual_key::vk_grave_o, 0xf2}, // ò
    {virtual_key::vk_grave_u, 0xf9}, // ù
    {virtual_key::vk_cedilla_c, 0xe7}, // ç
    {virtual_key::vk_eszett, 0xdf}, // ß
    {virtual_key::vk_umlaut_u, 0xfc}, // ü
    {virtual_key::vk_umlaut_o, 0xf6}, // ö
    {virtual_key::vk_umlaut_a, 0xe4}, // ä
    {virtual_key::vk_acuteaccent, 0xb4}, // ´
    {virtual_key::vk_pound, 0xa3}, // £
    {virtual_key::vk_euro, 0x80}, // €
    {virtual_key::vk_degree, 0xb0}, // °
    {virtual_key::vk_section, 0xa7}, // §
    {virtual_key::vk_negation, 0xac}, // ¬
    {virtual_key::vk_square, 0xb2}, // ²
    {virtual_key::vk_mu, 0xb5}, // µ
    {virtual_key::vk_cedilla_C, 0xc7}, // Ç
    {virtual_key::vk_tilde_n, 0xf1}, // ñ
    {virtual_key::vk_tilde_N, 0xd1}, // Ñ
    {virtual_key::vk_upper_a, 0xaa}, // ª
    {virtual_key::vk_acute_a, 0xe1}, // á
    {virtual_key::vk_acute_o, 0xf3}, // ó
    {virtual_key::vk_acute_u, 0xfa}, // ú
    {virtual_key::vk_umlaut_i, 0xef}, // ï
    {virtual_key::vk_exclaim_inv, 0xa1}, // ¡
    {virtual_key::vk_question_inv, 0xbf}, // ¿
    {virtual_key::vk_acute_A, 0xc1}, // Á
    {virtual_key::vk_acute_E, 0xc9}, // É
    {virtual_key::vk_acute_I, 0xcd}, // Í
    {virtual_key::vk_acute_O, 0xd3}, // Ó
    {virtual_key::vk_acute_U, 0xda}, // Ú
    {virtual_key::vk_grave_A, 0xc0}, // À
    {virtual_key::vk_grave_E, 0xc8}, // È
    {virtual_key::vk_grave_I, 0xcc}, // Ì
    {virtual_key::vk_grave_O, 0xd2}, // Ò
    {virtual_key::vk_grave_U, 0xd9}, // Ù
    {virtual_key::vk_interpunct, 0xb7}, // ·
    {virtual_key::vk_diaeresis, 0xa8}, // ¨
    {virtual_key::vk_umlaut_e, 0xeb}, // ë
    {virtual_key::vk_umlaut_A, 0xc4}, // Ä
    {virtual_key::vk_umlaut_E, 0xcb}, // Ë
    {virtual_key::vk_umlaut_I, 0xcf}, // Ï
    {virtual_key::vk_umlaut_O, 0xd6}, // Ö
    {virtual_key::vk_umlaut_U, 0xdc}, // Ü
    {virtual_key::vk_caret_a, 0xe2}, // â
    {virtual_key::vk_caret_e, 0xea}, // ê
    {virtual_key::vk_caret_i, 0xee}, // î
    {virtual_key::vk_caret_o, 0xf4}, // ô
    {virtual_key::vk_caret_u, 0xfb}, // û
    {virtual_key::vk_caret_A, 0xc2}, // Â
    {virtual_key::vk_caret_E, 0xca}, // Ê
    {virtual_key::vk_caret_I, 0xce}, // Î
    {virtual_key::vk_caret_O, 0xd4}, // Ô
    {virtual_key::vk_caret_U, 0xdb}, // Û
    {virtual_key::vk_tilde_a, 0xe3}, // ã
    {virtual_key::vk_tilde_A, 0xc3}, // Ã
    {virtual_key::vk_tilde_o, 0xf5}, // õ
    {virtual_key::vk_tilde_O, 0xd5}, // Õ
    {virtual_key::vk_grave_y, 0x79}, // ỳ -> y (нет в CP1252)
    {virtual_key::vk_grave_Y, 0x59}, // Ỳ -> Y (нет в CP1252)
    {virtual_key::vk_acute_y, 0xfd}, // ý
    {virtual_key::vk_acute_Y, 0xdd}, // Ý
    {virtual_key::vk_caret_y, 0x79}, // ŷ -> y (нет в CP1252)
    {virtual_key::vk_caret_Y, 0x59}, // Ŷ -> Y (нет в CP1252)
    {virtual_key::vk_umlaut_y, 0xff}, // ÿ
    {virtual_key::vk_umlaut_Y, 0x9f}, // Ÿ

    {virtual_key::vk_none, 0x00}, // Конец таблицы.
};

}


/**
 * @brief Кодовая страница 437 — набор знаков IBM PC.
 */
const esp::ps2::code_page esp::ps2::CODE_PAGE_437 = {
    437,
    esp::ps2::p::VK_TO_ASCII_437,
};

/**
 * @brief Кодовая страница 1252 — западноевропейская страница Windows.
 */
const esp::ps2::code_page esp::ps2::CODE_PAGE_1252 = {
    1252,
    esp::ps2::p::VK_TO_ASCII_1252,
};


// Открытые

/**
 * @brief Переводит событие клавиатуры в код знака.
 *
 * Буквы и знаки ASCII переводятся сами собой (@c virtual_key::vk_A — 0x41). С CTRL
 * получаются управляющие коды: CTRL + пробел — NUL, CTRL + буква — от SOH
 * (0x01) до SUB (0x1A), CTRL + \ — FS (0x1C), CTRL + ? — US (0x1F),
 * CTRL + [ — ESC, CTRL + ] — GS, CTRL + ~ — RS. SCROLL LOCK даёт XON или
 * XOFF. Знаки за пределами ASCII ищутся в таблице кодовой страницы.
 *
 * Из события используются виртуальная клавиша и состояние CTRL, SHIFT и
 * SCROLL LOCK.
 *
 * @param item Событие клавиатуры.
 * @param codepage Кодовая страница для знаков за пределами ASCII.
 * @return Код знака или −1, если клавиша в код не переводится.
 */
int esp::ps2::virtual_key_to_ascii(const virtual_key_item &item, const code_page *codepage)
{
    int r = -1;

    if (item.ctrl)
    {
        switch (item.vk)
        {
            case virtual_key::vk_space:
                r = ASCII_NUL; // CTRL SPACE = NUL
                break;

            case virtual_key::vk_backspace:
                r = ASCII_DEL; // CTRL BACKSPACE = DEL
                break;

            case virtual_key::vk_delete:
            case virtual_key::vk_kp_delete:
                r = ASCII_DEL; // CTRL DELETE = DEL
                break;

            case virtual_key::vk_return:
            case virtual_key::vk_kp_enter:
                r = ASCII_LF; // CTRL ENTER = LF
                break;

            case virtual_key::vk_escape:
                r = ASCII_ESC; // CTRL ESCAPE = ESC
                break;

            case virtual_key::vk_2:
                r = ASCII_NUL; // CTRL 2 = NUL
                break;

            case virtual_key::vk_6:
                r = ASCII_RS; // CTRL 6 = RS
                break;

            case virtual_key::vk_minus:
                r = ASCII_US; // CTRL - = US
                break;

            case virtual_key::vk_backslash:
                r = ASCII_FS; // CTRL \ = FS
                break;

            case virtual_key::vk_question:
                r = ASCII_US; // CTRL ? = US
                break;

            case virtual_key::vk_leftbracket:
                r = ASCII_ESC; // CTRL [ = ESC
                break;

            case virtual_key::vk_rightbracket:
                r = ASCII_GS; // CTRL ] = GS
                break;

            case virtual_key::vk_tilde:
                r = ASCII_RS; // CTRL ~ = RS
                break;

            default:
                // CTRL + буква — от SOH (a, A) до SUB (z, Z): буквы в
                // перечислении идут подряд.
                if (item.vk >= virtual_key::vk_a && item.vk <= virtual_key::vk_z)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_a) + ASCII_SOH;
                else if (item.vk >= virtual_key::vk_A && item.vk <= virtual_key::vk_Z)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_A) + ASCII_SOH;
                break;
        }
    }
    else
    {
        switch (item.vk)
        {
            case virtual_key::vk_backspace:
                r = ASCII_BS; // BACKSPACE = BS
                break;

            case virtual_key::vk_return:
            case virtual_key::vk_kp_enter:
                r = ASCII_CR; // ENTER = CR
                break;

            case virtual_key::vk_tab:
                if (!item.shift)
                    r = ASCII_HT; // TAB = HT
                break;

            case virtual_key::vk_escape:
                r = ASCII_ESC; // ESCAPE = ESC
                break;

            case virtual_key::vk_scrolllock:
                r = item.scrolllock ? ASCII_XOFF : ASCII_XON; // SCROLL LOCK = XOFF или XON
                break;

            case virtual_key::vk_space:
                r = ASCII_SPC; // SPACE
                break;

            case virtual_key::vk_quote:
                r = '\''; // '
                break;

            case virtual_key::vk_quotedbl:
                r = '"'; // "
                break;

            case virtual_key::vk_equals:
                r = '='; // =
                break;

            case virtual_key::vk_minus:
            case virtual_key::vk_kp_minus:
                r = '-'; // -
                break;

            case virtual_key::vk_plus:
            case virtual_key::vk_kp_plus:
                r = '+'; // +
                break;

            case virtual_key::vk_kp_multiply:
            case virtual_key::vk_asterisk:
                r = '*'; // *
                break;

            case virtual_key::vk_kp_divide:
            case virtual_key::vk_slash:
                r = '/'; // /
                break;

            case virtual_key::vk_kp_period:
            case virtual_key::vk_period:
                r = '.'; // .
                break;

            case virtual_key::vk_colon:
                r = ':'; // :
                break;

            case virtual_key::vk_comma:
                r = ','; // ,
                break;

            case virtual_key::vk_semicolon:
                r = ';'; // ;
                break;

            case virtual_key::vk_ampersand:
                r = '&'; // &
                break;

            case virtual_key::vk_verticalbar:
                r = '|'; // |
                break;

            case virtual_key::vk_hash:
                r = '#'; // #
                break;

            case virtual_key::vk_at:
                r = '@'; // @
                break;

            case virtual_key::vk_caret:
                r = '^'; // ^
                break;

            case virtual_key::vk_dollar:
                r = '$'; // $
                break;

            case virtual_key::vk_graveaccent:
                r = 0x60; // `
                break;

            case virtual_key::vk_percent:
                r = '%'; // %
                break;

            case virtual_key::vk_exclaim:
                r = '!'; // !
                break;

            case virtual_key::vk_leftbrace:
                r = '{'; // {
                break;

            case virtual_key::vk_rightbrace:
                r = '}'; // }
                break;

            case virtual_key::vk_leftparen:
                r = '('; // (
                break;

            case virtual_key::vk_rightparen:
                r = ')'; // )
                break;

            case virtual_key::vk_less:
                r = '<'; // <
                break;

            case virtual_key::vk_greater:
                r = '>'; // >
                break;

            case virtual_key::vk_underscore:
                r = '_'; // _
                break;

            case virtual_key::vk_backslash:
                r = '\\'; // Обратная косая черта.
                break;

            case virtual_key::vk_question:
                r = '?'; // ?
                break;

            case virtual_key::vk_leftbracket:
                r = '['; // [
                break;

            case virtual_key::vk_rightbracket:
                r = ']'; // ]
                break;

            case virtual_key::vk_tilde:
                r = '~'; // ~
                break;

            default:
                // Цифры и буквы идут в перечислении подряд.
                if (item.vk >= virtual_key::vk_0 && item.vk <= virtual_key::vk_9)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_0) + '0';
                else if (item.vk >= virtual_key::vk_kp_0 && item.vk <= virtual_key::vk_kp_9)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_kp_0) + '0';
                else if (item.vk >= virtual_key::vk_a && item.vk <= virtual_key::vk_z)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_a) + 'a';
                else if (item.vk >= virtual_key::vk_A && item.vk <= virtual_key::vk_Z)
                    r = static_cast<int>(item.vk) - static_cast<int>(virtual_key::vk_A) + 'A';
                else if (codepage)
                {
                    // Знаки за пределами ASCII — из таблицы кодовой страницы.
                    for (const esp::ps2::virtual_key_ascii *cp_item = codepage->conv_table;
                         cp_item->vk != virtual_key::vk_none; ++cp_item)
                    {
                        if (item.vk == cp_item->vk)
                        {
                            r = cp_item->ascii;
                            break;
                        }
                    }
                }
                break;
        }
    }

    return r;
}
