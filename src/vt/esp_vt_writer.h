#pragma once


/**
 * @file
 * @brief Получатель потока вывода esp::vt::writer.
 */


#include <string_view>


namespace esp::vt
{

/**
 * @brief Получатель потока вывода: байтов UTF-8 с последовательностями VT220.
 *
 * Через него выводит редактор строки esp::vt::line_edit. На плате это
 * терминал esp::vt::terminal, в проверках на ПК — разборщик с экраном.
 */
class writer
{

public:

    virtual ~writer() = default;

    virtual void write(std::string_view bytes) = 0;
};

}


// Реализация

// Открытые

/**
 * @fn void esp::vt::writer::write(std::string_view bytes)
 * @brief Выводит байты потока.
 * @param bytes Байты UTF-8 с последовательностями VT220; действительны до возврата.
 */
