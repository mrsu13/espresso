/**
 * @file esp_vt_line_edit_test.cpp
 * @brief Проверки редактора строки esp::vt::line_edit: вывод проверяется на экране esp::vt::screen.
 */

#include "esp_vt_tests.h"

#include "esp_vt_config.h"
#include "esp_vt_input_parser.h"
#include "esp_vt_key_event.h"
#include "esp_vt_line_edit.h"
#include "esp_vt_line_history.h"
#include "esp_vt_parser.h"
#include "esp_vt_screen.h"
#include "esp_vt_utf8.h"
#include "esp_vt_writer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>


namespace esp::vt::test::p
{

/**
 * @brief Экран с разборщиком — получатель вывода редактора строки.
 */
class edit_screen : public esp::vt::writer
{

public:

    edit_screen(int columns, int rows);

    void write(std::string_view bytes) override;
    std::string row(int row) const;

    /** @brief Экран, на котором видно, что вывел редактор. */
    esp::vt::screen screen;

    /** @brief Разборщик, исполнитель которого — @c screen. */
    esp::vt::parser parser;

    /** @brief Сколько звонков (BEL) вывел редактор. */
    int bells;
};

/** @brief Слова для проверки дополнения. */
constexpr std::string_view COMPLETION_WORDS[] = {"help", "hello", "history", "mode"};

/**
 * @brief Набирает клавиши: байты идут через разборщик ввода, каждое событие — в редактор.
 *
 * ESC в конце текста — клавиша Esc: разборщик не дождался продолжения.
 *
 * @param edit Редактор строки.
 * @param text Байты, как от терминала; @c ^[ — ESC (esp::vt::test::caret()).
 * @return Чем кончилась обработка последнего события.
 */
esp::vt::line_edit::result type_keys(esp::vt::line_edit &edit, std::string_view text)
{
    esp::vt::input_parser parser;
    esp::vt::line_edit::result result = esp::vt::line_edit::result::editing;
    for (const char byte : esp::vt::test::caret(text))
    {
        if (const std::optional<esp::vt::key_event> key =
                parser.feed(static_cast<std::uint8_t>(byte)))
            result = edit.feed(*key);
    }
    if (parser.is_pending())
    {
        if (const std::optional<esp::vt::key_event> key = parser.time_out())
            result = edit.feed(*key);
    }
    return result;
}

}


/**
 * @brief Создаёт экран и разборщик.
 * @param columns Число столбцов.
 * @param rows Число строк.
 */
esp::vt::test::p::edit_screen::edit_screen(int columns, int rows)
    : screen{columns, rows}
    , parser{screen}
    , bells{0}
{
}

/**
 * @brief Принимает вывод редактора: считает звонки и отдаёт байты разборщику.
 * @param bytes Байты потока вывода.
 */
void esp::vt::test::p::edit_screen::write(std::string_view bytes)
{
    for (const char byte : bytes)
    {
        if (byte == '\a')
            ++bells;
    }
    parser.feed(bytes);
}

/**
 * @brief Возвращает текст строки экрана без пробелов в конце.
 * @param row Строка, с 0.
 * @return Текст в UTF-8.
 */
std::string esp::vt::test::p::edit_screen::row(int row) const
{
    std::string text;
    for (int column = 0; column < screen.columns(); ++column)
    {
        std::array<char, esp::vt::utf8::MAX_BYTES> bytes{};
        const int count = esp::vt::utf8::encode(screen.cell(row, column).code_point, bytes);
        text.append(bytes.data(), static_cast<std::size_t>(count));
    }
    text.erase(text.find_last_not_of(' ') + 1);
    return text;
}


/**
 * @brief Проверяет редактор строки.
 * @param context Счёт проверок.
 */
void esp::vt::test::line_edit_tests(esp::test::context &context)
{
    context.set_case("typing and Enter");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        ESP_CHECK_EQUAL(context, out.row(0), std::string{">"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        ESP_CHECK(context,
                  esp::vt::test::p::type_keys(edit, "hello")
                      == esp::vt::line_edit::result::editing);
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> hello"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 7);
        ESP_CHECK(context,
                  esp::vt::test::p::type_keys(edit, "\r") == esp::vt::line_edit::result::accepted);
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"hello"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
    }

    context.set_case("cursor keys");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        esp::vt::test::p::type_keys(edit, "abc^[[D^[[DX");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"aXbc"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> aXbc"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 4);
        esp::vt::test::p::type_keys(edit, "^[[H");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "^[[F");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 6);
        esp::vt::test::p::type_keys(edit, "\x01");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "\x05\x02");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 5);
        esp::vt::test::p::type_keys(edit, "\x06\x06");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 6);
    }

    context.set_case("deleting");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        esp::vt::test::p::type_keys(edit, "abcd^[[D\x7f");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"abd"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abd"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 4);
        esp::vt::test::p::type_keys(edit, "^[[3~\x04");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"ab"});
        esp::vt::test::p::type_keys(edit, "\x01\x04\x08");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"b"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> b"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
    }

    context.set_case("words, kill and yank");
    {
        esp::vt::test::p::edit_screen out{40, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 40);
        esp::vt::test::p::type_keys(edit, "dup swap  over^[[1;5D");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 12);
        esp::vt::test::p::type_keys(edit, "^[b");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 6);
        esp::vt::test::p::type_keys(edit, "^[f");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 10);
        esp::vt::test::p::type_keys(edit, "^[[1;5C");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 16);
        esp::vt::test::p::type_keys(edit, "\x17");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"dup swap  "});
        esp::vt::test::p::type_keys(edit, "\x19");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"dup swap  over"});
        esp::vt::test::p::type_keys(edit, "\x01^[d");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{" swap  over"});
        esp::vt::test::p::type_keys(edit, "\x0b");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{">"});
        esp::vt::test::p::type_keys(edit, "\x19");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{" swap  over"});
        esp::vt::test::p::type_keys(edit, "\x15");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{});
        esp::vt::test::p::type_keys(edit, "a b^[\x7f");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"a "});
    }

    context.set_case("UTF-8");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        esp::vt::test::p::type_keys(edit, "жук");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> жук"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 5);
        esp::vt::test::p::type_keys(edit, "^[[D\x7f");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"жк"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> жк"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 3);
    }

    context.set_case("wrapping");
    {
        esp::vt::test::p::edit_screen out{10, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 10);
        esp::vt::test::p::type_keys(edit, "abcdefghijklmnopqrst");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abcdefgh"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"ijklmnopqr"});
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"st"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 2);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "\x01X");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> Xabcdefg"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"hijklmnopq"});
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"rst"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 3);
        esp::vt::test::p::type_keys(edit, "\x7f");
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"st"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "\x05");
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 2);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "\x01\r");
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 3);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"st"});
    }

    context.set_case("wrapping: kill to the end");
    {
        esp::vt::test::p::edit_screen out{10, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 10);
        esp::vt::test::p::type_keys(edit, "abcdefghijklmnopqrst\x01\x0b");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{">"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{});
        ESP_CHECK_EQUAL(context, out.row(2), std::string{});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
    }

    context.set_case("line ends at the right edge");
    {
        esp::vt::test::p::edit_screen out{10, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 10);
        esp::vt::test::p::type_keys(edit, "abcdefgh");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abcdefgh"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
        esp::vt::test::p::type_keys(edit, "\x7f");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abcdefg"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 9);
        esp::vt::test::p::type_keys(edit, "h\r");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"abcdefgh"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
    }

    context.set_case("Ctrl+C");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        ESP_CHECK(context,
                  esp::vt::test::p::type_keys(edit, "abc\x03")
                      == esp::vt::line_edit::result::cancelled);
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abc^C"});
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
    }

    context.set_case("history");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        history.add("first");
        history.add("second");
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 20);
        esp::vt::test::p::type_keys(edit, "^[[A");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> second"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 8);
        esp::vt::test::p::type_keys(edit, "^[[A");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> first"});
        esp::vt::test::p::type_keys(edit, "^[[A");
        ESP_CHECK_EQUAL(context, out.bells, 1);
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"first"});
        esp::vt::test::p::type_keys(edit, "^[[B");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"second"});
        esp::vt::test::p::type_keys(edit, "^[[B");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{">"});
        esp::vt::test::p::type_keys(edit, "^[[B");
        ESP_CHECK_EQUAL(context, out.bells, 2);
        esp::vt::test::p::type_keys(edit, "dra\x10");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"second"});
        esp::vt::test::p::type_keys(edit, "\x0e");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"dra"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> dra"});
    }

    context.set_case("search");
    {
        // Экран шире приглашения неудачного поиска со строкой: 43 знака.
        esp::vt::test::p::edit_screen out{60, 6};
        esp::vt::line_history history{1024, 32};
        for (const std::string_view line : {"2 3 + .", ": sq dup * ;", "5 sq ."})
            history.add(line);
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 60);
        esp::vt::test::p::type_keys(edit, "x\x12");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"(reverse-i-search)`': x"});
        esp::vt::test::p::type_keys(edit, "sq");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"(reverse-i-search)`sq': 5 sq ."});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 26);
        esp::vt::test::p::type_keys(edit, "\x12");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"(reverse-i-search)`sq': : sq dup * ;"});
        esp::vt::test::p::type_keys(edit, "\x12");
        ESP_CHECK_EQUAL(context, out.bells, 1);
        ESP_CHECK_EQUAL(context, out.row(0),
                        std::string{"(failed reverse-i-search)`sq': : sq dup * ;"});
        ESP_CHECK(context,
                  esp::vt::test::p::type_keys(edit, "\r") == esp::vt::line_edit::result::accepted);
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{": sq dup * ;"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> : sq dup * ;"});
    }

    context.set_case("search: cancel");
    {
        esp::vt::test::p::edit_screen out{40, 6};
        esp::vt::line_history history{1024, 32};
        for (const std::string_view line : {"2 3 + .", ": sq dup * ;", "5 sq ."})
            history.add(line);
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 40);
        esp::vt::test::p::type_keys(edit,
                                    "x\x12"
                                    "2 3");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"(reverse-i-search)`2 3': 2 3 + ."});
        esp::vt::test::p::type_keys(edit, "\x07");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"x"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> x"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 3);
        esp::vt::test::p::type_keys(edit,
                                    "\x12"
                                    "dup");
        esp::vt::test::p::type_keys(edit, "^[");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"x"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> x"});
    }

    context.set_case("search: edit the match");
    {
        esp::vt::test::p::edit_screen out{40, 6};
        esp::vt::line_history history{1024, 32};
        for (const std::string_view line : {"2 3 + .", ": sq dup * ;", "5 sq ."})
            history.add(line);
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 40);
        esp::vt::test::p::type_keys(edit,
                                    "\x12"
                                    "dup^[[C");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{": sq dup * ;"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> : sq dup * ;"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 8);
        esp::vt::test::p::type_keys(edit, "^[[B");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"5 sq ."});
        esp::vt::test::p::type_keys(edit, "^[[B");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{});
    }

    context.set_case("completion");
    {
        esp::vt::test::p::edit_screen out{40, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.set_words(esp::vt::test::p::COMPLETION_WORDS);
        for (const std::string_view keys : {"mo\t", "help mo\t", "hi\t", "mode\t"})
        {
            edit.start("> ", 40);
            esp::vt::test::p::type_keys(edit, keys);
            esp::vt::test::p::type_keys(edit, "\r");
        }
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> mode"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"> help mode"});
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"> history"});
        ESP_CHECK_EQUAL(context, out.row(3), std::string{"> mode"});
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"mode "});
        ESP_CHECK_EQUAL(context, out.bells, 0);
    }

    context.set_case("completion: common prefix and list");
    {
        esp::vt::test::p::edit_screen out{40, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.set_words(esp::vt::test::p::COMPLETION_WORDS);
        edit.start("> ", 40);
        esp::vt::test::p::type_keys(edit, "HE\t");
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"hel"});
        ESP_CHECK_EQUAL(context, out.bells, 0);
        esp::vt::test::p::type_keys(edit, "\t");
        ESP_CHECK_EQUAL(context, out.bells, 1);
        ESP_CHECK_EQUAL(context, out.row(1), std::string{});
        esp::vt::test::p::type_keys(edit, "\t");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> hel"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"hello  help"});
        ESP_CHECK_EQUAL(context, out.row(2), std::string{"> hel"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 2);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 5);
        esp::vt::test::p::type_keys(edit, "\x15x\t");
        ESP_CHECK_EQUAL(context, out.bells, 2);
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"x"});
    }

    context.set_case("Ctrl+L");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        out.write("junk\r\n");
        edit.start("> ", 20);
        esp::vt::test::p::type_keys(edit, "abc");
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"> abc"});
        esp::vt::test::p::type_keys(edit, "\x0c");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> abc"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 5);
    }

    context.set_case("full line");
    {
        esp::vt::test::p::edit_screen out{80, 8};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("> ", 80);
        esp::vt::test::p::type_keys(edit, std::string(esp::vt::MAX_LINE_BYTES, 'x'));
        ESP_CHECK_EQUAL(context, static_cast<long long>(edit.line().size()),
                        esp::vt::MAX_LINE_BYTES);
        ESP_CHECK_EQUAL(context, out.bells, 0);
        esp::vt::test::p::type_keys(edit, "y");
        ESP_CHECK_EQUAL(context, out.bells, 1);
        esp::vt::test::p::type_keys(edit, "\x7fж");
        ESP_CHECK_EQUAL(context, out.bells, 2);
        esp::vt::test::p::type_keys(edit, "y");
        ESP_CHECK_EQUAL(context, static_cast<long long>(edit.line().size()),
                        esp::vt::MAX_LINE_BYTES);
        ESP_CHECK(context, edit.line().back() == 'y');
    }

    context.set_case("start after the caller's text");
    {
        esp::vt::test::p::edit_screen out{10, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        out.write("Name? ");
        edit.start("", 10, out.screen.cursor_column());
        esp::vt::test::p::type_keys(edit, "Bob");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"Name? Bob"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 9);
        esp::vt::test::p::type_keys(edit, "\x01");
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 6);
        esp::vt::test::p::type_keys(edit,
                                    "\x05"
                                    "ert");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"Name? Bobe"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{"rt"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 2);
        esp::vt::test::p::type_keys(edit, "\x01\x0b");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"Name?"});
        ESP_CHECK_EQUAL(context, out.row(1), std::string{});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 6);
        ESP_CHECK(context,
                  esp::vt::test::p::type_keys(edit, "Al\r")
                      == esp::vt::line_edit::result::accepted);
        ESP_CHECK_EQUAL(context, std::string{edit.line()}, std::string{"Al"});
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"Name? Al"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_row(), 1);
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 0);
    }

    context.set_case("prompt with colour");
    {
        esp::vt::test::p::edit_screen out{20, 6};
        esp::vt::line_history history{1024, 32};
        esp::vt::line_edit edit{out, history};
        edit.start("\x1b[93m>\x1b[0m ", 20);
        esp::vt::test::p::type_keys(edit, "a");
        ESP_CHECK_EQUAL(context, out.row(0), std::string{"> a"});
        ESP_CHECK_EQUAL(context, out.screen.cursor_column(), 3);
    }
}
