/**
 * @file esp_vt_scrollback_test.cpp
 * @brief Проверки истории экрана esp::vt::scrollback.
 */

#include "esp_vt_tests.h"

#include "esp_vt_cell.h"
#include "esp_vt_scrollback.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>


namespace esp::vt::test::p
{

/**
 * @brief Добавляет в историю строку из знаков ASCII, дополненную пробелами до ширины.
 * @param history История.
 * @param text Знаки строки.
 * @param columns Ширина строки.
 */
void push_row(esp::vt::scrollback &history, std::string_view text, int columns)
{
    std::vector<esp::vt::cell> cells(static_cast<std::size_t>(columns));
    for (std::size_t i = 0; i < text.size() && i < cells.size(); ++i)
        cells[i].code_point = static_cast<std::uint16_t>(text[i]);
    history.push(std::span<const esp::vt::cell>{cells});
}

/**
 * @brief Возвращает строку истории текстом без пробелов в конце.
 * @param history История.
 * @param row Строка, с 0 — самой новой.
 * @param columns Сколько столбцов читать.
 * @return Знаки строки.
 */
std::string history_row(const esp::vt::scrollback &history, int row, int columns)
{
    std::string text;
    for (int column = 0; column < columns; ++column)
        text += static_cast<char>(history.cell(row, column).code_point);
    text.erase(text.find_last_not_of(' ') + 1);
    return text;
}

}


// Открытые

void esp::vt::test::scrollback_tests(esp::test::context &context)
{
    context.set_case("scrollback: order and trimming");
    {
        esp::vt::scrollback history{64, 8};
        ESP_CHECK_EQUAL(context, history.rows(), 0);
        esp::vt::test::p::push_row(history, "1", 10);
        esp::vt::test::p::push_row(history, "two", 10);
        esp::vt::test::p::push_row(history, "", 10);
        ESP_CHECK_EQUAL(context, history.rows(), 3);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 10), "");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 1, 10), "two");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 2, 10), "1");
        ESP_CHECK(context, history.cell(1, 3) == esp::vt::cell{});
        ESP_CHECK(context, history.cell(1, 99) == esp::vt::cell{});
    }

    context.set_case("scrollback: coloured blanks are kept");
    {
        esp::vt::scrollback history{64, 8};
        std::vector<esp::vt::cell> cells(10);
        cells[0].code_point = 'a';
        cells[5].style.background = 4;
        history.push(std::span<const esp::vt::cell>{cells});
        ESP_CHECK_EQUAL(context, history.cell(0, 5).style.background, 4);
        ESP_CHECK(context, history.cell(0, 6) == esp::vt::cell{});
    }

    context.set_case("scrollback: row limit drops the oldest");
    {
        esp::vt::scrollback history{64, 3};
        for (const char *text : {"a", "b", "c", "d"})
            esp::vt::test::p::push_row(history, text, 4);
        ESP_CHECK_EQUAL(context, history.rows(), 3);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 4), "d");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 2, 4), "b");
    }

    context.set_case("scrollback: cell limit drops the oldest and wraps around");
    {
        esp::vt::scrollback history{8, 16};
        esp::vt::test::p::push_row(history, "abcd", 8);
        esp::vt::test::p::push_row(history, "efgh", 8);
        esp::vt::test::p::push_row(history, "ij", 8);
        ESP_CHECK_EQUAL(context, history.rows(), 2);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 8), "ij");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 1, 8), "efgh");
        esp::vt::test::p::push_row(history, "klmnop", 8);
        ESP_CHECK_EQUAL(context, history.rows(), 2);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 8), "klmnop");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 1, 8), "ij");
    }

    context.set_case("scrollback: a row across the end of the buffer");
    {
        esp::vt::scrollback history{8, 16};
        esp::vt::test::p::push_row(history, "abcdef", 8);
        esp::vt::test::p::push_row(history, "xyz", 8);
        ESP_CHECK_EQUAL(context, history.rows(), 1);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 8), "xyz");
        esp::vt::test::p::push_row(history, "uv", 8);
        ESP_CHECK_EQUAL(context, history.rows(), 2);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 1, 8), "xyz");
    }

    context.set_case("scrollback: a row longer than the history is cut");
    {
        esp::vt::scrollback history{4, 4};
        esp::vt::test::p::push_row(history, "abcdef", 8);
        ESP_CHECK_EQUAL(context, history.rows(), 1);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 8), "abcd");
    }

    context.set_case("scrollback: clear");
    {
        esp::vt::scrollback history{16, 4};
        esp::vt::test::p::push_row(history, "abc", 4);
        history.clear();
        ESP_CHECK_EQUAL(context, history.rows(), 0);
        esp::vt::test::p::push_row(history, "d", 4);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_row(history, 0, 4), "d");
    }
}
