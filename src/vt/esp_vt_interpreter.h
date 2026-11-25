#pragma once


/**
 * @file
 * @brief Исполнитель разобранного потока вывода esp::vt::interpreter.
 */


#include "esp_vt_command.h"

#include <cstdint>


namespace esp::vt
{

/**
 * @brief Исполнитель того, что разобрал esp::vt::parser.
 *
 * Разборщик вызывает его методы по мере разбора потока: для каждого знака,
 * управляющего байта и завершённой последовательности CSI или ESC. Главный
 * исполнитель — экран esp::vt::screen; проверки подставляют свой, чтобы
 * записать вызовы. Строки OSC, DCS, SOS, PM и APC разборщик пропускает и
 * исполнителю не передаёт.
 */
class interpreter
{

public:

    virtual ~interpreter() = default;

    virtual void print(std::uint32_t code_point) = 0;
    virtual void execute(std::uint8_t control) = 0;
    virtual void csi(const esp::vt::command &command) = 0;
    virtual void esc(const esp::vt::command &command) = 0;
};

}


// Реализация

// Открытые

/**
 * @fn void esp::vt::interpreter::print(std::uint32_t code_point)
 * @brief Выводит знак.
 *
 * Разборщик уже собрал знак из байтов UTF-8; испорченная последовательность
 * приходит как @c esp::vt::REPLACEMENT_CHARACTER.
 *
 * @param code_point Код знака Unicode.
 */

/**
 * @fn void esp::vt::interpreter::execute(std::uint8_t control)
 * @brief Выполняет управляющий байт C0.
 *
 * Приходят байты 0x00–0x1F, в том числе BEL, BS, HT, LF и CR, а также CAN
 * и SUB, которые к тому же прерывают начатую последовательность. Что делать
 * с байтом, решает исполнитель.
 *
 * @param control Управляющий байт.
 */

/**
 * @fn void esp::vt::interpreter::csi(const esp::vt::command &command)
 * @brief Выполняет завершённую последовательность CSI (@c ESC @c [).
 * @param command Параметры и последний байт последовательности; действительны до возврата.
 */

/**
 * @fn void esp::vt::interpreter::esc(const esp::vt::command &command)
 * @brief Выполняет завершённую последовательность ESC без CSI, например @c ESC @c 7.
 * @param command Промежуточные и последний байт последовательности; действительны до возврата.
 */
