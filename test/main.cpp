/**
 * @file main.cpp
 * @brief Точка входа проверок на ПК: запускает все наборы и сообщает итог.
 */

#include "esp_test.h"
#include "esp_vt_tests.h"

#include <cstdio>


/**
 * @brief Запускает все наборы проверок.
 * @return 0, если все проверки прошли, иначе 1.
 */
int main()
{
    esp::test::context context;
    esp::vt::test::keys_tests(context);
    esp::vt::test::parser_tests(context);
    esp::vt::test::scrollback_tests(context);
    esp::vt::test::screen_tests(context);
    esp::vt::test::input_parser_tests(context);
    esp::vt::test::utf8_tests(context);
    esp::vt::test::line_history_tests(context);
    esp::vt::test::line_edit_tests(context);
    std::printf("%d checks, %d failed.\n", context.checks(), context.failures());
    return context.failures() == 0 ? 0 : 1;
}
