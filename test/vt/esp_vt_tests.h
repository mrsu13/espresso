#pragma once


/**
 * @file
 * @brief Наборы проверок терминала esp::vt.
 */


#include "esp_test.h"

#include <cstddef>
#include <string>
#include <string_view>


namespace esp::vt::test
{

void parser_tests(esp::test::context &context);
void screen_tests(esp::test::context &context);

inline std::string caret(std::string_view text);

}


// Реализация

// Открытые

/**
 * @brief Переводит запись с @c ^[ вместо ESC в байты потока.
 *
 * Проверки пишут ESC как @c ^[ — так его обозначают в описаниях
 * терминалов. Запись @c \\x1b перед цифрой или буквой A–F пришлось бы
 * разрывать на две строки: иначе компилятор принял бы следующий знак за
 * часть кода байта.
 *
 * @param text Запись: @c ^[ — ESC, остальные знаки — как есть.
 * @return Байты потока.
 */
std::string esp::vt::test::caret(std::string_view text)
{
    std::string bytes;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '^' && i + 1 < text.size() && text[i + 1] == '[')
        {
            bytes += '\x1b';
            ++i;
        }
        else
            bytes += text[i];
    }
    return bytes;
}

/**
 * @fn void esp::vt::test::parser_tests(esp::test::context &context)
 * @brief Проверяет разборщик esp::vt::parser: UTF-8, управляющие байты, CSI, ESC и строки.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::screen_tests(esp::test::context &context)
 * @brief Проверяет экран esp::vt::screen: вывод, курсор, стирание, прокрутку, вид текста и альтернативный экран.
 * @param context Счёт проверок.
 */
