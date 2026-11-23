// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Управляющие коды ASCII.
 *
 * Коды, которые клавиатура выдаёт для управляющих клавиш и сочетаний с
 * CTRL (esp::ps2::virtual_key_to_ascii()). У одного кода бывает несколько
 * имён: по назначению (@c ASCII_CR) и по сочетанию клавиш (@c ASCII_CTRLM).
 */


#include <cstdint>


namespace esp::ps2
{

/** @brief Пустой знак (NUL). */
constexpr std::uint8_t ASCII_NUL = 0x00;

/** @brief Начало заголовка (SOH). */
constexpr std::uint8_t ASCII_SOH = 0x01;

/** @brief CTRL + A. */
constexpr std::uint8_t ASCII_CTRLA = 0x01;

/** @brief Начало текста (STX). */
constexpr std::uint8_t ASCII_STX = 0x02;

/** @brief CTRL + B. */
constexpr std::uint8_t ASCII_CTRLB = 0x02;

/** @brief Конец текста (ETX). */
constexpr std::uint8_t ASCII_ETX = 0x03;

/** @brief CTRL + C. */
constexpr std::uint8_t ASCII_CTRLC = 0x03;

/** @brief Конец передачи (EOT). */
constexpr std::uint8_t ASCII_EOT = 0x04;

/** @brief CTRL + D. */
constexpr std::uint8_t ASCII_CTRLD = 0x04;

/** @brief Запрос (ENQ). */
constexpr std::uint8_t ASCII_ENQ = 0x05;

/** @brief CTRL + E. */
constexpr std::uint8_t ASCII_CTRLE = 0x05;

/** @brief Подтверждение (ACK). */
constexpr std::uint8_t ASCII_ACK = 0x06;

/** @brief CTRL + F. */
constexpr std::uint8_t ASCII_CTRLF = 0x06;

/** @brief Звонок (BEL). */
constexpr std::uint8_t ASCII_BEL = 0x07;

/** @brief CTRL + G. */
constexpr std::uint8_t ASCII_CTRLG = 0x07;

/** @brief Шаг назад (BS). */
constexpr std::uint8_t ASCII_BS = 0x08;

/** @brief CTRL + H. */
constexpr std::uint8_t ASCII_CTRLH = 0x08;

/** @brief Горизонтальная табуляция (HT). */
constexpr std::uint8_t ASCII_HT = 0x09;

/** @brief Горизонтальная табуляция (HT), клавиша TAB. */
constexpr std::uint8_t ASCII_TAB = 0x09;

/** @brief CTRL + I. */
constexpr std::uint8_t ASCII_CTRLI = 0x09;

/** @brief Перевод строки (LF). */
constexpr std::uint8_t ASCII_LF = 0x0A;

/** @brief CTRL + J. */
constexpr std::uint8_t ASCII_CTRLJ = 0x0A;

/** @brief Вертикальная табуляция (VT). */
constexpr std::uint8_t ASCII_VT = 0x0B;

/** @brief CTRL + K. */
constexpr std::uint8_t ASCII_CTRLK = 0x0B;

/** @brief Перевод страницы (FF). */
constexpr std::uint8_t ASCII_FF = 0x0C;

/** @brief CTRL + L. */
constexpr std::uint8_t ASCII_CTRLL = 0x0C;

/** @brief Возврат каретки (CR). */
constexpr std::uint8_t ASCII_CR = 0x0D;

/** @brief CTRL + M. */
constexpr std::uint8_t ASCII_CTRLM = 0x0D;

/** @brief Переход на другой набор (SO). */
constexpr std::uint8_t ASCII_SO = 0x0E;

/** @brief CTRL + N. */
constexpr std::uint8_t ASCII_CTRLN = 0x0E;

/** @brief Возврат к основному набору (SI). */
constexpr std::uint8_t ASCII_SI = 0x0F;

/** @brief CTRL + O. */
constexpr std::uint8_t ASCII_CTRLO = 0x0F;

/** @brief Переключение канала (DLE). */
constexpr std::uint8_t ASCII_DLE = 0x10;

/** @brief CTRL + P. */
constexpr std::uint8_t ASCII_CTRLP = 0x10;

/** @brief Управление устройством 1 (DC1). */
constexpr std::uint8_t ASCII_DC1 = 0x11;

/** @brief CTRL + Q. */
constexpr std::uint8_t ASCII_CTRLQ = 0x11;

/** @brief Возобновить передачу (XON, DC1). */
constexpr std::uint8_t ASCII_XON = 0x11;

/** @brief Управление устройством 2 (DC2). */
constexpr std::uint8_t ASCII_DC2 = 0x12;

/** @brief CTRL + R. */
constexpr std::uint8_t ASCII_CTRLR = 0x12;

/** @brief Управление устройством 3 (DC3). */
constexpr std::uint8_t ASCII_DC3 = 0x13;

/** @brief Приостановить передачу (XOFF, DC3). */
constexpr std::uint8_t ASCII_XOFF = 0x13;

/** @brief CTRL + S. */
constexpr std::uint8_t ASCII_CTRLS = 0x13;

/** @brief Управление устройством 4 (DC4). */
constexpr std::uint8_t ASCII_DC4 = 0x14;

/** @brief CTRL + T. */
constexpr std::uint8_t ASCII_CTRLT = 0x14;

/** @brief Отрицательное подтверждение (NAK). */
constexpr std::uint8_t ASCII_NAK = 0x15;

/** @brief CTRL + U. */
constexpr std::uint8_t ASCII_CTRLU = 0x15;

/** @brief Синхронизация (SYN). */
constexpr std::uint8_t ASCII_SYN = 0x16;

/** @brief CTRL + V. */
constexpr std::uint8_t ASCII_CTRLV = 0x16;

/** @brief Конец блока передачи (ETB). */
constexpr std::uint8_t ASCII_ETB = 0x17;

/** @brief CTRL + W. */
constexpr std::uint8_t ASCII_CTRLW = 0x17;

/** @brief Отмена (CAN). */
constexpr std::uint8_t ASCII_CAN = 0x18;

/** @brief CTRL + X. */
constexpr std::uint8_t ASCII_CTRLX = 0x18;

/** @brief Конец носителя (EM). */
constexpr std::uint8_t ASCII_EM = 0x19;

/** @brief CTRL + Y. */
constexpr std::uint8_t ASCII_CTRLY = 0x19;

/** @brief Замена (SUB). */
constexpr std::uint8_t ASCII_SUB = 0x1A;

/** @brief CTRL + Z. */
constexpr std::uint8_t ASCII_CTRLZ = 0x1A;

/** @brief Начало управляющей последовательности (ESC). */
constexpr std::uint8_t ASCII_ESC = 0x1B;

/** @brief Разделитель файлов (FS). */
constexpr std::uint8_t ASCII_FS = 0x1C;

/** @brief Разделитель групп (GS). */
constexpr std::uint8_t ASCII_GS = 0x1D;

/** @brief Разделитель записей (RS). */
constexpr std::uint8_t ASCII_RS = 0x1E;

/** @brief Разделитель элементов (US). */
constexpr std::uint8_t ASCII_US = 0x1F;

/** @brief Пробел. */
constexpr std::uint8_t ASCII_SPC = 0x20;

/** @brief Удаление (DEL). */
constexpr std::uint8_t ASCII_DEL = 0x7F;

}
