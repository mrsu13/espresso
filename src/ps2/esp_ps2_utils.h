// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains some utility classes and functions
 *
 */


#include "esp_tools.h"


namespace esp::ps2
{


// Integer square root by Halleck's method, with Legalize's speedup
int isqrt(int x);


template<typename T>
T move_items(T dest, T src, size_t n)
{
    T pd = dest;
    T ps = src;
    if (pd != ps)
    {
        if (ps < pd)
            for (pd += n, ps += n; n--;)
                *--pd = *--ps;
        else
            while (n--)
                *pd++ = *ps++;
    }
    return dest;
}


/**
 * @brief Describes mouse buttons status.
 */
struct mouse_buttons
{
    uint8_t left : 1; /**< Contains 1 when left button is pressed. */
    uint8_t middle : 1; /**< Contains 1 when middle button is pressed. */
    uint8_t right : 1; /**< Contains 1 when right button is pressed. */

    mouse_buttons()
        : left(0)
        , middle(0)
        , right(0)
    {
    }
};


/**
 * @brief Describes mouse absolute position, scroll wheel delta and buttons status.
 */
struct mouse_status
{
    int16_t x; /**< Absolute horizontal mouse position. */
    int16_t y; /**< Absolute vertical mouse position. */
    int8_t wheel_delta; /**< Scroll wheel delta. */
    mouse_buttons buttons; /**< Mouse buttons status. */

    mouse_status()
        : x(0)
        , y(0)
        , wheel_delta(0)
    {
    }
};


///////////////////////////////////////////////////////////////////////////////////
// TimeOut


struct time_out
{
    time_out();

    // -1 means "infinite", never times out
    bool expired(int value_ms);

private:

    int64_t _start;
};


///////////////////////////////////////////////////////////////////////////////////
// Delegate

template<typename... Params>
struct delegate
{

    // empty constructor
    delegate()
        : _closure(nullptr)
        , _func(nullptr)
    {
    }

    // denied copy
    delegate(const delegate &c) = delete;

    // construct from lambda
    template<typename Func>
    delegate(Func f)
        : delegate()
    {
        *this = f;
    }

    ~delegate()
    {
        clean_up();
    }

    // assignment operator from Func
    template<typename Func>
    void operator=(Func f)
    {
        clean_up();
        _closure = [](void *func, const Params &...params) -> void { (*(Func *)func)(params...); };
        _func = heap_caps_malloc(sizeof(Func), MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
        move_items<uint32_t *>((uint32_t *)_func, (uint32_t *)&f, sizeof(Func) / sizeof(uint32_t));
    }

    // denied assignment from Delegate
    void operator=(const delegate &) = delete;

    void operator()(const Params &...params)
    {
        if (_func)
            _closure(_func, params...);
    }

private:

    void (*_closure)(void *func, const Params &...params);
    void *_func;

    void clean_up()
    {
        if (_func)
            heap_caps_free(_func);
    }
};


bool calc_parity(uint8_t v);


/** \ingroup Enumerations
 * @brief Represents each possible real or derived (SHIFT + real) key.
 */
enum virtual_key
{
    vk_none, /**< No character (marks the first virtual key) */

    vk_space, /**< Space */

    vk_0, /**< Number 0 */
    vk_1, /**< Number 1 */
    vk_2, /**< Number 2 */
    vk_3, /**< Number 3 */
    vk_4, /**< Number 4 */
    vk_5, /**< Number 5 */
    vk_6, /**< Number 6 */
    vk_7, /**< Number 7 */
    vk_8, /**< Number 8 */
    vk_9, /**< Number 9 */
    vk_kp_0, /**< Keypad number 0 */
    vk_kp_1, /**< Keypad number 1 */
    vk_kp_2, /**< Keypad number 2 */
    vk_kp_3, /**< Keypad number 3 */
    vk_kp_4, /**< Keypad number 4 */
    vk_kp_5, /**< Keypad number 5 */
    vk_kp_6, /**< Keypad number 6 */
    vk_kp_7, /**< Keypad number 7 */
    vk_kp_8, /**< Keypad number 8 */
    vk_kp_9, /**< Keypad number 9 */

    vk_a, /**< Lower case letter 'a' */
    vk_b, /**< Lower case letter 'b' */
    vk_c, /**< Lower case letter 'c' */
    vk_d, /**< Lower case letter 'd' */
    vk_e, /**< Lower case letter 'e' */
    vk_f, /**< Lower case letter 'f' */
    vk_g, /**< Lower case letter 'g' */
    vk_h, /**< Lower case letter 'h' */
    vk_i, /**< Lower case letter 'i' */
    vk_j, /**< Lower case letter 'j' */
    vk_k, /**< Lower case letter 'k' */
    vk_l, /**< Lower case letter 'l' */
    vk_m, /**< Lower case letter 'm' */
    vk_n, /**< Lower case letter 'n' */
    vk_o, /**< Lower case letter 'o' */
    vk_p, /**< Lower case letter 'p' */
    vk_q, /**< Lower case letter 'q' */
    vk_r, /**< Lower case letter 'r' */
    vk_s, /**< Lower case letter 's' */
    vk_t, /**< Lower case letter 't' */
    vk_u, /**< Lower case letter 'u' */
    vk_v, /**< Lower case letter 'v' */
    vk_w, /**< Lower case letter 'w' */
    vk_x, /**< Lower case letter 'x' */
    vk_y, /**< Lower case letter 'y' */
    vk_z, /**< Lower case letter 'z' */
    vk_A, /**< Upper case letter 'A' */
    vk_B, /**< Upper case letter 'B' */
    vk_C, /**< Upper case letter 'C' */
    vk_D, /**< Upper case letter 'D' */
    vk_E, /**< Upper case letter 'E' */
    vk_F, /**< Upper case letter 'F' */
    vk_G, /**< Upper case letter 'G' */
    vk_H, /**< Upper case letter 'H' */
    vk_I, /**< Upper case letter 'I' */
    vk_J, /**< Upper case letter 'J' */
    vk_K, /**< Upper case letter 'K' */
    vk_L, /**< Upper case letter 'L' */
    vk_M, /**< Upper case letter 'M' */
    vk_N, /**< Upper case letter 'N' */
    vk_O, /**< Upper case letter 'O' */
    vk_P, /**< Upper case letter 'P' */
    vk_Q, /**< Upper case letter 'Q' */
    vk_R, /**< Upper case letter 'R' */
    vk_S, /**< Upper case letter 'S' */
    vk_T, /**< Upper case letter 'T' */
    vk_U, /**< Upper case letter 'U' */
    vk_V, /**< Upper case letter 'V' */
    vk_W, /**< Upper case letter 'W' */
    vk_X, /**< Upper case letter 'X' */
    vk_Y, /**< Upper case letter 'Y' */
    vk_Z, /**< Upper case letter 'Z' */

    vk_graveaccent, /**< Grave accent: ` */
    vk_acuteaccent, /**< Acute accent: ´ */
    vk_quote, /**< Quote: ' */
    vk_quotedbl, /**< Double quote: " */
    vk_equals, /**< Equals: = */
    vk_minus, /**< Minus: - */
    vk_kp_minus, /**< Keypad minus: - */
    vk_plus, /**< Plus: + */
    vk_kp_plus, /**< Keypad plus: + */
    vk_kp_multiply, /**< Keypad multiply: * */
    vk_asterisk, /**< Asterisk: * */
    vk_backslash, /**< Backslash: \ */
    vk_kp_divide, /**< Keypad divide: / */
    vk_slash, /**< Slash: / */
    vk_kp_period, /**< Keypad period: . */
    vk_period, /**< Period: . */
    vk_colon, /**< Colon: : */
    vk_comma, /**< Comma: , */
    vk_semicolon, /**< Semicolon: ; */
    vk_ampersand, /**< Ampersand: & */
    vk_verticalbar, /**< Vertical bar: | */
    vk_hash, /**< Hash: # */
    vk_at, /**< At: @ */
    vk_caret, /**< Caret: ^ */
    vk_dollar, /**< Dollar: $ */
    vk_pound, /**< Pound: £ */
    vk_euro, /**< Euro: € */
    vk_percent, /**< Percent: % */
    vk_exclaim, /**< Exclamation mark: ! */
    vk_question, /**< Question mark: ? */
    vk_leftbrace, /**< Left brace: { */
    vk_rightbrace, /**< Right brace: } */
    vk_leftbracket, /**< Left bracket: [ */
    vk_rightbracket, /**< Right bracket: ] */
    vk_leftparen, /**< Left parenthesis: ( */
    vk_rightparen, /**< Right parenthesis: ) */
    vk_less, /**< Less: < */
    vk_greater, /**< Greater: > */
    vk_underscore, /**< Underscore: _ */
    vk_degree, /**< Degree: ° */
    vk_section, /**< Section: § */
    vk_tilde, /**< Tilde: ~ */
    vk_negation, /**< Negation: ¬ */

    vk_lshift, /**< Left SHIFT */
    vk_rshift, /**< Right SHIFT */
    vk_lalt, /**< Left ALT */
    vk_ralt, /**< Right ALT */
    vk_lctrl, /**< Left CTRL */
    vk_rctrl, /**< Right CTRL */
    vk_lgui, /**< Left GUI */
    vk_rgui, /**< Right GUI */

    vk_escape, /**< ESC */

    vk_printscreen, /**< PRINTSCREEN */
    vk_sysreq, /**< SYSREQ */

    vk_insert, /**< INS */
    vk_kp_insert, /**< Keypad INS */
    vk_delete, /**< DEL */
    vk_kp_delete, /**< Keypad DEL */
    vk_backspace, /**< Backspace */
    vk_home, /**< HOME */
    vk_kp_home, /**< Keypad HOME */
    vk_end, /**< END */
    vk_kp_end, /**< Keypad END */
    vk_pause, /**< PAUSE */
    vk_break, /**< CTRL + PAUSE */
    vk_scrolllock, /**< SCROLLLOCK */
    vk_numlock, /**< NUMLOCK */
    vk_capslock, /**< CAPSLOCK */
    vk_tab, /**< TAB */
    vk_return, /**< RETURN */
    vk_kp_enter, /**< Keypad ENTER */
    vk_application, /**< APPLICATION / MENU key */
    vk_pageup, /**< PAGEUP */
    vk_kp_pageup, /**< Keypad PAGEUP */
    vk_pagedown, /**< PAGEDOWN */
    vk_kp_pagedown, /**< Keypad PAGEDOWN */
    vk_up, /**< Cursor UP */
    vk_kp_up, /**< Keypad cursor UP  */
    vk_down, /**< Cursor DOWN */
    vk_kp_down, /**< Keypad cursor DOWN */
    vk_left, /**< Cursor LEFT */
    vk_kp_left, /**< Keypad cursor LEFT */
    vk_right, /**< Cursor RIGHT */
    vk_kp_right, /**< Keypad cursor RIGHT */
    vk_kp_center, /**< Keypad CENTER key */

    vk_f1, /**< F1 function key */
    vk_f2, /**< F2 function key */
    vk_f3, /**< F3 function key */
    vk_f4, /**< F4 function key */
    vk_f5, /**< F5 function key */
    vk_f6, /**< F6 function key */
    vk_f7, /**< F7 function key */
    vk_f8, /**< F8 function key */
    vk_f9, /**< F9 function key */
    vk_f10, /**< F10 function key */
    vk_f11, /**< F11 function key */
    vk_f12, /**< F12 function key */

    vk_grave_a, /**< Grave a: à */
    vk_grave_e, /**< Grave e: è */
    vk_grave_i, /**< Grave i: ì */
    vk_grave_o, /**< Grave o: ò */
    vk_grave_u, /**< Grave u: ù */
    vk_grave_y, /**< Grave y: ỳ */

    vk_acute_a, /**< Acute a: á */
    vk_acute_e, /**< Acute e: é */
    vk_acute_i, /**< Acute i: í */
    vk_acute_o, /**< Acute o: ó */
    vk_acute_u, /**< Acute u: ú */
    vk_acute_y, /**< Acute y: ý */

    vk_grave_A, /**< Grave A: À */
    vk_grave_E, /**< Grave E: È */
    vk_grave_I, /**< Grave I: Ì */
    vk_grave_O, /**< Grave O: Ò */
    vk_grave_U, /**< Grave U: Ù */
    vk_grave_Y, /**< Grave Y: Ỳ */

    vk_acute_A, /**< Acute A: Á */
    vk_acute_E, /**< Acute E: É */
    vk_acute_I, /**< Acute I: Í */
    vk_acute_O, /**< Acute O: Ó */
    vk_acute_U, /**< Acute U: Ú */
    vk_acute_Y, /**< Acute Y: Ý */

    vk_umlaut_a, /**< Diaeresis a: ä */
    vk_umlaut_e, /**< Diaeresis e: ë */
    vk_umlaut_i, /**< Diaeresis i: ï */
    vk_umlaut_o, /**< Diaeresis o: ö */
    vk_umlaut_u, /**< Diaeresis u: ü */
    vk_umlaut_y, /**< Diaeresis y: ÿ */

    vk_umlaut_A, /**< Diaeresis A: Ä */
    vk_umlaut_E, /**< Diaeresis E: Ë */
    vk_umlaut_I, /**< Diaeresis I: Ï */
    vk_umlaut_O, /**< Diaeresis O: Ö */
    vk_umlaut_U, /**< Diaeresis U: Ü */
    vk_umlaut_Y, /**< Diaeresis Y: Ÿ */

    vk_caret_a, /**< Caret a: â */
    vk_caret_e, /**< Caret e: ê */
    vk_caret_i, /**< Caret i: î */
    vk_caret_o, /**< Caret o: ô */
    vk_caret_u, /**< Caret u: û */
    vk_caret_y, /**< Caret y: ŷ */

    vk_caret_A, /**< Caret A: Â */
    vk_caret_E, /**< Caret E: Ê */
    vk_caret_I, /**< Caret I: Î */
    vk_caret_O, /**< Caret O: Ô */
    vk_caret_U, /**< Caret U: Û */
    vk_caret_Y, /**< Caret Y: Ŷ */

    vk_cedilla_c, /**< Cedilla c: ç */
    vk_cedilla_C, /**< Cedilla C: Ç */

    vk_tilde_a, /**< Lower case tilde a: ã */
    vk_tilde_o, /**< Lower case tilde o: õ */
    vk_tilde_n, /**< Lower case tilde n: ñ */

    vk_tilde_A, /**< Upper case tilde A: Ã */
    vk_tilde_O, /**< Upper case tilde O: Õ */
    vk_tilde_N, /**< Upper case tilde N: Ñ */

    vk_upper_a, /**< primera: a */
    vk_eszett, /**< Eszett: ß */
    vk_exclaim_inv, /**< Inverted exclamation mark: ! */
    vk_question_inv, /**< Inverted question mark : ? */
    vk_interpunct, /**< Interpunct : · */
    vk_diaeresis, /**< Diaeresis  : ¨ */
    vk_square, /**< Square     : ² */
    vk_currency, /**< Currency   : ¤ */
    vk_mu, /**< Mu         : µ */

    vk_aelig, /** Lower case aelig  : æ */
    vk_oslash, /** Lower case oslash : ø */
    vk_aring, /** Lower case aring  : å */

    vk_AELIG, /** Upper case aelig  : Æ */
    vk_OSLASH, /** Upper case oslash : Ø */
    vk_ARING, /** Upper case aring  : Å */

    // Japanese layout support
    vk_yen,
    vk_muhenkan,
    vk_henkan,
    vk_katakana_hiragana_romaji,
    vk_hankaku_zenkaku_kanji,
    vk_shift_0,

    vk_ascii, /**< Specifies an ASCII code - used when virtual key is embedded in VirtualKeyItem structure and VirtualKeyItem.ASCII is valid */
    vk_last, // marks the last virtual key
};


/**
 * @brief A struct which contains a virtual key, key state and associated scan code
 */
struct virtual_key_item
{
    virtual_key vk; /**< Virtual key */
    uint8_t down; /**< 0 = up, 1 = down */
    uint8_t scancode
        [8]; /**< Keyboard scancode. Ends with zero if length is <8, otherwise gets the entire length (like PAUSE, which is 8 bytes) */
    uint8_t ascii; /**< ASCII value (0 = if it isn't possible to translate from virtual key) */
    uint8_t ctrl : 1; /**< CTRL key state at the time of this virtual key event */
    uint8_t lalt : 1; /**< LEFT ALT key state at the time of this virtual key event */
    uint8_t ralt : 1; /**< RIGHT ALT key state at the time of this virtual key event */
    uint8_t shift : 1; /**< SHIFT key state at the time of this virtual key event */
    uint8_t gui : 1; /**< GUI key state at the time of this virtual key event */
    uint8_t capslock : 1; /**< CAPSLOCK key state at the time of this virtual key event */
    uint8_t numlock : 1; /**< NUMLOCK key state at the time of this virtual key event */
    uint8_t scrolllock : 1; /**< SCROLLLOCK key state at the time of this virtual key event */
};


///////////////////////////////////////////////////////////////////////////////////
// Virtual keys helpers

inline bool is_shift(virtual_key value)
{
    return value == vk_lshift || value == vk_rshift;
}


inline bool is_alt(virtual_key value)
{
    return value == vk_lalt || value == vk_ralt;
}


inline bool is_ctrl(virtual_key value)
{
    return value == vk_lctrl || value == vk_rctrl;
}


inline bool is_gui(virtual_key value)
{
    return value == vk_lgui || value == vk_rgui;
}


///////////////////////////////////////////////////////////////////////////////////
// ASCII control characters

#define ASCII_NUL 0x00 // Null
#define ASCII_SOH 0x01 // Start of Heading
#define ASCII_CTRLA 0x01 // CTRL-A
#define ASCII_STX 0x02 // Start of Text
#define ASCII_CTRLB 0x02 // CTRL-B
#define ASCII_ETX 0x03 // End Of Text
#define ASCII_CTRLC 0x03 // CTRL-C
#define ASCII_EOT 0x04 // End Of Transmission
#define ASCII_CTRLD 0x04 // CTRL-D
#define ASCII_ENQ 0x05 // Enquiry
#define ASCII_CTRLE 0x05 // CTRL-E
#define ASCII_ACK 0x06 // Acknowledge
#define ASCII_CTRLF 0x06 // CTRL-F
#define ASCII_BEL 0x07 // Bell
#define ASCII_CTRLG 0x07 // CTRL-G
#define ASCII_BS 0x08 // Backspace
#define ASCII_CTRLH 0x08 // CTRL-H
#define ASCII_HT 0x09 // Horizontal Tab
#define ASCII_TAB 0x09 // Horizontal Tab
#define ASCII_CTRLI 0x09 // CTRL-I
#define ASCII_LF 0x0A // Line Feed
#define ASCII_CTRLJ 0x0A // CTRL-J
#define ASCII_VT 0x0B // Vertical Tab
#define ASCII_CTRLK 0x0B // CTRL-K
#define ASCII_FF 0x0C // Form Feed
#define ASCII_CTRLL 0x0C // CTRL-L
#define ASCII_CR 0x0D // Carriage Return
#define ASCII_CTRLM 0x0D // CTRL-M
#define ASCII_SO 0x0E // Shift Out
#define ASCII_CTRLN 0x0E // CTRL-N
#define ASCII_SI 0x0F // Shift In
#define ASCII_CTRLO 0x0F // CTRL-O
#define ASCII_DLE 0x10 // Data Link Escape
#define ASCII_CTRLP 0x10 // CTRL-P
#define ASCII_DC1 0x11 // Device Control 1
#define ASCII_CTRLQ 0x11 // CTRL-Q
#define ASCII_XON 0x11 // Transmission On
#define ASCII_DC2 0x12 // Device Control 2
#define ASCII_CTRLR 0x12 // CTRL-R
#define ASCII_DC3 0x13 // Device Control 3
#define ASCII_XOFF 0x13 // Transmission Off
#define ASCII_CTRLS 0x13 // CTRL-S
#define ASCII_DC4 0x14 // Device Control 4
#define ASCII_CTRLT 0x14 // CTRL-T
#define ASCII_NAK 0x15 // Negative Acknowledge
#define ASCII_CTRLU 0x15 // CTRL-U
#define ASCII_SYN 0x16 // Synchronous Idle
#define ASCII_CTRLV 0x16 // CTRL-V
#define ASCII_ETB 0x17 // End-of-Transmission-Block
#define ASCII_CTRLW 0x17 // CTRL-W
#define ASCII_CAN 0x18 // Cancel
#define ASCII_CTRLX 0x18 // CTRL-X
#define ASCII_EM 0x19 // End of Medium
#define ASCII_CTRLY 0x19 // CTRL-Y
#define ASCII_SUB 0x1A // Substitute
#define ASCII_CTRLZ 0x1A // CTRL-Z
#define ASCII_ESC 0x1B // Escape
#define ASCII_FS 0x1C // File Separator
#define ASCII_GS 0x1D // Group Separator
#define ASCII_RS 0x1E // Record Separator
#define ASCII_US 0x1F // Unit Separator
#define ASCII_SPC 0x20 // Space
#define ASCII_DEL 0x7F // Delete


} // end of namespace
