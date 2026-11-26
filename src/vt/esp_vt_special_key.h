#pragma once


/**
 * @file
 * @brief Особые клавиши esp::vt::special_key.
 */


#include <cstdint>


namespace esp::vt
{

/**
 * @brief Особая клавиша — та, что приходит последовательностью VT220, а не знаком.
 *
 * Номера идут с 0x110000, сразу за кодами Unicode, поэтому в
 * esp::vt::key_event особая клавиша не спутается со знаком. Те же номера
 * получат слова Форта @c ekey и @c k-left … @c k-f12 (раздел 7.5
 * документа об устройстве).
 */
enum class special_key : std::uint32_t
{
    /** @brief Стрелка вверх: <tt>ESC [A</tt>. */
    up = 0x110000,

    /** @brief Стрелка вниз: <tt>ESC [B</tt>. */
    down,

    /** @brief Стрелка вправо: <tt>ESC [C</tt>. */
    right,

    /** @brief Стрелка влево: <tt>ESC [D</tt>. */
    left,

    /** @brief Home: <tt>ESC [H</tt>; другие терминалы шлют <tt>ESC [1~</tt>, <tt>ESC [7~</tt> или <tt>ESC OH</tt>. */
    home,

    /** @brief End: <tt>ESC [F</tt>; другие терминалы шлют <tt>ESC [4~</tt>, <tt>ESC [8~</tt> или <tt>ESC OF</tt>. */
    end,

    /** @brief Insert: <tt>ESC [2~</tt>. */
    insert,

    /** @brief Delete: <tt>ESC [3~</tt>; имя короче, потому что @c delete — ключевое слово C++. */
    del,

    /** @brief PgUp: <tt>ESC [5~</tt>. */
    page_up,

    /** @brief PgDn: <tt>ESC [6~</tt>. */
    page_down,

    /** @brief F1: <tt>ESC OP</tt>; другие терминалы шлют <tt>ESC [11~</tt>. */
    f1,

    /** @brief F2: <tt>ESC OQ</tt> или <tt>ESC [12~</tt>. */
    f2,

    /** @brief F3: <tt>ESC OR</tt> или <tt>ESC [13~</tt>. */
    f3,

    /** @brief F4: <tt>ESC OS</tt> или <tt>ESC [14~</tt>. */
    f4,

    /** @brief F5: <tt>ESC [15~</tt>. */
    f5,

    /** @brief F6: <tt>ESC [17~</tt>. */
    f6,

    /** @brief F7: <tt>ESC [18~</tt>. */
    f7,

    /** @brief F8: <tt>ESC [19~</tt>. */
    f8,

    /** @brief F9: <tt>ESC [20~</tt>. */
    f9,

    /** @brief F10: <tt>ESC [21~</tt>. */
    f10,

    /** @brief F11: <tt>ESC [23~</tt>. */
    f11,

    /** @brief F12: <tt>ESC [24~</tt>. */
    f12,
};

}
