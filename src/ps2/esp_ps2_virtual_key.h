// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Виртуальные клавиши esp::ps2::virtual_key и помощники для них.
 */


namespace esp::ps2
{

/**
 * @brief Виртуальная клавиша: клавиша или знак, полученный из скан-кодов с учётом раскладки.
 *
 * Клавиатура esp::ps2::keyboard переводит скан-коды в виртуальные клавиши
 * по раскладке @c esp::ps2::keyboard_layout: одной физической клавише
 * соответствуют разные виртуальные клавиши в зависимости от SHIFT, ALT и
 * других модификаторов (например, @c vk_1 и @c vk_exclaim). Буквы, которые
 * различаются только регистром, различаются и в именах: @c vk_a и @c vk_A.
 * Префикс @c vk_ нужен и для того, чтобы имена вроде @c vk_delete и
 * @c vk_0 оставались допустимыми.
 *
 * Значения идут подряд от @c vk_none до @c vk_last; на этом держатся
 * таблица нажатых клавиш клавиатуры и перевод регистра букв.
 */
enum class virtual_key : unsigned
{
    /** @brief Нет клавиши; первое значение перечисления. */
    vk_none,

    /** @brief Пробел. */
    vk_space,

    /** @brief Цифра 0 основной клавиатуры. */
    vk_0,

    /** @brief Цифра 1 основной клавиатуры. */
    vk_1,

    /** @brief Цифра 2 основной клавиатуры. */
    vk_2,

    /** @brief Цифра 3 основной клавиатуры. */
    vk_3,

    /** @brief Цифра 4 основной клавиатуры. */
    vk_4,

    /** @brief Цифра 5 основной клавиатуры. */
    vk_5,

    /** @brief Цифра 6 основной клавиатуры. */
    vk_6,

    /** @brief Цифра 7 основной клавиатуры. */
    vk_7,

    /** @brief Цифра 8 основной клавиатуры. */
    vk_8,

    /** @brief Цифра 9 основной клавиатуры. */
    vk_9,

    /** @brief Цифра 0 цифровой клавиатуры. */
    vk_kp_0,

    /** @brief Цифра 1 цифровой клавиатуры. */
    vk_kp_1,

    /** @brief Цифра 2 цифровой клавиатуры. */
    vk_kp_2,

    /** @brief Цифра 3 цифровой клавиатуры. */
    vk_kp_3,

    /** @brief Цифра 4 цифровой клавиатуры. */
    vk_kp_4,

    /** @brief Цифра 5 цифровой клавиатуры. */
    vk_kp_5,

    /** @brief Цифра 6 цифровой клавиатуры. */
    vk_kp_6,

    /** @brief Цифра 7 цифровой клавиатуры. */
    vk_kp_7,

    /** @brief Цифра 8 цифровой клавиатуры. */
    vk_kp_8,

    /** @brief Цифра 9 цифровой клавиатуры. */
    vk_kp_9,

    /** @brief Строчная буква a. */
    vk_a,

    /** @brief Строчная буква b. */
    vk_b,

    /** @brief Строчная буква c. */
    vk_c,

    /** @brief Строчная буква d. */
    vk_d,

    /** @brief Строчная буква e. */
    vk_e,

    /** @brief Строчная буква f. */
    vk_f,

    /** @brief Строчная буква g. */
    vk_g,

    /** @brief Строчная буква h. */
    vk_h,

    /** @brief Строчная буква i. */
    vk_i,

    /** @brief Строчная буква j. */
    vk_j,

    /** @brief Строчная буква k. */
    vk_k,

    /** @brief Строчная буква l. */
    vk_l,

    /** @brief Строчная буква m. */
    vk_m,

    /** @brief Строчная буква n. */
    vk_n,

    /** @brief Строчная буква o. */
    vk_o,

    /** @brief Строчная буква p. */
    vk_p,

    /** @brief Строчная буква q. */
    vk_q,

    /** @brief Строчная буква r. */
    vk_r,

    /** @brief Строчная буква s. */
    vk_s,

    /** @brief Строчная буква t. */
    vk_t,

    /** @brief Строчная буква u. */
    vk_u,

    /** @brief Строчная буква v. */
    vk_v,

    /** @brief Строчная буква w. */
    vk_w,

    /** @brief Строчная буква x. */
    vk_x,

    /** @brief Строчная буква y. */
    vk_y,

    /** @brief Строчная буква z. */
    vk_z,

    /** @brief Заглавная буква A. */
    vk_A,

    /** @brief Заглавная буква B. */
    vk_B,

    /** @brief Заглавная буква C. */
    vk_C,

    /** @brief Заглавная буква D. */
    vk_D,

    /** @brief Заглавная буква E. */
    vk_E,

    /** @brief Заглавная буква F. */
    vk_F,

    /** @brief Заглавная буква G. */
    vk_G,

    /** @brief Заглавная буква H. */
    vk_H,

    /** @brief Заглавная буква I. */
    vk_I,

    /** @brief Заглавная буква J. */
    vk_J,

    /** @brief Заглавная буква K. */
    vk_K,

    /** @brief Заглавная буква L. */
    vk_L,

    /** @brief Заглавная буква M. */
    vk_M,

    /** @brief Заглавная буква N. */
    vk_N,

    /** @brief Заглавная буква O. */
    vk_O,

    /** @brief Заглавная буква P. */
    vk_P,

    /** @brief Заглавная буква Q. */
    vk_Q,

    /** @brief Заглавная буква R. */
    vk_R,

    /** @brief Заглавная буква S. */
    vk_S,

    /** @brief Заглавная буква T. */
    vk_T,

    /** @brief Заглавная буква U. */
    vk_U,

    /** @brief Заглавная буква V. */
    vk_V,

    /** @brief Заглавная буква W. */
    vk_W,

    /** @brief Заглавная буква X. */
    vk_X,

    /** @brief Заглавная буква Y. */
    vk_Y,

    /** @brief Заглавная буква Z. */
    vk_Z,

    /** @brief Обратный апостроф (гравис), код 0x60. */
    vk_graveaccent,

    /** @brief Акут «´». */
    vk_acuteaccent,

    /** @brief Апостроф «'». */
    vk_quote,

    /** @brief Двойная кавычка «\"». */
    vk_quotedbl,

    /** @brief Равно «=». */
    vk_equals,

    /** @brief Минус «-». */
    vk_minus,

    /** @brief Минус цифровой клавиатуры «-». */
    vk_kp_minus,

    /** @brief Плюс «+». */
    vk_plus,

    /** @brief Плюс цифровой клавиатуры «+». */
    vk_kp_plus,

    /** @brief Умножение цифровой клавиатуры «*». */
    vk_kp_multiply,

    /** @brief Звёздочка «*». */
    vk_asterisk,

    /** @brief Обратная косая черта «\\». */
    vk_backslash,

    /** @brief Деление цифровой клавиатуры «/». */
    vk_kp_divide,

    /** @brief Косая черта «/». */
    vk_slash,

    /** @brief Точка цифровой клавиатуры «.». */
    vk_kp_period,

    /** @brief Точка «.». */
    vk_period,

    /** @brief Двоеточие «:». */
    vk_colon,

    /** @brief Запятая «,». */
    vk_comma,

    /** @brief Точка с запятой «;». */
    vk_semicolon,

    /** @brief Амперсанд «\&». */
    vk_ampersand,

    /** @brief Вертикальная черта «|». */
    vk_verticalbar,

    /** @brief Решётка «\#». */
    vk_hash,

    /** @brief Коммерческое at «\@». */
    vk_at,

    /** @brief Циркумфлекс «^». */
    vk_caret,

    /** @brief Доллар «\$». */
    vk_dollar,

    /** @brief Фунт «£». */
    vk_pound,

    /** @brief Евро «€». */
    vk_euro,

    /** @brief Процент «\%». */
    vk_percent,

    /** @brief Восклицательный знак «!». */
    vk_exclaim,

    /** @brief Вопросительный знак «?». */
    vk_question,

    /** @brief Левая фигурная скобка «{». */
    vk_leftbrace,

    /** @brief Правая фигурная скобка «}». */
    vk_rightbrace,

    /** @brief Левая квадратная скобка «[». */
    vk_leftbracket,

    /** @brief Правая квадратная скобка «]». */
    vk_rightbracket,

    /** @brief Левая круглая скобка «(». */
    vk_leftparen,

    /** @brief Правая круглая скобка «)». */
    vk_rightparen,

    /** @brief Меньше «\<». */
    vk_less,

    /** @brief Больше «\>». */
    vk_greater,

    /** @brief Подчёркивание «_». */
    vk_underscore,

    /** @brief Градус «°». */
    vk_degree,

    /** @brief Параграф «§». */
    vk_section,

    /** @brief Тильда «~». */
    vk_tilde,

    /** @brief Знак отрицания «¬». */
    vk_negation,

    /** @brief Левая клавиша SHIFT. */
    vk_lshift,

    /** @brief Правая клавиша SHIFT. */
    vk_rshift,

    /** @brief Левая клавиша ALT. */
    vk_lalt,

    /** @brief Правая клавиша ALT (ALTGR). */
    vk_ralt,

    /** @brief Левая клавиша CTRL. */
    vk_lctrl,

    /** @brief Правая клавиша CTRL. */
    vk_rctrl,

    /** @brief Левая клавиша GUI (Windows). */
    vk_lgui,

    /** @brief Правая клавиша GUI (Windows). */
    vk_rgui,

    /** @brief ESC. */
    vk_escape,

    /** @brief PRINT SCREEN. */
    vk_printscreen,

    /** @brief SYSRQ (ALT + PRINT SCREEN). */
    vk_sysreq,

    /** @brief INSERT. */
    vk_insert,

    /** @brief INSERT цифровой клавиатуры. */
    vk_kp_insert,

    /** @brief DELETE. */
    vk_delete,

    /** @brief DELETE цифровой клавиатуры. */
    vk_kp_delete,

    /** @brief BACKSPACE. */
    vk_backspace,

    /** @brief HOME. */
    vk_home,

    /** @brief HOME цифровой клавиатуры. */
    vk_kp_home,

    /** @brief END. */
    vk_end,

    /** @brief END цифровой клавиатуры. */
    vk_kp_end,

    /** @brief PAUSE. */
    vk_pause,

    /** @brief BREAK (CTRL + PAUSE). */
    vk_break,

    /** @brief SCROLL LOCK. */
    vk_scrolllock,

    /** @brief NUM LOCK. */
    vk_numlock,

    /** @brief CAPS LOCK. */
    vk_capslock,

    /** @brief TAB. */
    vk_tab,

    /** @brief ENTER основной клавиатуры. */
    vk_return,

    /** @brief ENTER цифровой клавиатуры. */
    vk_kp_enter,

    /** @brief Клавиша контекстного меню. */
    vk_application,

    /** @brief PAGE UP. */
    vk_pageup,

    /** @brief PAGE UP цифровой клавиатуры. */
    vk_kp_pageup,

    /** @brief PAGE DOWN. */
    vk_pagedown,

    /** @brief PAGE DOWN цифровой клавиатуры. */
    vk_kp_pagedown,

    /** @brief Стрелка вверх. */
    vk_up,

    /** @brief Стрелка вверх цифровой клавиатуры. */
    vk_kp_up,

    /** @brief Стрелка вниз. */
    vk_down,

    /** @brief Стрелка вниз цифровой клавиатуры. */
    vk_kp_down,

    /** @brief Стрелка влево. */
    vk_left,

    /** @brief Стрелка влево цифровой клавиатуры. */
    vk_kp_left,

    /** @brief Стрелка вправо. */
    vk_right,

    /** @brief Стрелка вправо цифровой клавиатуры. */
    vk_kp_right,

    /** @brief Средняя клавиша цифровой клавиатуры (5 при выключенном NUM LOCK). */
    vk_kp_center,

    /** @brief Функциональная клавиша F1. */
    vk_f1,

    /** @brief Функциональная клавиша F2. */
    vk_f2,

    /** @brief Функциональная клавиша F3. */
    vk_f3,

    /** @brief Функциональная клавиша F4. */
    vk_f4,

    /** @brief Функциональная клавиша F5. */
    vk_f5,

    /** @brief Функциональная клавиша F6. */
    vk_f6,

    /** @brief Функциональная клавиша F7. */
    vk_f7,

    /** @brief Функциональная клавиша F8. */
    vk_f8,

    /** @brief Функциональная клавиша F9. */
    vk_f9,

    /** @brief Функциональная клавиша F10. */
    vk_f10,

    /** @brief Функциональная клавиша F11. */
    vk_f11,

    /** @brief Функциональная клавиша F12. */
    vk_f12,

    /** @brief Строчная a с грависом: à. */
    vk_grave_a,

    /** @brief Строчная e с грависом: è. */
    vk_grave_e,

    /** @brief Строчная i с грависом: ì. */
    vk_grave_i,

    /** @brief Строчная o с грависом: ò. */
    vk_grave_o,

    /** @brief Строчная u с грависом: ù. */
    vk_grave_u,

    /** @brief Строчная y с грависом: ỳ. */
    vk_grave_y,

    /** @brief Строчная a с акутом: á. */
    vk_acute_a,

    /** @brief Строчная e с акутом: é. */
    vk_acute_e,

    /** @brief Строчная i с акутом: í. */
    vk_acute_i,

    /** @brief Строчная o с акутом: ó. */
    vk_acute_o,

    /** @brief Строчная u с акутом: ú. */
    vk_acute_u,

    /** @brief Строчная y с акутом: ý. */
    vk_acute_y,

    /** @brief Заглавная A с грависом: À. */
    vk_grave_A,

    /** @brief Заглавная E с грависом: È. */
    vk_grave_E,

    /** @brief Заглавная I с грависом: Ì. */
    vk_grave_I,

    /** @brief Заглавная O с грависом: Ò. */
    vk_grave_O,

    /** @brief Заглавная U с грависом: Ù. */
    vk_grave_U,

    /** @brief Заглавная Y с грависом: Ỳ. */
    vk_grave_Y,

    /** @brief Заглавная A с акутом: Á. */
    vk_acute_A,

    /** @brief Заглавная E с акутом: É. */
    vk_acute_E,

    /** @brief Заглавная I с акутом: Í. */
    vk_acute_I,

    /** @brief Заглавная O с акутом: Ó. */
    vk_acute_O,

    /** @brief Заглавная U с акутом: Ú. */
    vk_acute_U,

    /** @brief Заглавная Y с акутом: Ý. */
    vk_acute_Y,

    /** @brief Строчная a с умлаутом: ä. */
    vk_umlaut_a,

    /** @brief Строчная e с умлаутом: ë. */
    vk_umlaut_e,

    /** @brief Строчная i с умлаутом: ï. */
    vk_umlaut_i,

    /** @brief Строчная o с умлаутом: ö. */
    vk_umlaut_o,

    /** @brief Строчная u с умлаутом: ü. */
    vk_umlaut_u,

    /** @brief Строчная y с умлаутом: ÿ. */
    vk_umlaut_y,

    /** @brief Заглавная A с умлаутом: Ä. */
    vk_umlaut_A,

    /** @brief Заглавная E с умлаутом: Ë. */
    vk_umlaut_E,

    /** @brief Заглавная I с умлаутом: Ï. */
    vk_umlaut_I,

    /** @brief Заглавная O с умлаутом: Ö. */
    vk_umlaut_O,

    /** @brief Заглавная U с умлаутом: Ü. */
    vk_umlaut_U,

    /** @brief Заглавная Y с умлаутом: Ÿ. */
    vk_umlaut_Y,

    /** @brief Строчная a с циркумфлексом: â. */
    vk_caret_a,

    /** @brief Строчная e с циркумфлексом: ê. */
    vk_caret_e,

    /** @brief Строчная i с циркумфлексом: î. */
    vk_caret_i,

    /** @brief Строчная o с циркумфлексом: ô. */
    vk_caret_o,

    /** @brief Строчная u с циркумфлексом: û. */
    vk_caret_u,

    /** @brief Строчная y с циркумфлексом: ŷ. */
    vk_caret_y,

    /** @brief Заглавная A с циркумфлексом: Â. */
    vk_caret_A,

    /** @brief Заглавная E с циркумфлексом: Ê. */
    vk_caret_E,

    /** @brief Заглавная I с циркумфлексом: Î. */
    vk_caret_I,

    /** @brief Заглавная O с циркумфлексом: Ô. */
    vk_caret_O,

    /** @brief Заглавная U с циркумфлексом: Û. */
    vk_caret_U,

    /** @brief Заглавная Y с циркумфлексом: Ŷ. */
    vk_caret_Y,

    /** @brief Строчная c с седилью: ç. */
    vk_cedilla_c,

    /** @brief Заглавная C с седилью: Ç. */
    vk_cedilla_C,

    /** @brief Строчная a с тильдой: ã. */
    vk_tilde_a,

    /** @brief Строчная o с тильдой: õ. */
    vk_tilde_o,

    /** @brief Строчная n с тильдой: ñ. */
    vk_tilde_n,

    /** @brief Заглавная A с тильдой: Ã. */
    vk_tilde_A,

    /** @brief Заглавная O с тильдой: Õ. */
    vk_tilde_O,

    /** @brief Заглавная N с тильдой: Ñ. */
    vk_tilde_N,

    /** @brief Порядковый индикатор: ª. */
    vk_upper_a,

    /** @brief Эсцет: ß. */
    vk_eszett,

    /** @brief Перевёрнутый восклицательный знак «¡». */
    vk_exclaim_inv,

    /** @brief Перевёрнутый вопросительный знак «¿». */
    vk_question_inv,

    /** @brief Интерпункт «·». */
    vk_interpunct,

    /** @brief Умлаут «¨». */
    vk_diaeresis,

    /** @brief Квадрат: ². */
    vk_square,

    /** @brief Знак валюты «¤». */
    vk_currency,

    /** @brief Микро: µ. */
    vk_mu,

    /** @brief Строчная лигатура æ. */
    vk_aelig,

    /** @brief Строчная перечёркнутая o: ø. */
    vk_oslash,

    /** @brief Строчная a с кружком: å. */
    vk_aring,

    /** @brief Заглавная лигатура Æ. */
    vk_AELIG,

    /** @brief Заглавная перечёркнутая O: Ø. */
    vk_OSLASH,

    /** @brief Заглавная A с кружком: Å. */
    vk_ARING,

    /** @brief Иена: ¥ (японская раскладка). */
    vk_yen,

    /** @brief MUHENKAN — «без преобразования» (японская раскладка). */
    vk_muhenkan,

    /** @brief HENKAN — «преобразование» (японская раскладка). */
    vk_henkan,

    /** @brief Переключение катакана / хирагана / ромадзи (японская раскладка). */
    vk_katakana_hiragana_romaji,

    /** @brief Переключение полуширинных и полноширинных знаков, кандзи (японская раскладка). */
    vk_hankaku_zenkaku_kanji,

    /** @brief SHIFT + 0 (японская раскладка). */
    vk_shift_0,

    /** @brief Код ASCII без виртуальной клавиши: действует поле esp::ps2::virtual_key_item::ascii. */
    vk_ascii,

    /** @brief Число виртуальных клавиш; последнее значение перечисления. */
    vk_last,
};

inline bool is_shift(virtual_key value);
inline bool is_alt(virtual_key value);
inline bool is_ctrl(virtual_key value);
inline bool is_gui(virtual_key value);

}


// Реализация

/**
 * @brief Сообщает, SHIFT ли это.
 * @param value Виртуальная клавиша.
 * @return @c true для левой и правой клавиш SHIFT.
 */
bool esp::ps2::is_shift(virtual_key value)
{
    return value == virtual_key::vk_lshift || value == virtual_key::vk_rshift;
}

/**
 * @brief Сообщает, ALT ли это.
 * @param value Виртуальная клавиша.
 * @return @c true для левой и правой клавиш ALT.
 */
bool esp::ps2::is_alt(virtual_key value)
{
    return value == virtual_key::vk_lalt || value == virtual_key::vk_ralt;
}

/**
 * @brief Сообщает, CTRL ли это.
 * @param value Виртуальная клавиша.
 * @return @c true для левой и правой клавиш CTRL.
 */
bool esp::ps2::is_ctrl(virtual_key value)
{
    return value == virtual_key::vk_lctrl || value == virtual_key::vk_rctrl;
}

/**
 * @brief Сообщает, GUI (Windows) ли это.
 * @param value Виртуальная клавиша.
 * @return @c true для левой и правой клавиш GUI.
 */
bool esp::ps2::is_gui(virtual_key value)
{
    return value == virtual_key::vk_lgui || value == virtual_key::vk_rgui;
}
