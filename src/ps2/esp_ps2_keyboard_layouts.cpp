// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_keyboard_layouts.h
 * @brief Объявление готовых раскладок клавиатуры.
 */

/**
 * @file esp_ps2_keyboard_layouts.cpp
 * @brief Готовые раскладки клавиатуры: US, UK, немецкая, итальянская, испанская,
 *        французская, бельгийская, норвежская и японская.
 */

#include "esp_ps2_keyboard_layouts.h"


/**
 * @brief Раскладка US English — американская; от неё наследуют остальные.
 */
const esp::ps2::keyboard_layout esp::ps2::US_LAYOUT{
    // Имя.
    "US",

    // Описание.
    "US English",

    // Унаследованная раскладка.
    nullptr,

    // Однобайтовые скан-коды.
    {
        {0x76, virtual_key::vk_escape},
        {0x05, virtual_key::vk_f1},
        {0x06, virtual_key::vk_f2},
        {0x04, virtual_key::vk_f3},
        {0x0C, virtual_key::vk_f4},
        {0x03, virtual_key::vk_f5},
        {0x0B, virtual_key::vk_f6},
        {0x83, virtual_key::vk_f7},
        {0x0A, virtual_key::vk_f8},
        {0x01, virtual_key::vk_f9},
        {0x09, virtual_key::vk_f10},
        {0x78, virtual_key::vk_f11},
        {0x07, virtual_key::vk_f12},
        {0x16, virtual_key::vk_1},
        {0x1E, virtual_key::vk_2},
        {0x26, virtual_key::vk_3},
        {0x25, virtual_key::vk_4},
        {0x2E, virtual_key::vk_5},
        {0x36, virtual_key::vk_6},
        {0x3D, virtual_key::vk_7},
        {0x3E, virtual_key::vk_8},
        {0x46, virtual_key::vk_9},
        {0x45, virtual_key::vk_0},
        {0x15, virtual_key::vk_q},
        {0x1D, virtual_key::vk_w},
        {0x24, virtual_key::vk_e},
        {0x2D, virtual_key::vk_r},
        {0x2C, virtual_key::vk_t},
        {0x35, virtual_key::vk_y},
        {0x3C, virtual_key::vk_u},
        {0x43, virtual_key::vk_i},
        {0x44, virtual_key::vk_o},
        {0x4D, virtual_key::vk_p},
        {0x1C, virtual_key::vk_a},
        {0x1b, virtual_key::vk_s},
        {0x23, virtual_key::vk_d},
        {0x2b, virtual_key::vk_f},
        {0x34, virtual_key::vk_g},
        {0x33, virtual_key::vk_h},
        {0x3B, virtual_key::vk_j},
        {0x42, virtual_key::vk_k},
        {0x4B, virtual_key::vk_l},
        {0x1A, virtual_key::vk_z},
        {0x22, virtual_key::vk_x},
        {0x21, virtual_key::vk_c},
        {0x2A, virtual_key::vk_v},
        {0x32, virtual_key::vk_b},
        {0x31, virtual_key::vk_n},
        {0x3A, virtual_key::vk_m},
        {0x0E, virtual_key::vk_graveaccent},
        {0x4E, virtual_key::vk_minus},
        {0x55, virtual_key::vk_equals},
        {0x54, virtual_key::vk_leftbracket},
        {0x5B, virtual_key::vk_rightbracket},
        {0x5D, virtual_key::vk_backslash},
        {0x4C, virtual_key::vk_semicolon},
        {0x52, virtual_key::vk_quote},
        {0x41, virtual_key::vk_comma},
        {0x49, virtual_key::vk_period},
        {0x4A, virtual_key::vk_slash},
        {0x70, virtual_key::vk_kp_insert},
        {0x69, virtual_key::vk_kp_end},
        {0x72, virtual_key::vk_kp_down},
        {0x7A, virtual_key::vk_kp_pagedown},
        {0x6B, virtual_key::vk_kp_left},
        {0x73, virtual_key::vk_kp_center}, // «5» на цифровой панели.
        {0x74, virtual_key::vk_kp_right},
        {0x6C, virtual_key::vk_kp_home},
        {0x75, virtual_key::vk_kp_up},
        {0x7D, virtual_key::vk_kp_pageup},
        {0x71, virtual_key::vk_kp_delete},
        {0x7C, virtual_key::vk_kp_multiply},
        {0x79, virtual_key::vk_kp_plus},
        {0x7B, virtual_key::vk_kp_minus},
        {0x66, virtual_key::vk_backspace},
        {0x0D, virtual_key::vk_tab},
        {0x5A, virtual_key::vk_return},
        {0x77, virtual_key::vk_numlock},
        {0x7E, virtual_key::vk_scrolllock},
        {0x58, virtual_key::vk_capslock},
        {0x12, virtual_key::vk_lshift},
        {0x59, virtual_key::vk_rshift},
        {0x14, virtual_key::vk_lctrl},
        {0x11, virtual_key::vk_lalt},
        {0x29, virtual_key::vk_space},
        {0x84, virtual_key::vk_sysreq}, // ALT + PRINTSCREEN: клавиатура переводит сама.
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {
        {0x14, virtual_key::vk_rctrl},
        {0x11, virtual_key::vk_ralt},
        {0x1F, virtual_key::vk_lgui},
        {0x27, virtual_key::vk_rgui},
        {0x2F, virtual_key::vk_application},
        {0x70, virtual_key::vk_insert},
        {0x71, virtual_key::vk_delete},
        {0x4A, virtual_key::vk_kp_divide},
        {0x5A, virtual_key::vk_kp_enter},
        {0x7D, virtual_key::vk_pageup},
        {0x7A, virtual_key::vk_pagedown},
        {0x6C, virtual_key::vk_home},
        {0x69, virtual_key::vk_end},
        {0x75, virtual_key::vk_up},
        {0x72, virtual_key::vk_down},
        {0x6B, virtual_key::vk_left},
        {0x74, virtual_key::vk_right},
        {0x7C, virtual_key::vk_printscreen},
        // CTRL + PAUSE даёт не обычную последовательность PAUSE, а «E0 7E» —
        // это BREAK; нажатие и отпускание приходят сразу (E0 7E E0 F0 7E).
        {0x7E, virtual_key::vk_break},
        // Эти коды отбрасываются: это что-то вроде расширенных левого и
        // правого SHIFT, которые окружают коды некоторых клавиш (INSERT,
        // стрелки...) при SHIFT или NUM LOCK и даже PRINTSCREEN без SHIFT.
        {0x12, virtual_key::vk_none},
        {0x59, virtual_key::vk_none},
    },

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_1, {0, 0, 0, 1}, virtual_key::vk_exclaim}, // SHIFT "1" = "!"
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_at}, // SHIFT "2" = "@"
        {virtual_key::vk_3, {0, 0, 0, 1}, virtual_key::vk_hash}, // SHIFT "3" = "#"
        {virtual_key::vk_4, {0, 0, 0, 1}, virtual_key::vk_dollar}, // SHIFT "4" = "$"
        {virtual_key::vk_5, {0, 0, 0, 1}, virtual_key::vk_percent}, // SHIFT "5" = "%"
        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_caret}, // SHIFT "6" = "^"
        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "7" = "&"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "8" = "*"
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "9" = "("
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "0" = ")"

        {virtual_key::vk_graveaccent, {0, 0, 0, 1}, virtual_key::vk_tilde}, // SHIFT "`" = "~"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"
        {virtual_key::vk_equals, {0, 0, 0, 1}, virtual_key::vk_plus}, // SHIFT "=" = "+"
        {virtual_key::vk_leftbracket, {0, 0, 0, 1}, virtual_key::vk_leftbrace}, // SHIFT "[" = "{"
        {virtual_key::vk_rightbracket, {0, 0, 0, 1}, virtual_key::vk_rightbrace}, // SHIFT "]" = "}"
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_verticalbar}, // SHIFT "\" = "|"
        {virtual_key::vk_semicolon, {0, 0, 0, 1}, virtual_key::vk_colon}, // SHIFT ";" = ":"
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "'" = """
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_less}, // SHIFT "," = "<"
        {virtual_key::vk_period, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "." = ">"
        {virtual_key::vk_slash, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "/" = "?"

        // SHIFT "a".."z" = "A".."Z"
        {virtual_key::vk_a, {0, 0, 0, 1}, virtual_key::vk_A},
        {virtual_key::vk_b, {0, 0, 0, 1}, virtual_key::vk_B},
        {virtual_key::vk_c, {0, 0, 0, 1}, virtual_key::vk_C},
        {virtual_key::vk_d, {0, 0, 0, 1}, virtual_key::vk_D},
        {virtual_key::vk_e, {0, 0, 0, 1}, virtual_key::vk_E},
        {virtual_key::vk_f, {0, 0, 0, 1}, virtual_key::vk_F},
        {virtual_key::vk_g, {0, 0, 0, 1}, virtual_key::vk_G},
        {virtual_key::vk_h, {0, 0, 0, 1}, virtual_key::vk_H},
        {virtual_key::vk_i, {0, 0, 0, 1}, virtual_key::vk_I},
        {virtual_key::vk_j, {0, 0, 0, 1}, virtual_key::vk_J},
        {virtual_key::vk_k, {0, 0, 0, 1}, virtual_key::vk_K},
        {virtual_key::vk_l, {0, 0, 0, 1}, virtual_key::vk_L},
        {virtual_key::vk_m, {0, 0, 0, 1}, virtual_key::vk_M},
        {virtual_key::vk_n, {0, 0, 0, 1}, virtual_key::vk_N},
        {virtual_key::vk_o, {0, 0, 0, 1}, virtual_key::vk_O},
        {virtual_key::vk_p, {0, 0, 0, 1}, virtual_key::vk_P},
        {virtual_key::vk_q, {0, 0, 0, 1}, virtual_key::vk_Q},
        {virtual_key::vk_r, {0, 0, 0, 1}, virtual_key::vk_R},
        {virtual_key::vk_s, {0, 0, 0, 1}, virtual_key::vk_S},
        {virtual_key::vk_t, {0, 0, 0, 1}, virtual_key::vk_T},
        {virtual_key::vk_u, {0, 0, 0, 1}, virtual_key::vk_U},
        {virtual_key::vk_v, {0, 0, 0, 1}, virtual_key::vk_V},
        {virtual_key::vk_w, {0, 0, 0, 1}, virtual_key::vk_W},
        {virtual_key::vk_x, {0, 0, 0, 1}, virtual_key::vk_X},
        {virtual_key::vk_y, {0, 0, 0, 1}, virtual_key::vk_Y},
        {virtual_key::vk_z, {0, 0, 0, 1}, virtual_key::vk_Z},

        // SHIFT "A".."Z" = "a".."z"
        {virtual_key::vk_A, {0, 0, 0, 1}, virtual_key::vk_a},
        {virtual_key::vk_B, {0, 0, 0, 1}, virtual_key::vk_b},
        {virtual_key::vk_C, {0, 0, 0, 1}, virtual_key::vk_c},
        {virtual_key::vk_D, {0, 0, 0, 1}, virtual_key::vk_d},
        {virtual_key::vk_E, {0, 0, 0, 1}, virtual_key::vk_e},
        {virtual_key::vk_F, {0, 0, 0, 1}, virtual_key::vk_f},
        {virtual_key::vk_G, {0, 0, 0, 1}, virtual_key::vk_g},
        {virtual_key::vk_H, {0, 0, 0, 1}, virtual_key::vk_h},
        {virtual_key::vk_I, {0, 0, 0, 1}, virtual_key::vk_i},
        {virtual_key::vk_J, {0, 0, 0, 1}, virtual_key::vk_j},
        {virtual_key::vk_K, {0, 0, 0, 1}, virtual_key::vk_k},
        {virtual_key::vk_L, {0, 0, 0, 1}, virtual_key::vk_l},
        {virtual_key::vk_M, {0, 0, 0, 1}, virtual_key::vk_m},
        {virtual_key::vk_N, {0, 0, 0, 1}, virtual_key::vk_n},
        {virtual_key::vk_O, {0, 0, 0, 1}, virtual_key::vk_o},
        {virtual_key::vk_P, {0, 0, 0, 1}, virtual_key::vk_p},
        {virtual_key::vk_Q, {0, 0, 0, 1}, virtual_key::vk_q},
        {virtual_key::vk_R, {0, 0, 0, 1}, virtual_key::vk_r},
        {virtual_key::vk_S, {0, 0, 0, 1}, virtual_key::vk_s},
        {virtual_key::vk_T, {0, 0, 0, 1}, virtual_key::vk_t},
        {virtual_key::vk_U, {0, 0, 0, 1}, virtual_key::vk_u},
        {virtual_key::vk_V, {0, 0, 0, 1}, virtual_key::vk_v},
        {virtual_key::vk_W, {0, 0, 0, 1}, virtual_key::vk_w},
        {virtual_key::vk_X, {0, 0, 0, 1}, virtual_key::vk_x},
        {virtual_key::vk_Y, {0, 0, 0, 1}, virtual_key::vk_y},
        {virtual_key::vk_Z, {0, 0, 0, 1}, virtual_key::vk_z},

    },

    // Мёртвые клавиши.
    {},

    // Мёртвая клавиша + клавиша.
    {},
};


/**
 * @brief Раскладка UK British — британская.
 */
const esp::ps2::keyboard_layout esp::ps2::UK_LAYOUT{
    // Имя.
    "UK",

    // Описание.
    "UK British",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x5D, virtual_key::vk_hash},
        {0x61, virtual_key::vk_backslash},
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_graveaccent, {0, 0, 0, 1}, virtual_key::vk_negation}, // SHIFT "`" = "¬"
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_3, {0, 0, 0, 1}, virtual_key::vk_pound}, // SHIFT "3" = "£"
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_at}, // SHIFT "'" = "@"
        {virtual_key::vk_hash, {0, 0, 0, 1}, virtual_key::vk_tilde}, // SHIFT "#" = "~"
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_verticalbar}, // SHIFT "\" = "|"
    },

    // Мёртвые клавиши.
    {},

    // Мёртвая клавиша + клавиша.
    {},
};


/**
 * @brief Раскладка German — немецкая.
 */
const esp::ps2::keyboard_layout esp::ps2::GERMAN_LAYOUT{
    // Имя.
    "DE",

    // Описание.
    "German",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_caret},
        {0x4E, virtual_key::vk_eszett},
        {0x55, virtual_key::vk_acuteaccent},
        {0x54, virtual_key::vk_umlaut_u},
        {0x5B, virtual_key::vk_plus},
        {0x4C, virtual_key::vk_umlaut_o},
        {0x52, virtual_key::vk_umlaut_a},
        {0x5D, virtual_key::vk_hash},
        {0x61, virtual_key::vk_less},
        {0x4A, virtual_key::vk_minus},
        {0x35, virtual_key::vk_z},
        {0x1A, virtual_key::vk_y},
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_caret, {0, 0, 0, 1}, virtual_key::vk_degree}, // SHIFT "^" = "°"
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_3, {0, 0, 0, 1}, virtual_key::vk_section}, // SHIFT "3" = "§"
        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "6" = "&"
        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT "7" = "/"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "8" = "("
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "9" = ")"
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_equals}, // SHIFT "0" = "="
        {virtual_key::vk_eszett, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "ß" = "?"
        {virtual_key::vk_acuteaccent, {0, 0, 0, 1}, virtual_key::vk_graveaccent}, // SHIFT "´" = "`"
        {virtual_key::vk_plus, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "+" = "*"
        {virtual_key::vk_hash, {0, 0, 0, 1}, virtual_key::vk_quote}, // SHIFT "#" = "'"
        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_semicolon}, // SHIFT "," = ";"
        {virtual_key::vk_period, {0, 0, 0, 1}, virtual_key::vk_colon}, // SHIFT "." = ":"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"
        {virtual_key::vk_7, {0, 0, 1, 0}, virtual_key::vk_leftbrace}, // ALT "7"   = "{"
        {virtual_key::vk_8, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // ALT "8"   = "["
        {virtual_key::vk_9, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // ALT "9"   = "]"
        {virtual_key::vk_0, {0, 0, 1, 0}, virtual_key::vk_rightbrace}, // ALT "0"   = "}"
        {virtual_key::vk_eszett, {0, 0, 1, 0}, virtual_key::vk_backslash}, // ALT "ß"   = "\"
        {virtual_key::vk_q, {0, 0, 1, 0}, virtual_key::vk_at}, // ALT "q"   = "@"
        {virtual_key::vk_e, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALT "e"   = "€"
        {virtual_key::vk_plus, {0, 0, 1, 0}, virtual_key::vk_tilde}, // ALT "+"   = "~"
        {virtual_key::vk_less, {0, 0, 1, 0}, virtual_key::vk_verticalbar}, // ALT "<"   = "|"
    },

    // Мёртвые клавиши.
    {},

    // Мёртвая клавиша + клавиша.
    {},
};


/**
 * @brief Раскладка Italian — итальянская.
 */
const esp::ps2::keyboard_layout esp::ps2::ITALIAN_LAYOUT{
    // Имя.
    "IT",

    // Описание.
    "Italian",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_backslash},
        {0x4E, virtual_key::vk_quote},
        {0x55, virtual_key::vk_grave_i},
        {0x54, virtual_key::vk_grave_e},
        {0x5B, virtual_key::vk_plus},
        {0x4C, virtual_key::vk_grave_o},
        {0x52, virtual_key::vk_grave_a},
        {0x5D, virtual_key::vk_grave_u},
        {0x61, virtual_key::vk_less},
        {0x4A, virtual_key::vk_minus},
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_verticalbar}, // SHIFT "\" = "|"
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_3, {0, 0, 0, 1}, virtual_key::vk_pound}, // SHIFT "3" = "£"
        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "6" = "&"
        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT "7" = "/"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "8" = "("
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "9" = ")"
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_equals}, // SHIFT "0" = "="
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "'" = "?"
        {virtual_key::vk_grave_i, {0, 0, 0, 1}, virtual_key::vk_caret}, // SHIFT "ì" = "^"
        {virtual_key::vk_grave_e, {0, 0, 0, 1}, virtual_key::vk_acute_e}, // SHIFT "è" = "é"
        {virtual_key::vk_plus, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "+" = "*"
        {virtual_key::vk_grave_o, {0, 0, 0, 1}, virtual_key::vk_cedilla_c}, // SHIFT "ò" = "ç"
        {virtual_key::vk_grave_a, {0, 0, 0, 1}, virtual_key::vk_degree}, // SHIFT "à" = "°"
        {virtual_key::vk_grave_u, {0, 0, 0, 1}, virtual_key::vk_section}, // SHIFT "ù" = "§"
        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_semicolon}, // SHIFT "," = ";"
        {virtual_key::vk_period, {0, 0, 0, 1}, virtual_key::vk_colon}, // SHIFT "." = ":"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"
        {virtual_key::vk_backslash, {0, 0, 1, 0}, virtual_key::vk_graveaccent}, // ALT "\"   = "`"
        {virtual_key::vk_5, {0, 0, 1, 0}, virtual_key::vk_tilde}, // ALT "5"   = "~"
        {virtual_key::vk_e, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALT "e"   = "€"
        {virtual_key::vk_grave_e, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // ALT "è"   = "["
        {virtual_key::vk_plus, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // ALT "+"   = "]"
        {virtual_key::vk_grave_o, {0, 0, 1, 0}, virtual_key::vk_at}, // ALT "ò"   = "@"
        {virtual_key::vk_grave_a, {0, 0, 1, 0}, virtual_key::vk_hash}, // ALT "à"   = "#"
        {virtual_key::vk_grave_e, {0, 0, 1, 1}, virtual_key::vk_leftbrace}, // SHIFT ALT "è" = "{"
        {virtual_key::vk_plus, {0, 0, 1, 1}, virtual_key::vk_rightbrace}, // SHIFT ALT "+" = "}"
    },

    // Мёртвые клавиши.
    {},

    // Мёртвая клавиша + клавиша.
    {},
};


/**
 * @brief Раскладка Spanish — испанская.
 *
 * Автор — Carles Oriol (https://github.com/carlesoriol).
 */
const esp::ps2::keyboard_layout esp::ps2::SPANISH_LAYOUT{
    // Имя.
    "ES",

    // Описание.
    "Spanish",

    // Унаследованная раскладка.
    &US_LAYOUT, // Чтобы не было конфликтующих сочетаний.

    // Однобайтовые скан-коды.
    {{0x0E, virtual_key::vk_degree},
     {0x4E, virtual_key::vk_quote},
     {0x55, virtual_key::vk_exclaim_inv}, // Перевёрнутый восклицательный знак.
     {0x5D, virtual_key::vk_cedilla_c},

     {0x54, virtual_key::vk_graveaccent},
     {0x5B, virtual_key::vk_plus},

     {0x4C, virtual_key::vk_tilde_n}, // Испанская ñ.
     {0x52, virtual_key::vk_acuteaccent},

     {0x61, virtual_key::vk_less},
     {0x4A, virtual_key::vk_minus}

    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},


    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {

        {virtual_key::vk_degree, {0, 0, 1, 0}, virtual_key::vk_backslash}, // ALT   "º" = "\"
        {virtual_key::vk_degree, {0, 0, 0, 1}, virtual_key::vk_upper_a}, // SHIFT "º" = "ª"

        {virtual_key::vk_1, {0, 0, 1, 0}, virtual_key::vk_verticalbar}, // ALT   "1" = "|"
        {virtual_key::vk_1, {0, 0, 0, 1}, virtual_key::vk_exclaim}, // SHIFT "1" = "!"

        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_2, {0, 0, 1, 0}, virtual_key::vk_at}, // ALT   "2" = "@"

        {virtual_key::vk_3, {0, 0, 0, 1}, virtual_key::vk_interpunct}, // SHIFT "3" = "·"
        {virtual_key::vk_3, {0, 0, 1, 0}, virtual_key::vk_hash}, // SHIFT "3" = "#"

        {virtual_key::vk_4, {0, 0, 1, 0}, virtual_key::vk_tilde}, // SHIFT "4" = "~"

        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "6" = "&"
        {virtual_key::vk_6, {0, 0, 1, 0}, virtual_key::vk_negation}, // ALT   "6" = "¬"

        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT "7" = "/"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "8" = "("
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "9" = ")"
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_equals}, // SHIFT "0" = "="

        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "'" = "?"
        {virtual_key::vk_exclaim_inv,
         {0, 0, 0, 1},
         virtual_key::vk_question_inv}, // SHIFT "¡" = "¿"

        {virtual_key::vk_graveaccent, {0, 0, 0, 1}, virtual_key::vk_caret}, // SHIFT "`" = "^"
        {virtual_key::vk_graveaccent, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // SHIFT "`" = "["

        {virtual_key::vk_plus, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "'" = "*"
        {virtual_key::vk_plus, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // SHIFT "'" = "]"

        {virtual_key::vk_cedilla_c, {0, 0, 0, 1}, virtual_key::vk_cedilla_C}, // SHIFT "ç" = "Ç"
        {virtual_key::vk_cedilla_c,
         {0, 0, 1, 0},
         virtual_key::vk_rightbrace}, // ALT   "ç" = "}" (должна быть прописной)

        {virtual_key::vk_acuteaccent,
         {0, 0, 0, 1},
         virtual_key::vk_diaeresis}, // ALT   "'" = "¨" (для гласных должен быть умляут)
        {virtual_key::vk_acuteaccent,
         {0, 0, 1, 0},
         virtual_key::vk_leftbrace}, // ALT   "'" = "{" //

        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_less, {0, 0, 1, 0}, virtual_key::vk_verticalbar}, // ALT   "<" = "|"

        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_semicolon}, // SHIFT "," = ";"
        {virtual_key::vk_period, {0, 0, 0, 1}, virtual_key::vk_colon}, // SHIFT "." = ":"

        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"

        {virtual_key::vk_tilde_n, {0, 0, 0, 1}, virtual_key::vk_tilde_N}, // SHIFT "ñ" = "Ñ"
    },

    // Мёртвые клавиши.
    {
        virtual_key::vk_graveaccent,
        virtual_key::vk_acuteaccent,
        virtual_key::vk_diaeresis,
        virtual_key::vk_caret,
    },

    // Мёртвая клавиша + клавиша.
    {
        {virtual_key::vk_graveaccent, virtual_key::vk_space, virtual_key::vk_graveaccent},
        {virtual_key::vk_acuteaccent, virtual_key::vk_space, virtual_key::vk_acuteaccent},
        {virtual_key::vk_diaeresis, virtual_key::vk_space, virtual_key::vk_diaeresis},
        {virtual_key::vk_caret, virtual_key::vk_space, virtual_key::vk_caret},

        {virtual_key::vk_graveaccent, virtual_key::vk_a, virtual_key::vk_grave_a},
        {virtual_key::vk_graveaccent, virtual_key::vk_e, virtual_key::vk_grave_e},
        {virtual_key::vk_graveaccent, virtual_key::vk_i, virtual_key::vk_grave_i},
        {virtual_key::vk_graveaccent, virtual_key::vk_o, virtual_key::vk_grave_o},
        {virtual_key::vk_graveaccent, virtual_key::vk_u, virtual_key::vk_grave_u},

        {virtual_key::vk_graveaccent, virtual_key::vk_A, virtual_key::vk_grave_A},
        {virtual_key::vk_graveaccent, virtual_key::vk_E, virtual_key::vk_grave_E},
        {virtual_key::vk_graveaccent, virtual_key::vk_I, virtual_key::vk_grave_I},
        {virtual_key::vk_graveaccent, virtual_key::vk_O, virtual_key::vk_grave_O},
        {virtual_key::vk_graveaccent, virtual_key::vk_U, virtual_key::vk_grave_U},

        {virtual_key::vk_acuteaccent, virtual_key::vk_a, virtual_key::vk_acute_a},
        {virtual_key::vk_acuteaccent, virtual_key::vk_e, virtual_key::vk_acute_e},
        {virtual_key::vk_acuteaccent, virtual_key::vk_i, virtual_key::vk_acute_i},
        {virtual_key::vk_acuteaccent, virtual_key::vk_o, virtual_key::vk_acute_o},
        {virtual_key::vk_acuteaccent, virtual_key::vk_u, virtual_key::vk_acute_u},

        {virtual_key::vk_acuteaccent, virtual_key::vk_A, virtual_key::vk_acute_A},
        {virtual_key::vk_acuteaccent, virtual_key::vk_E, virtual_key::vk_acute_E},
        {virtual_key::vk_acuteaccent, virtual_key::vk_I, virtual_key::vk_acute_I},
        {virtual_key::vk_acuteaccent, virtual_key::vk_O, virtual_key::vk_acute_O},
        {virtual_key::vk_acuteaccent, virtual_key::vk_U, virtual_key::vk_acute_U},

        {virtual_key::vk_caret, virtual_key::vk_a, virtual_key::vk_caret_a},
        {virtual_key::vk_caret, virtual_key::vk_e, virtual_key::vk_caret_e},
        {virtual_key::vk_caret, virtual_key::vk_i, virtual_key::vk_caret_i},
        {virtual_key::vk_caret, virtual_key::vk_o, virtual_key::vk_caret_o},
        {virtual_key::vk_caret, virtual_key::vk_u, virtual_key::vk_caret_u},

        {virtual_key::vk_caret, virtual_key::vk_A, virtual_key::vk_caret_A},
        {virtual_key::vk_caret, virtual_key::vk_E, virtual_key::vk_caret_E},
        {virtual_key::vk_caret, virtual_key::vk_I, virtual_key::vk_caret_I},
        {virtual_key::vk_caret, virtual_key::vk_O, virtual_key::vk_caret_O},
        {virtual_key::vk_caret, virtual_key::vk_U, virtual_key::vk_caret_U},

        {virtual_key::vk_diaeresis, virtual_key::vk_a, virtual_key::vk_umlaut_a},
        {virtual_key::vk_diaeresis, virtual_key::vk_e, virtual_key::vk_umlaut_e},
        {virtual_key::vk_diaeresis, virtual_key::vk_i, virtual_key::vk_umlaut_i},
        {virtual_key::vk_diaeresis, virtual_key::vk_o, virtual_key::vk_umlaut_o},
        {virtual_key::vk_diaeresis, virtual_key::vk_u, virtual_key::vk_umlaut_u},

        {virtual_key::vk_diaeresis, virtual_key::vk_A, virtual_key::vk_umlaut_A},
        {virtual_key::vk_diaeresis, virtual_key::vk_E, virtual_key::vk_umlaut_E},
        {virtual_key::vk_diaeresis, virtual_key::vk_I, virtual_key::vk_umlaut_I},
        {virtual_key::vk_diaeresis, virtual_key::vk_O, virtual_key::vk_umlaut_O},
        {virtual_key::vk_diaeresis, virtual_key::vk_U, virtual_key::vk_umlaut_U},
    },
};


/**
 * @brief Раскладка French — французская.
 *
 * Авторы — Alain Trentin и Denis Bodor (https://github.com/0xDRRB).
 */
const esp::ps2::keyboard_layout esp::ps2::FRENCH_LAYOUT{
    // Имя.
    "FR",

    // Описание.
    "French",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_square},      {0x16, virtual_key::vk_ampersand},
        {0x1E, virtual_key::vk_acute_e},     {0x26, virtual_key::vk_quotedbl},
        {0x25, virtual_key::vk_quote},       {0x2E, virtual_key::vk_leftparen},
        {0x36, virtual_key::vk_minus},       {0x3D, virtual_key::vk_grave_e},
        {0x3E, virtual_key::vk_underscore},  {0x46, virtual_key::vk_cedilla_c},
        {0x45, virtual_key::vk_grave_a},     {0x4E, virtual_key::vk_rightparen},
        {0x79, virtual_key::vk_equals},

        {0x15, virtual_key::vk_a},           {0x1D, virtual_key::vk_z},
        {0x54, virtual_key::vk_caret},       {0x5B, virtual_key::vk_dollar},

        {0x1C, virtual_key::vk_q},           {0x4C, virtual_key::vk_m},
        {0x52, virtual_key::vk_grave_u},     {0x5D, virtual_key::vk_asterisk},

        {0x61, virtual_key::vk_less},        {0x1A, virtual_key::vk_w},
        {0x3A, virtual_key::vk_comma},       {0x41, virtual_key::vk_semicolon},
        {0x49, virtual_key::vk_colon},       {0x4A, virtual_key::vk_exclaim},
        {0x55, virtual_key::vk_exclaim_inv},

    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_ampersand, {0, 0, 0, 1}, virtual_key::vk_1}, // SHIFT "&" = "1"
        {virtual_key::vk_acute_e, {0, 0, 0, 1}, virtual_key::vk_2}, // SHIFT "é" = "2"
        {virtual_key::vk_quotedbl, {0, 0, 0, 1}, virtual_key::vk_3}, // SHIFT """ = "3"
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_4}, // SHIFT "'" = "4"
        {virtual_key::vk_leftparen, {0, 0, 0, 1}, virtual_key::vk_5}, // SHIFT "(" = "5"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_6}, // SHIFT "-" = "6"
        {virtual_key::vk_grave_e, {0, 0, 0, 1}, virtual_key::vk_7}, // SHIFT "è" = "7"
        {virtual_key::vk_underscore, {0, 0, 0, 1}, virtual_key::vk_8}, // SHIFT "_" = "8"
        {virtual_key::vk_cedilla_c, {0, 0, 0, 1}, virtual_key::vk_9}, // SHIFT "ç" = "9"
        {virtual_key::vk_grave_a, {0, 0, 0, 1}, virtual_key::vk_0}, // SHIFT "à" = "0"
        {virtual_key::vk_rightparen, {0, 0, 0, 1}, virtual_key::vk_degree}, // SHIFT ")" = "°"
        {virtual_key::vk_equals, {0, 0, 0, 1}, virtual_key::vk_plus}, // SHIFT "=" = "+"
        {virtual_key::vk_ampersand, {0, 0, 1, 0}, virtual_key::vk_none}, // ALTGR "&" = ""
        {virtual_key::vk_acute_e, {0, 0, 1, 0}, virtual_key::vk_tilde}, // ALTGR "é" = "~"
        {virtual_key::vk_quotedbl, {0, 0, 1, 0}, virtual_key::vk_hash}, // ALTGR """ = "#"
        {virtual_key::vk_quote, {0, 0, 1, 0}, virtual_key::vk_leftbrace}, // ALTGR "'" = "{"
        {virtual_key::vk_leftparen, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // ALTGR "(" = "["
        {virtual_key::vk_minus, {0, 0, 1, 0}, virtual_key::vk_verticalbar}, // ALTGR "-" = "|"
        {virtual_key::vk_grave_e, {0, 0, 1, 0}, virtual_key::vk_graveaccent}, // ALTGR "è" = "`"
        {virtual_key::vk_underscore, {0, 0, 1, 0}, virtual_key::vk_backslash}, // ALTGR "_" = "\"
        {virtual_key::vk_cedilla_c, {0, 0, 1, 0}, virtual_key::vk_caret}, // ALTGR "ç" = "^"
        {virtual_key::vk_grave_a, {0, 0, 1, 0}, virtual_key::vk_at}, // ALTGR "à" = "@"
        {virtual_key::vk_rightparen, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // ALTGR ")" = "]"
        {virtual_key::vk_equals, {0, 0, 1, 0}, virtual_key::vk_rightbrace}, // ALTGR "=" = "}"
        {virtual_key::vk_caret, {0, 0, 0, 1}, virtual_key::vk_diaeresis}, // SHIFT "^" = "¨"
        {virtual_key::vk_dollar, {0, 0, 0, 1}, virtual_key::vk_pound}, // SHIFT "$" = "£"
        {virtual_key::vk_dollar, {0, 0, 1, 0}, virtual_key::vk_currency}, // ALTGR "$" = "¤"
        {virtual_key::vk_grave_u, {0, 0, 0, 1}, virtual_key::vk_percent}, // SHIFT "ù" = "%"
        {virtual_key::vk_asterisk, {0, 0, 0, 1}, virtual_key::vk_mu}, // SHIFT "*" = "µ"
        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "," = "?"
        {virtual_key::vk_semicolon, {0, 0, 0, 1}, virtual_key::vk_period}, // SHIFT ";" = "."
        {virtual_key::vk_colon, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT ":" = "/"
        {virtual_key::vk_exclaim, {0, 0, 0, 1}, virtual_key::vk_section}, // SHIFT "!" = "§"
        {virtual_key::vk_e, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALTGR "e" = "€"
    },

    // Мёртвые клавиши.
    {virtual_key::vk_caret, virtual_key::vk_diaeresis},

    // Мёртвая клавиша + клавиша.
    {
        {virtual_key::vk_caret, virtual_key::vk_a, virtual_key::vk_caret_a}, // "â"
        {virtual_key::vk_caret, virtual_key::vk_e, virtual_key::vk_caret_e}, // "ê"
        {virtual_key::vk_caret, virtual_key::vk_i, virtual_key::vk_caret_i}, // "î"
        {virtual_key::vk_caret, virtual_key::vk_o, virtual_key::vk_caret_o}, // "ô"
        {virtual_key::vk_caret, virtual_key::vk_u, virtual_key::vk_caret_u}, // "û"

        {virtual_key::vk_caret, virtual_key::vk_A, virtual_key::vk_caret_A}, // "Â"
        {virtual_key::vk_caret, virtual_key::vk_E, virtual_key::vk_caret_E}, // "Ê"
        {virtual_key::vk_caret, virtual_key::vk_I, virtual_key::vk_caret_I}, // "Î"
        {virtual_key::vk_caret, virtual_key::vk_O, virtual_key::vk_caret_O}, // "Ô"
        {virtual_key::vk_caret, virtual_key::vk_U, virtual_key::vk_caret_U}, // "Û"

        {virtual_key::vk_diaeresis, virtual_key::vk_a, virtual_key::vk_umlaut_a}, // "ä"
        {virtual_key::vk_diaeresis, virtual_key::vk_e, virtual_key::vk_umlaut_e}, // "ë"
        {virtual_key::vk_diaeresis, virtual_key::vk_i, virtual_key::vk_umlaut_i}, // "ï"
        {virtual_key::vk_diaeresis, virtual_key::vk_o, virtual_key::vk_umlaut_o}, // "ö"
        {virtual_key::vk_diaeresis, virtual_key::vk_u, virtual_key::vk_umlaut_u}, // "ü"

        {virtual_key::vk_diaeresis, virtual_key::vk_A, virtual_key::vk_umlaut_A}, // "Ä"
        {virtual_key::vk_diaeresis, virtual_key::vk_E, virtual_key::vk_umlaut_E}, // "Ë"
        {virtual_key::vk_diaeresis, virtual_key::vk_I, virtual_key::vk_umlaut_I}, // "Ï"
        {virtual_key::vk_diaeresis, virtual_key::vk_O, virtual_key::vk_umlaut_O}, // "Ö"
        {virtual_key::vk_diaeresis, virtual_key::vk_U, virtual_key::vk_umlaut_U} // "Ü"
    },
};


/**
 * @brief Раскладка Belgian — бельгийская.
 *
 * Автор — Michel Bernard (https://github.com/GmEsoft).
 */
const esp::ps2::keyboard_layout esp::ps2::BELGIAN_LAYOUT{
    // Имя.
    "BE",

    // Описание.
    "Belgian",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_square}, // Вторая степень.
        {0x16, virtual_key::vk_ampersand}, // 1 -> &
        {0x1E, virtual_key::vk_acute_e}, // 2 -> é
        {0x26, virtual_key::vk_quotedbl}, // 3 -> "
        {0x25, virtual_key::vk_quote}, // 4 -> '
        {0x2E, virtual_key::vk_leftparen}, // 5 -> (
        {0x36, virtual_key::vk_section}, // 6 -> §
        {0x3D, virtual_key::vk_grave_e}, // 7 -> è
        {0x3E, virtual_key::vk_exclaim}, // 8 -> !
        {0x46, virtual_key::vk_cedilla_c}, // 9 -> ç
        {0x45, virtual_key::vk_grave_a}, // 0 -> à

        {0x15, virtual_key::vk_a}, // q -> a
        {0x1D, virtual_key::vk_z}, // w -> z
        {0x1C, virtual_key::vk_q}, // a -> q
        {0x1A, virtual_key::vk_w}, // z -> w
        {0x3A, virtual_key::vk_comma}, // m -> ,

        {0x4E, virtual_key::vk_rightparen}, // - -> )
        {0x55, virtual_key::vk_minus}, // = -> -
        {0x54, virtual_key::vk_caret}, // [ -> ^ (мёртвая)
        {0x5B, virtual_key::vk_dollar}, // ] -> $

        {0x5D, virtual_key::vk_mu}, // \ -> mu
        {0x61, virtual_key::vk_less}, // UK \ -> <

        {0x4C, virtual_key::vk_m}, // ; -> m
        {0x52, virtual_key::vk_grave_u}, // ' -> ù
        {0x41, virtual_key::vk_semicolon}, // , -> ;
        {0x49, virtual_key::vk_colon}, // . -> :
        {0x4A, virtual_key::vk_equals}, // / -> =

        {0x00, virtual_key::vk_none} // END MARKER
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {
        {0x00, virtual_key::vk_none} // END MARKER
    },

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_ampersand, {0, 0, 0, 1}, virtual_key::vk_1}, // SHIFT "&" = "1"
        {virtual_key::vk_acute_e, {0, 0, 0, 1}, virtual_key::vk_2}, // SHIFT "é" = "2"
        {virtual_key::vk_quotedbl, {0, 0, 0, 1}, virtual_key::vk_3}, // SHIFT """ = "3"
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_4}, // SHIFT "'" = "4"
        {virtual_key::vk_leftparen, {0, 0, 0, 1}, virtual_key::vk_5}, // SHIFT "(" = "5"
        {virtual_key::vk_section, {0, 0, 0, 1}, virtual_key::vk_6}, // SHIFT "§" = "6"
        {virtual_key::vk_grave_e, {0, 0, 0, 1}, virtual_key::vk_7}, // SHIFT "è" = "7"
        {virtual_key::vk_exclaim, {0, 0, 0, 1}, virtual_key::vk_8}, // SHIFT "!" = "8"
        {virtual_key::vk_cedilla_c, {0, 0, 0, 1}, virtual_key::vk_9}, // SHIFT "ç" = "9"
        {virtual_key::vk_grave_a, {0, 0, 0, 1}, virtual_key::vk_0}, // SHIFT "à" = "0"

        {virtual_key::vk_square,
         {0, 0, 0, 1},
         virtual_key::vk_hash}, // SHIFT "@" = "#" (Apple) or Cube (tbd)
        {virtual_key::vk_rightparen, {0, 0, 0, 1}, virtual_key::vk_degree}, // SHIFT ")" = "°"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"
        {virtual_key::vk_caret, {0, 0, 0, 1}, virtual_key::vk_diaeresis}, // SHIFT "^" = "¨"
        {virtual_key::vk_dollar, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "$" = "*"
        {virtual_key::vk_grave_u, {0, 0, 0, 1}, virtual_key::vk_percent}, // SHIFT "ù" = "%"
        {virtual_key::vk_eszett, {0, 0, 0, 1}, virtual_key::vk_pound}, // SHIFT "`" = "£"
        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "," = "?"
        {virtual_key::vk_semicolon, {0, 0, 0, 1}, virtual_key::vk_period}, // SHIFT ";" = "."
        {virtual_key::vk_colon, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT ":" = "/"
        {virtual_key::vk_equals, {0, 0, 0, 1}, virtual_key::vk_plus}, // SHIFT "=" = "+"

        {virtual_key::vk_ampersand, {0, 0, 1, 0}, virtual_key::vk_verticalbar}, // ALTGR "&" = "|"
        {virtual_key::vk_acute_e, {0, 0, 1, 0}, virtual_key::vk_at}, // ALTGR "é" = "@"
        {virtual_key::vk_quotedbl, {0, 0, 1, 0}, virtual_key::vk_hash}, // ALTGR """ = "#"
        {virtual_key::vk_quote,
         {0, 0, 1, 0},
         virtual_key::vk_leftbrace}, // ALTGR "'" = "{" (add-on)
        {virtual_key::vk_leftparen,
         {0, 0, 1, 0},
         virtual_key::vk_leftbracket}, // ALTGR "(" = "[" (add-on)
        {virtual_key::vk_section,
         {0, 0, 1, 0},
         virtual_key::vk_caret}, // ALTGR "§" = "^" (не должна быть мёртвой)
        {virtual_key::vk_cedilla_c, {0, 0, 1, 0}, virtual_key::vk_leftbrace}, // ALTGR "ç" = "{"
        {virtual_key::vk_grave_a, {0, 0, 1, 0}, virtual_key::vk_rightbrace}, // ALTGR "à" = "}"
        {virtual_key::vk_rightparen,
         {0, 0, 1, 0},
         virtual_key::vk_rightbracket}, // ALTGR "à" = "]" (add-on)
        {virtual_key::vk_grave_a,
         {0, 0, 1, 0},
         virtual_key::vk_rightbrace}, // ALTGR "-" = "}" (add-on)

        {virtual_key::vk_e, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALTGR "e" = "€"
        {virtual_key::vk_caret, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // ALTGR "^" = "["
        {virtual_key::vk_dollar, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // ALTGR "$" = "]"
        {virtual_key::vk_grave_u, {0, 0, 1, 0}, virtual_key::vk_acuteaccent}, // ALTGR "ù" = "´"
        {virtual_key::vk_mu, {0, 0, 1, 0}, virtual_key::vk_graveaccent}, // ALTGR "mu" = "`"
        {virtual_key::vk_less, {0, 0, 1, 0}, virtual_key::vk_backslash}, // ALTGR "<" = "\"
        {virtual_key::vk_equals, {0, 0, 1, 0}, virtual_key::vk_tilde}, // ALTGR "=" = "~"

        {virtual_key::vk_none, {0, 0, 0, 0}, virtual_key::vk_none} // END MARKER
    },

    // Мёртвые клавиши.
    {
        virtual_key::vk_graveaccent, // `
        virtual_key::vk_acuteaccent, // ´
        virtual_key::vk_diaeresis, // ¨
        virtual_key::vk_caret, // ^
        virtual_key::vk_tilde, // ~
        virtual_key::vk_none // END MARKER
    },

    // Мёртвая клавиша + клавиша.
    {
        {virtual_key::vk_graveaccent, virtual_key::vk_space, virtual_key::vk_graveaccent},
        {virtual_key::vk_acuteaccent, virtual_key::vk_space, virtual_key::vk_acuteaccent},
        {virtual_key::vk_diaeresis, virtual_key::vk_space, virtual_key::vk_diaeresis},
        {virtual_key::vk_caret, virtual_key::vk_space, virtual_key::vk_caret},
        {virtual_key::vk_tilde, virtual_key::vk_space, virtual_key::vk_tilde},

        {virtual_key::vk_tilde, virtual_key::vk_n, virtual_key::vk_tilde_n},
        {virtual_key::vk_tilde, virtual_key::vk_N, virtual_key::vk_tilde_N},
        {virtual_key::vk_tilde, virtual_key::vk_a, virtual_key::vk_tilde_a},
        {virtual_key::vk_tilde, virtual_key::vk_A, virtual_key::vk_tilde_A},
        {virtual_key::vk_tilde, virtual_key::vk_o, virtual_key::vk_tilde_o},
        {virtual_key::vk_tilde, virtual_key::vk_O, virtual_key::vk_tilde_O},

        {virtual_key::vk_graveaccent, virtual_key::vk_a, virtual_key::vk_grave_a},
        {virtual_key::vk_graveaccent, virtual_key::vk_e, virtual_key::vk_grave_e},
        {virtual_key::vk_graveaccent, virtual_key::vk_i, virtual_key::vk_grave_i},
        {virtual_key::vk_graveaccent, virtual_key::vk_o, virtual_key::vk_grave_o},
        {virtual_key::vk_graveaccent, virtual_key::vk_u, virtual_key::vk_grave_u},
        {virtual_key::vk_graveaccent, virtual_key::vk_y, virtual_key::vk_grave_y},

        {virtual_key::vk_graveaccent, virtual_key::vk_A, virtual_key::vk_grave_A},
        {virtual_key::vk_graveaccent, virtual_key::vk_E, virtual_key::vk_grave_E},
        {virtual_key::vk_graveaccent, virtual_key::vk_I, virtual_key::vk_grave_I},
        {virtual_key::vk_graveaccent, virtual_key::vk_O, virtual_key::vk_grave_O},
        {virtual_key::vk_graveaccent, virtual_key::vk_U, virtual_key::vk_grave_U},
        {virtual_key::vk_graveaccent, virtual_key::vk_Y, virtual_key::vk_grave_Y},

        {virtual_key::vk_acuteaccent, virtual_key::vk_a, virtual_key::vk_acute_a},
        {virtual_key::vk_acuteaccent, virtual_key::vk_e, virtual_key::vk_acute_e},
        {virtual_key::vk_acuteaccent, virtual_key::vk_i, virtual_key::vk_acute_i},
        {virtual_key::vk_acuteaccent, virtual_key::vk_o, virtual_key::vk_acute_o},
        {virtual_key::vk_acuteaccent, virtual_key::vk_u, virtual_key::vk_acute_u},
        {virtual_key::vk_acuteaccent, virtual_key::vk_y, virtual_key::vk_acute_y},

        {virtual_key::vk_acuteaccent, virtual_key::vk_A, virtual_key::vk_acute_A},
        {virtual_key::vk_acuteaccent, virtual_key::vk_E, virtual_key::vk_acute_E},
        {virtual_key::vk_acuteaccent, virtual_key::vk_I, virtual_key::vk_acute_I},
        {virtual_key::vk_acuteaccent, virtual_key::vk_O, virtual_key::vk_acute_O},
        {virtual_key::vk_acuteaccent, virtual_key::vk_U, virtual_key::vk_acute_U},
        {virtual_key::vk_acuteaccent, virtual_key::vk_Y, virtual_key::vk_acute_Y},

        {virtual_key::vk_caret, virtual_key::vk_a, virtual_key::vk_caret_a},
        {virtual_key::vk_caret, virtual_key::vk_e, virtual_key::vk_caret_e},
        {virtual_key::vk_caret, virtual_key::vk_i, virtual_key::vk_caret_i},
        {virtual_key::vk_caret, virtual_key::vk_o, virtual_key::vk_caret_o},
        {virtual_key::vk_caret, virtual_key::vk_u, virtual_key::vk_caret_u},
        {virtual_key::vk_caret, virtual_key::vk_y, virtual_key::vk_caret_y},

        {virtual_key::vk_caret, virtual_key::vk_A, virtual_key::vk_caret_A},
        {virtual_key::vk_caret, virtual_key::vk_E, virtual_key::vk_caret_E},
        {virtual_key::vk_caret, virtual_key::vk_I, virtual_key::vk_caret_I},
        {virtual_key::vk_caret, virtual_key::vk_O, virtual_key::vk_caret_O},
        {virtual_key::vk_caret, virtual_key::vk_U, virtual_key::vk_caret_U},
        {virtual_key::vk_caret, virtual_key::vk_Y, virtual_key::vk_caret_Y},

        {virtual_key::vk_diaeresis, virtual_key::vk_a, virtual_key::vk_umlaut_a},
        {virtual_key::vk_diaeresis, virtual_key::vk_e, virtual_key::vk_umlaut_e},
        {virtual_key::vk_diaeresis, virtual_key::vk_i, virtual_key::vk_umlaut_i},
        {virtual_key::vk_diaeresis, virtual_key::vk_o, virtual_key::vk_umlaut_o},
        {virtual_key::vk_diaeresis, virtual_key::vk_u, virtual_key::vk_umlaut_u},
        {virtual_key::vk_diaeresis, virtual_key::vk_y, virtual_key::vk_umlaut_y},

        {virtual_key::vk_diaeresis, virtual_key::vk_A, virtual_key::vk_umlaut_A},
        {virtual_key::vk_diaeresis, virtual_key::vk_E, virtual_key::vk_umlaut_E},
        {virtual_key::vk_diaeresis, virtual_key::vk_I, virtual_key::vk_umlaut_I},
        {virtual_key::vk_diaeresis, virtual_key::vk_O, virtual_key::vk_umlaut_O},
        {virtual_key::vk_diaeresis, virtual_key::vk_U, virtual_key::vk_umlaut_U},
        {virtual_key::vk_diaeresis, virtual_key::vk_Y, virtual_key::vk_umlaut_Y},

        {virtual_key::vk_none, virtual_key::vk_none, virtual_key::vk_none} // END MARKER

    },
};


/**
 * @brief Раскладка Norwegian — норвежская.
 *
 * Автор — Knut Zakariassen (https://github.com/zakariassen).
 */
const esp::ps2::keyboard_layout esp::ps2::NORWEGIAN_LAYOUT{
    // Имя.
    "NO",

    // Описание.
    "Norwegian",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_verticalbar}, //
        {0x4E, virtual_key::vk_plus}, //
        {0x55, virtual_key::vk_backslash}, //
        {0x54, virtual_key::vk_aring}, // å
        {0x5B, virtual_key::vk_caret}, //
        {0x4C, virtual_key::vk_oslash}, // ø
        {0x52, virtual_key::vk_aelig}, // æ
        {0x5D, virtual_key::vk_quote}, //
        {0x61, virtual_key::vk_less}, //
        {0x4A, virtual_key::vk_minus}, //

    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_verticalbar, {0, 0, 0, 1}, virtual_key::vk_section}, // SHIFT "|" = "§"
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_4, {0, 0, 0, 1}, virtual_key::vk_currency}, // SHIFT "4" = "¤"
        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "6" = "&"
        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_slash}, // SHIFT "7" = "/"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "8" = "("
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "9" = ")"
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_equals}, // SHIFT "0" = "="
        {virtual_key::vk_plus, {0, 0, 0, 1}, virtual_key::vk_question}, // SHIFT "+" = "?"
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_graveaccent}, // SHIFT "\" = "`"
        {virtual_key::vk_aring, {0, 0, 0, 1}, virtual_key::vk_ARING}, // SHIFT "å" = "Å"
        {virtual_key::vk_diaeresis, {0, 0, 0, 1}, virtual_key::vk_caret}, // SHIFT "¨" = "^"
        {virtual_key::vk_oslash, {0, 0, 0, 1}, virtual_key::vk_OSLASH}, // SHIFT "ø" = "Ø"
        {virtual_key::vk_aelig, {0, 0, 0, 1}, virtual_key::vk_AELIG}, // SHIFT "æ" = "Æ"
        {virtual_key::vk_quote, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT "'" = "*"
        {virtual_key::vk_less, {0, 0, 0, 1}, virtual_key::vk_greater}, // SHIFT "<" = ">"
        {virtual_key::vk_comma, {0, 0, 0, 1}, virtual_key::vk_semicolon}, // SHIFT "," = ";"
        {virtual_key::vk_period, {0, 0, 0, 1}, virtual_key::vk_colon}, // SHIFT "." = ":"
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "-" = "_"
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_graveaccent}, // SHIFT "\" = "`"

        {virtual_key::vk_2, {0, 0, 1, 0}, virtual_key::vk_at}, // ALTGR "2" = "@"
        {virtual_key::vk_3, {0, 0, 1, 0}, virtual_key::vk_pound}, // ALTGR "3" = "£"
        {virtual_key::vk_4, {0, 0, 1, 0}, virtual_key::vk_dollar}, // ALTGR "4" = "$"
        {virtual_key::vk_5, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALTGR "5" = "€"
        {virtual_key::vk_7, {0, 0, 1, 0}, virtual_key::vk_leftbrace}, // ALTGR "7" = "{"
        {virtual_key::vk_8, {0, 0, 1, 0}, virtual_key::vk_leftbracket}, // ALTGR "8" = "["
        {virtual_key::vk_9, {0, 0, 1, 0}, virtual_key::vk_rightbracket}, // ALTGR "9" = "]"
        {virtual_key::vk_0, {0, 0, 1, 0}, virtual_key::vk_rightbrace}, // ALTGR "0" = "}"
        {virtual_key::vk_backslash, {0, 0, 1, 0}, virtual_key::vk_acuteaccent}, // ALTGR "\" = "´"
        {virtual_key::vk_diaeresis, {0, 0, 1, 0}, virtual_key::vk_tilde}, // ALTGR "¨" = "~"
        {virtual_key::vk_e, {0, 0, 1, 0}, virtual_key::vk_euro}, // ALTGR "e" = "€"
        {virtual_key::vk_m, {0, 0, 1, 0}, virtual_key::vk_mu}, // ALTGR "m" = "µ"

    },

    // Мёртвые клавиши.
    {
        virtual_key::vk_graveaccent, // `
        virtual_key::vk_acuteaccent, // ´
        virtual_key::vk_diaeresis, // ¨
        virtual_key::vk_caret, // ^
        virtual_key::vk_tilde, // ~
        virtual_key::vk_none // END MARKER
    },

    // Мёртвая клавиша + клавиша.
    {
        {virtual_key::vk_graveaccent, virtual_key::vk_space, virtual_key::vk_graveaccent},
        {virtual_key::vk_acuteaccent, virtual_key::vk_space, virtual_key::vk_acuteaccent},
        {virtual_key::vk_diaeresis, virtual_key::vk_space, virtual_key::vk_diaeresis},
        {virtual_key::vk_caret, virtual_key::vk_space, virtual_key::vk_caret},
        {virtual_key::vk_tilde, virtual_key::vk_space, virtual_key::vk_tilde},

        {virtual_key::vk_graveaccent, virtual_key::vk_a, virtual_key::vk_grave_a},
        {virtual_key::vk_graveaccent, virtual_key::vk_e, virtual_key::vk_grave_e},
        {virtual_key::vk_graveaccent, virtual_key::vk_i, virtual_key::vk_grave_i},
        {virtual_key::vk_graveaccent, virtual_key::vk_o, virtual_key::vk_grave_o},
        {virtual_key::vk_graveaccent, virtual_key::vk_u, virtual_key::vk_grave_u},

        {virtual_key::vk_graveaccent, virtual_key::vk_A, virtual_key::vk_grave_A},
        {virtual_key::vk_graveaccent, virtual_key::vk_E, virtual_key::vk_grave_E},
        {virtual_key::vk_graveaccent, virtual_key::vk_I, virtual_key::vk_grave_I},
        {virtual_key::vk_graveaccent, virtual_key::vk_O, virtual_key::vk_grave_O},
        {virtual_key::vk_graveaccent, virtual_key::vk_U, virtual_key::vk_grave_U},

        {virtual_key::vk_acuteaccent, virtual_key::vk_a, virtual_key::vk_acute_a},
        {virtual_key::vk_acuteaccent, virtual_key::vk_e, virtual_key::vk_acute_e},
        {virtual_key::vk_acuteaccent, virtual_key::vk_i, virtual_key::vk_acute_i},
        {virtual_key::vk_acuteaccent, virtual_key::vk_o, virtual_key::vk_acute_o},
        {virtual_key::vk_acuteaccent, virtual_key::vk_u, virtual_key::vk_acute_u},
        {virtual_key::vk_acuteaccent, virtual_key::vk_y, virtual_key::vk_acute_y},

        {virtual_key::vk_acuteaccent, virtual_key::vk_A, virtual_key::vk_acute_A},
        {virtual_key::vk_acuteaccent, virtual_key::vk_E, virtual_key::vk_acute_E},
        {virtual_key::vk_acuteaccent, virtual_key::vk_I, virtual_key::vk_acute_I},
        {virtual_key::vk_acuteaccent, virtual_key::vk_O, virtual_key::vk_acute_O},
        {virtual_key::vk_acuteaccent, virtual_key::vk_U, virtual_key::vk_acute_U},
        {virtual_key::vk_acuteaccent, virtual_key::vk_Y, virtual_key::vk_acute_Y},

        {virtual_key::vk_diaeresis, virtual_key::vk_a, virtual_key::vk_umlaut_a},
        {virtual_key::vk_diaeresis, virtual_key::vk_e, virtual_key::vk_umlaut_e},
        {virtual_key::vk_diaeresis, virtual_key::vk_i, virtual_key::vk_umlaut_i},
        {virtual_key::vk_diaeresis, virtual_key::vk_o, virtual_key::vk_umlaut_o},
        {virtual_key::vk_diaeresis, virtual_key::vk_u, virtual_key::vk_umlaut_u},
        {virtual_key::vk_diaeresis, virtual_key::vk_y, virtual_key::vk_umlaut_y},

        {virtual_key::vk_diaeresis, virtual_key::vk_A, virtual_key::vk_umlaut_A},
        {virtual_key::vk_diaeresis, virtual_key::vk_E, virtual_key::vk_umlaut_E},
        {virtual_key::vk_diaeresis, virtual_key::vk_I, virtual_key::vk_umlaut_I},
        {virtual_key::vk_diaeresis, virtual_key::vk_O, virtual_key::vk_umlaut_O},
        {virtual_key::vk_diaeresis, virtual_key::vk_U, virtual_key::vk_umlaut_U},
        {virtual_key::vk_diaeresis, virtual_key::vk_Y, virtual_key::vk_umlaut_Y},

        {virtual_key::vk_caret, virtual_key::vk_a, virtual_key::vk_caret_a},
        {virtual_key::vk_caret, virtual_key::vk_e, virtual_key::vk_caret_e},
        {virtual_key::vk_caret, virtual_key::vk_i, virtual_key::vk_caret_i},
        {virtual_key::vk_caret, virtual_key::vk_o, virtual_key::vk_caret_o},
        {virtual_key::vk_caret, virtual_key::vk_u, virtual_key::vk_caret_u},

        {virtual_key::vk_caret, virtual_key::vk_A, virtual_key::vk_caret_A},
        {virtual_key::vk_caret, virtual_key::vk_E, virtual_key::vk_caret_E},
        {virtual_key::vk_caret, virtual_key::vk_I, virtual_key::vk_caret_I},
        {virtual_key::vk_caret, virtual_key::vk_O, virtual_key::vk_caret_O},
        {virtual_key::vk_caret, virtual_key::vk_U, virtual_key::vk_caret_U},

        {virtual_key::vk_tilde, virtual_key::vk_n, virtual_key::vk_tilde_n},
        {virtual_key::vk_tilde, virtual_key::vk_N, virtual_key::vk_tilde_N},
        {virtual_key::vk_tilde, virtual_key::vk_a, virtual_key::vk_tilde_a},
        {virtual_key::vk_tilde, virtual_key::vk_A, virtual_key::vk_tilde_A},
        {virtual_key::vk_tilde, virtual_key::vk_o, virtual_key::vk_tilde_o},
        {virtual_key::vk_tilde, virtual_key::vk_O, virtual_key::vk_tilde_O},

        {virtual_key::vk_none, virtual_key::vk_none, virtual_key::vk_none} // END MARKER
    },
};


/**
 * @brief Раскладка Japanese — японская.
 *
 * Основана на https://github.com/bellTanTan/fabglKeyboardJapaneseLayout.
 */
const esp::ps2::keyboard_layout esp::ps2::JAPANESE_LAYOUT{
    // Имя.
    "JP",

    // Описание.
    "Japanese",

    // Унаследованная раскладка.
    &US_LAYOUT,

    // Однобайтовые скан-коды.
    {
        {0x0E, virtual_key::vk_hankaku_zenkaku_kanji},
        {0x55, virtual_key::vk_caret},
        {0x54, virtual_key::vk_at},
        {0x5B, virtual_key::vk_leftbracket},
        {0x5D, virtual_key::vk_rightbracket},
        {0x52, virtual_key::vk_colon},
        {0x6A, virtual_key::vk_yen},
        {0x51, virtual_key::vk_backslash},
        {0x67, virtual_key::vk_muhenkan},
        {0x64, virtual_key::vk_henkan},
        {0x13, virtual_key::vk_katakana_hiragana_romaji},
    },

    // Расширенные скан-коды (с префиксом 0xE0).
    {},

    // Клавиши, получаемые с модификаторами.
    // Клавиша, {CTRL, LALT, RALT, SHIFT}, результат.
    {
        {virtual_key::vk_2, {0, 0, 0, 1}, virtual_key::vk_quotedbl}, // SHIFT "2" = """
        {virtual_key::vk_6, {0, 0, 0, 1}, virtual_key::vk_ampersand}, // SHIFT "6" = "&"
        {virtual_key::vk_7, {0, 0, 0, 1}, virtual_key::vk_quote}, // SHIFT "7" = "'"
        {virtual_key::vk_8, {0, 0, 0, 1}, virtual_key::vk_leftparen}, // SHIFT "8" = "("
        {virtual_key::vk_9, {0, 0, 0, 1}, virtual_key::vk_rightparen}, // SHIFT "9" = ")"
        {virtual_key::vk_0, {0, 0, 0, 1}, virtual_key::vk_shift_0}, // SHIFT "0" = ""
        {virtual_key::vk_minus, {0, 0, 0, 1}, virtual_key::vk_equals}, // SHIFT "-" = "="
        {virtual_key::vk_caret, {0, 0, 0, 1}, virtual_key::vk_tilde}, // SHIFT "^" = "~"
        {virtual_key::vk_yen, {0, 0, 0, 1}, virtual_key::vk_verticalbar}, // SHIFT "￥" = "|"
        {virtual_key::vk_at, {0, 0, 0, 1}, virtual_key::vk_graveaccent}, // SHIFT "@" = "`"
        {virtual_key::vk_semicolon, {0, 0, 0, 1}, virtual_key::vk_plus}, // SHIFT ";" = "+"
        {virtual_key::vk_colon, {0, 0, 0, 1}, virtual_key::vk_asterisk}, // SHIFT ":" = "*"
        {virtual_key::vk_backslash, {0, 0, 0, 1}, virtual_key::vk_underscore}, // SHIFT "\" = "_"
    },

    // Мёртвые клавиши.
    {},

    // Мёртвая клавиша + клавиша.
    {},
};
