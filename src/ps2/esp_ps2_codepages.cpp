// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include "esp_ps2_codepages.h"


#pragma GCC optimize("O2")


namespace esp::ps2
{


///////////////////////////////////////////////////////////////////////////////////
// Codepage 437

const virtual_key_ascii VK2ASCII437[] = {

    {vk_grave_a, 0x85}, // à
    {vk_grave_e, 0x8a}, // è
    {vk_acute_e, 0x82}, // é
    {vk_grave_i, 0x8d}, // ì
    {vk_grave_o, 0x95}, // ò
    {vk_grave_u, 0x97}, // ù
    {vk_cedilla_c, 0x87}, // ç
    {vk_eszett, 0xe1}, // ß
    {vk_umlaut_u, 0x81}, // ü
    {vk_umlaut_o, 0x94}, // ö
    {vk_umlaut_a, 0x84}, // ä
    {vk_acuteaccent, 0x27}, // ´ -> '  (not in cp437)
    {vk_pound, 0x9c}, // £
    {vk_euro, 0xee}, // € -> ε  (not in cp437)
    {vk_degree, 0xf8}, // °
    {vk_negation, 0xaa}, // ¬
    {vk_square, 0xfd}, // ²
    {vk_mu, 0xe6}, // µ
    {vk_cedilla_C, 0x80}, // Ç
    {vk_tilde_n, 0xa4}, // ñ
    {vk_tilde_N, 0xa5}, // Ñ
    {vk_upper_a, 0xa6}, // ª
    {vk_acute_a, 0xa0}, // á
    {vk_acute_o, 0xa2}, // ó
    {vk_acute_u, 0xa3}, // ú
    {vk_umlaut_i, 0x8b}, // ï
    {vk_exclaim_inv, 0xad}, // ¡
    {vk_question_inv, 0xa8}, // ¿
    {vk_acute_A, 0x41}, // Á -> A  (not in cp437)
    {vk_acute_E, 0x90}, // É
    {vk_acute_I, 0x49}, // Í -> I  (not in cp437)
    {vk_acute_O, 0xe0}, // Ó -> O  (not in cp437)
    {vk_acute_U, 0x55}, // Ú -> U  (not in cp437)
    {vk_grave_A, 0x41}, // À -> A  (not in cp437)
    {vk_grave_E, 0x45}, // È -> E  (not in cp437)
    {vk_grave_I, 0x49}, // Ì -> I  (not in cp437)
    {vk_grave_O, 0x4f}, // Ò -> O  (not in cp437)
    {vk_grave_U, 0x55}, // Ù -> U  (not in cp437)
    {vk_interpunct, 0xfa}, // ·
    {vk_umlaut_e, 0x89}, // ë
    {vk_umlaut_A, 0x8e}, // Ä
    {vk_umlaut_E, 0x45}, // Ë -> E  (not in cp437)
    {vk_umlaut_I, 0x49}, // Ï -> I  (not in cp437)
    {vk_umlaut_O, 0x99}, // Ö
    {vk_umlaut_U, 0x9a}, // Ü
    {vk_caret_a, 0x83}, // â
    {vk_caret_e, 0x88}, // ê
    {vk_caret_i, 0x8c}, // î
    {vk_caret_o, 0x93}, // ô
    {vk_caret_u, 0x96}, // û
    {vk_caret_A, 0x41}, // Â -> A  (not in cp437)
    {vk_caret_E, 0x45}, // Ê -> E  (not in cp437)
    {vk_caret_I, 0x49}, // Î -> I  (not in cp437)
    {vk_caret_O, 0x4f}, // Ô -> O  (not in cp437)
    {vk_caret_U, 0x55}, // Û -> U  (not in cp437)
    {vk_tilde_a, 0x61}, // ã -> a  (not in cp437)
    {vk_tilde_A, 0x41}, // Ã -> A  (not in cp437)
    {vk_tilde_o, 0x6f}, // õ -> o  (not in cp437)
    {vk_tilde_O, 0x4f}, // Õ -> O  (not in cp437)
    {vk_grave_y, 0x79}, // ỳ -> y  (not in cp437)
    {vk_grave_Y, 0x59}, // Ỳ -> Y  (not in cp437)
    {vk_acute_y, 0x79}, // ý -> y  (not in cp437)
    {vk_acute_Y, 0x59}, // Ý -> Y  (not in cp437)
    {vk_caret_y, 0x79}, // ŷ -> y  (not in cp437)
    {vk_caret_Y, 0x59}, // Ŷ -> Y  (not in cp437)
    {vk_umlaut_y, 0x98}, // ÿ
    {vk_umlaut_Y, 0x59}, // Ÿ -> Y  (not in cp437)

    {vk_none, 0x00}, // end of table
};

const code_page CODE_PAGE_437 = {
    437,
    VK2ASCII437,
};


///////////////////////////////////////////////////////////////////////////////////
// Codepage 1252

const virtual_key_ascii VK2ASCII1252[] = {
    {vk_grave_a, 0xe0}, // à
    {vk_grave_e, 0xe8}, // è
    {vk_acute_e, 0xe9}, // é
    {vk_grave_i, 0xec}, // ì
    {vk_grave_o, 0xf2}, // ò
    {vk_grave_u, 0xf9}, // ù
    {vk_cedilla_c, 0xe7}, // ç
    {vk_eszett, 0xdf}, // ß
    {vk_umlaut_u, 0xfc}, // ü
    {vk_umlaut_o, 0xf6}, // ö
    {vk_umlaut_a, 0xe4}, // ä
    {vk_acuteaccent, 0xb4}, // ´
    {vk_pound, 0xa3}, // £
    {vk_euro, 0x80}, // €
    {vk_degree, 0xb0}, // °
    {vk_section, 0xa7}, // §
    {vk_negation, 0xac}, // ¬
    {vk_square, 0xb2}, // ²
    {vk_mu, 0xb5}, // µ
    {vk_cedilla_C, 0xc7}, // Ç
    {vk_tilde_n, 0xf1}, // ñ
    {vk_tilde_N, 0xd1}, // Ñ
    {vk_upper_a, 0xaa}, // ª
    {vk_acute_a, 0xe1}, // á
    {vk_acute_o, 0xf3}, // ó
    {vk_acute_u, 0xfa}, // ú
    {vk_umlaut_i, 0xef}, // ï
    {vk_exclaim_inv, 0xa1}, // ¡
    {vk_question_inv, 0xbf}, // ¿
    {vk_acute_A, 0xc1}, // Á
    {vk_acute_E, 0xc9}, // É
    {vk_acute_I, 0xcd}, // Í
    {vk_acute_O, 0xd3}, // Ó
    {vk_acute_U, 0xda}, // Ú
    {vk_grave_A, 0xc0}, // À
    {vk_grave_E, 0xc8}, // È
    {vk_grave_I, 0xcc}, // Ì
    {vk_grave_O, 0xd2}, // Ò
    {vk_grave_U, 0xd9}, // Ù
    {vk_interpunct, 0xb7}, // ·
    {vk_diaeresis, 0xa8}, // ¨
    {vk_umlaut_e, 0xeb}, // ë
    {vk_umlaut_A, 0xc4}, // Ä
    {vk_umlaut_E, 0xcb}, // Ë
    {vk_umlaut_I, 0xcf}, // Ï
    {vk_umlaut_O, 0xd6}, // Ö
    {vk_umlaut_U, 0xdc}, // Ü
    {vk_caret_a, 0xe2}, // â
    {vk_caret_e, 0xea}, // ê
    {vk_caret_i, 0xee}, // î
    {vk_caret_o, 0xf4}, // ô
    {vk_caret_u, 0xfb}, // û
    {vk_caret_A, 0xc2}, // Â
    {vk_caret_E, 0xca}, // Ê
    {vk_caret_I, 0xce}, // Î
    {vk_caret_O, 0xd4}, // Ô
    {vk_caret_U, 0xdb}, // Û
    {vk_tilde_a, 0xe3}, // ã
    {vk_tilde_A, 0xc3}, // Ã
    {vk_tilde_o, 0xf5}, // õ
    {vk_tilde_O, 0xd5}, // Õ
    {vk_grave_y, 0x79}, // ỳ -> y  (not in cp1252)
    {vk_grave_Y, 0x59}, // Ỳ -> Y  (not in cp1252)
    {vk_acute_y, 0xfd}, // ý
    {vk_acute_Y, 0xdd}, // Ý
    {vk_caret_y, 0x79}, // ŷ -> y  (not in cp1252)
    {vk_caret_Y, 0x59}, // Ŷ -> Y  (not in cp1252)
    {vk_umlaut_y, 0xff}, // ÿ
    {vk_umlaut_Y, 0x9f}, // Ÿ

    {vk_none, 0x00}, // end of table
};

const code_page CODE_PAGE_1252 = {
    1252,
    VK2ASCII1252,
};


///////////////////////////////////////////////////////////////////////////////////
// virtualKeyToASCII


// -1 = virtual key cannot be translated to ASCII
// of VirtualKeyItem uses:
//    - .vk
//    - .CTRL
//    - .SHIFT
//    - .SCROLLLOCK
int virtual_key_to_ascii(virtual_key_item const &item, code_page const *codepage)
{
    int r = -1;

    if (item.ctrl)
    {

        // CTRL + ....

        switch (item.vk)
        {

            case vk_space:
                r = ASCII_NUL; // CTRL SPACE = NUL
                break;

            case vk_backspace:
                r = ASCII_DEL; // CTRL BACKSPACE = DEL
                break;

            case vk_delete:
            case vk_kp_delete:
                r = ASCII_DEL; // DELETE = dEL
                break;

            case vk_return:
            case vk_kp_enter:
                r = ASCII_LF; // CTRL ENTER = LF
                break;

            case vk_escape:
                r = ASCII_ESC; // CTRL ESCAPE = ESC
                break;

            case vk_2:
                r = ASCII_NUL; // CTRL 2 = NUL
                break;

            case vk_6:
                r = ASCII_RS; // CTRL 6 = RS
                break;

            case vk_a ... vk_z:
                r = item.vk - vk_a + ASCII_SOH; // CTRL alpha = SOH (a) ...SUB (z)
                break;

            case vk_A ... vk_Z:
                r = item.vk - vk_A + ASCII_SOH; // CTRL alpha = SOH (A) ...SUB (Z)
                break;

            case vk_minus:
                r = ASCII_US; // CTRL - = US
                break;

            case vk_backslash:
                r = ASCII_FS; // CTRL \ = FS
                break;

            case vk_question:
                r = ASCII_US; // CTRL ? = US
                break;

            case vk_leftbracket:
                r = ASCII_ESC; // CTRL [ = ESC
                break;

            case vk_rightbracket:
                r = ASCII_GS; // CTRL ] = GS
                break;

            case vk_tilde:
                r = ASCII_RS; // CTRL ~ = RS
                break;

            default:
                break;
        }
    }
    else
    {

        switch (item.vk)
        {

            case vk_backspace:
                r = ASCII_BS; // BACKSPACE = BS
                break;

            case vk_return:
            case vk_kp_enter:
                r = ASCII_CR; // ENTER = CR
                break;

            case vk_tab:
                if (!item.shift)
                    r = ASCII_HT; // TAB = HT
                break;

            case vk_escape:
                r = ASCII_ESC; // ESCAPE = ESC
                break;

            case vk_scrolllock:
                r = item.scrolllock ? ASCII_XOFF : ASCII_XON; // SCROLLLOCK = switch XOFF / XON
                break;

            case vk_0 ... vk_9:
                r = item.vk - vk_0 + '0'; // digits
                break;

            case vk_kp_0 ... vk_kp_9:
                r = item.vk - vk_kp_0 + '0'; // keypad digits
                break;

            case vk_a ... vk_z:
                r = item.vk - vk_a + 'a'; // lowercase alpha
                break;

            case vk_A ... vk_Z:
                r = item.vk - vk_A + 'A'; // uppercase alpha
                break;

            case vk_space:
                r = ASCII_SPC; // SPACE
                break;

            case vk_quote:
                r = '\''; // '
                break;

            case vk_quotedbl:
                r = '"'; // "
                break;

            case vk_equals:
                r = '='; // =
                break;

            case vk_minus:
            case vk_kp_minus:
                r = '-'; // -
                break;

            case vk_plus:
            case vk_kp_plus:
                r = '+'; // +
                break;

            case vk_kp_multiply:
            case vk_asterisk:
                r = '*'; // *
                break;

            case vk_kp_divide:
            case vk_slash:
                r = '/'; // /
                break;

            case vk_kp_period:
            case vk_period:
                r = '.'; // .
                break;

            case vk_colon:
                r = ':'; // :
                break;

            case vk_comma:
                r = ','; // ,
                break;

            case vk_semicolon:
                r = ';'; // ;
                break;

            case vk_ampersand:
                r = '&'; // &
                break;

            case vk_verticalbar:
                r = '|'; // |
                break;

            case vk_hash:
                r = '#'; // #
                break;

            case vk_at:
                r = '@'; // @
                break;

            case vk_caret:
                r = '^'; // ^
                break;

            case vk_dollar:
                r = '$'; // $
                break;

            case vk_graveaccent:
                r = 0x60; // `
                break;

            case vk_percent:
                r = '%'; // %
                break;

            case vk_exclaim:
                r = '!'; // !
                break;

            case vk_leftbrace:
                r = '{'; // {
                break;

            case vk_rightbrace:
                r = '}'; // }
                break;

            case vk_leftparen:
                r = '('; // (
                break;

            case vk_rightparen:
                r = ')'; // )
                break;

            case vk_less:
                r = '<'; // <
                break;

            case vk_greater:
                r = '>'; // >
                break;

            case vk_underscore:
                r = '_'; // _
                break;

            case vk_backslash:
                r = '\\'; // backslash
                break;

            case vk_question:
                r = '?'; // ?
                break;

            case vk_leftbracket:
                r = '['; // [
                break;

            case vk_rightbracket:
                r = ']'; // ]
                break;

            case vk_tilde:
                r = '~'; // ~
                break;

            default:
                // look into codepage table
                if (codepage)
                {
                    for (auto cpitem = codepage->conv_table; cpitem->vk != vk_none; ++cpitem)
                    {
                        if (item.vk == cpitem->vk)
                        {
                            r = cpitem->ascii;
                            break;
                        }
                    }
                }
                break;
        }
    }

    return r;
}


} // end of namespace
