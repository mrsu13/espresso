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

void keys_tests(esp::test::context &context);
void parser_tests(esp::test::context &context);
void scrollback_tests(esp::test::context &context);
void screen_tests(esp::test::context &context);
void input_parser_tests(esp::test::context &context);
void utf8_tests(esp::test::context &context);
void line_history_tests(esp::test::context &context);
void line_edit_tests(esp::test::context &context);

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
 * @fn void esp::vt::test::keys_tests(esp::test::context &context)
 * @brief Проверяет перевод клавиш PS/2 в байты VT220: знаки, Alt, Ctrl, особые клавиши с признаками.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::parser_tests(esp::test::context &context)
 * @brief Проверяет разборщик esp::vt::parser: UTF-8, управляющие байты, CSI, ESC и строки.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::scrollback_tests(esp::test::context &context)
 * @brief Проверяет историю экрана esp::vt::scrollback: порядок строк, обрезку пустых знакомест, вытеснение старых строк, переход через конец буфера.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::screen_tests(esp::test::context &context)
 * @brief Проверяет экран esp::vt::screen: вывод, курсор, стирание, прокрутку, вид текста и альтернативный экран.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::input_parser_tests(esp::test::context &context)
 * @brief Проверяет разборщик ввода esp::vt::input_parser: знаки, Esc и Alt, особые клавиши, варианты терминалов, клавиши PS/2 туда и обратно.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::utf8_tests(esp::test::context &context)
 * @brief Проверяет действия над UTF-8 esp::vt::utf8: запись знака, шаг по знакам, проверку строки.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::line_history_tests(esp::test::context &context)
 * @brief Проверяет историю строк esp::vt::line_history: порядок, отказ в строке, вытеснение старых строк, переход через конец буфера, поиск.
 * @param context Счёт проверок.
 */

/**
 * @fn void esp::vt::test::line_edit_tests(esp::test::context &context)
 * @brief Проверяет редактор строки esp::vt::line_edit по тому, что он вывел на экран.
 * @param context Счёт проверок.
 */
