#pragma once


/**
 * @file
 * @brief Имена управляющих байтов C0, с которыми работает терминал esp::vt.
 */


#include <cstdint>


namespace esp::vt
{

/** @brief NUL — пустой байт; экран его пропускает. */
constexpr std::uint8_t NUL = 0x00;

/** @brief BEL — звонок; в строке OSC — ещё и её конец. */
constexpr std::uint8_t BEL = 0x07;

/** @brief BS — курсор на знак влево. */
constexpr std::uint8_t BS = 0x08;

/** @brief HT — курсор к следующему столбцу, кратному @c esp::vt::TAB_WIDTH. */
constexpr std::uint8_t HT = 0x09;

/** @brief LF — курсор на строку вниз, у нижнего края области прокрутки — прокрутка. */
constexpr std::uint8_t LF = 0x0A;

/** @brief VT — то же, что LF. */
constexpr std::uint8_t VT = 0x0B;

/** @brief FF — то же, что LF. */
constexpr std::uint8_t FF = 0x0C;

/** @brief CR — курсор в начало строки. */
constexpr std::uint8_t CR = 0x0D;

/** @brief CAN — прерывает начатую последовательность. */
constexpr std::uint8_t CAN = 0x18;

/** @brief SUB — прерывает начатую последовательность, как CAN. */
constexpr std::uint8_t SUB = 0x1A;

/** @brief ESC — начало последовательности; внутри строки — первый байт её конца @c ESC @c \\. */
constexpr std::uint8_t ESC = 0x1B;

/** @brief DEL — в потоке вывода ничего не значит и пропускается. */
constexpr std::uint8_t DEL = 0x7F;

}
