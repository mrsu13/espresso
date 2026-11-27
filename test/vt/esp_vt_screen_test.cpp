/**
 * @file esp_vt_screen_test.cpp
 * @brief Проверки модели экрана esp::vt::screen.
 */

#include "esp_vt_tests.h"

#include "esp_vt_cell.h"
#include "esp_vt_parser.h"
#include "esp_vt_screen.h"

#include <cstdint>
#include <string>
#include <string_view>


namespace esp::vt::test::p
{

/**
 * @brief Экран с разборщиком: то, что выведено в терминал, сразу оказывается на экране.
 */
class terminal
{

public:

    terminal(int columns, int rows);

    void write(std::string_view text);
    std::string row(int row) const;
    std::string view_row(int row) const;

    /** @brief Проверяемый экран. */
    esp::vt::screen screen;

    /** @brief Разборщик, исполнитель которого — @c screen. */
    esp::vt::parser parser;
};

/**
 * @brief Дописывает знак в UTF-8.
 * @param text Строка.
 * @param code_point Код знака из основной плоскости.
 */
void append_utf8(std::string &text, std::uint32_t code_point)
{
    if (code_point < 0x80)
        text += static_cast<char>(code_point);
    else if (code_point < 0x800)
    {
        text += static_cast<char>(0xC0 | (code_point >> 6));
        text += static_cast<char>(0x80 | (code_point & 0x3F));
    }
    else
    {
        text += static_cast<char>(0xE0 | (code_point >> 12));
        text += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (code_point & 0x3F));
    }
}

}


/**
 * @brief Создаёт экран и разборщик.
 * @param columns Число столбцов.
 * @param rows Число строк.
 */
esp::vt::test::p::terminal::terminal(int columns, int rows)
    : screen{columns, rows}
    , parser{screen}
{
}

/**
 * @brief Выводит текст.
 * @param text Поток вывода; @c ^[ — ESC (esp::vt::test::caret()).
 */
void esp::vt::test::p::terminal::write(std::string_view text)
{
    parser.feed(esp::vt::test::caret(text));
}

/**
 * @brief Возвращает текст строки экрана без пробелов в конце.
 * @param row Строка, с 0.
 * @return Текст в UTF-8.
 */
std::string esp::vt::test::p::terminal::row(int row) const
{
    std::string text;
    for (int column = 0; column < screen.columns(); ++column)
        esp::vt::test::p::append_utf8(text, screen.cell(row, column).code_point);
    text.erase(text.find_last_not_of(' ') + 1);
    return text;
}

/**
 * @brief Возвращает текст строки, показанной на экране с учётом сдвига в историю.
 * @param row Строка экрана, с 0.
 * @return Текст в UTF-8 без пробелов в конце.
 */
std::string esp::vt::test::p::terminal::view_row(int row) const
{
    std::string text;
    for (int column = 0; column < screen.columns(); ++column)
        esp::vt::test::p::append_utf8(text, screen.view_cell(row, column).code_point);
    text.erase(text.find_last_not_of(' ') + 1);
    return text;
}


// Открытые

void esp::vt::test::screen_tests(esp::test::context &context)
{
    context.set_case("screen: print");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("abc");
        ESP_CHECK_EQUAL(context, t.row(0), "abc");
        ESP_CHECK_EQUAL(context, t.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, t.screen.cursor_column(), 3);
        t.write("\r\nЖ▶");
        ESP_CHECK_EQUAL(context, t.row(1), "Ж▶");
        t.write("\xF0\x9F\x98\x80");
        ESP_CHECK_EQUAL(context, t.screen.cell(1, 2).code_point, 0xFFFD);
    }

    context.set_case("screen: LF keeps the column");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("ab\ncd");
        ESP_CHECK_EQUAL(context, t.row(1), "  cd");
    }

    context.set_case("screen: deferred wrap");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("0123456789");
        ESP_CHECK_EQUAL(context, t.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, t.screen.cursor_column(), 9);
        t.write("\r\nX");
        ESP_CHECK_EQUAL(context, t.row(1), "X");
        t.write("\r\n0123456789Y");
        ESP_CHECK_EQUAL(context, t.row(2), "0123456789");
        ESP_CHECK_EQUAL(context, t.row(3), "Y");
    }

    context.set_case("screen: autowrap off");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("^[[?7l0123456789AB");
        ESP_CHECK_EQUAL(context, t.row(0), "012345678B");
        ESP_CHECK_EQUAL(context, t.row(1), "");
    }

    context.set_case("screen: scroll and changes");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("1\r\n2\r\n3\r\n4");
        t.screen.clear_changes();
        t.write("\r\n5");
        ESP_CHECK_EQUAL(context, t.row(0), "2");
        ESP_CHECK_EQUAL(context, t.row(3), "5");
        ESP_CHECK_EQUAL(context, t.screen.pending_scroll(), 1);
        ESP_CHECK_EQUAL(context, static_cast<long long>(t.screen.changed_rows()), 0b1000);
        t.screen.clear_changes();
        t.write("^[[2;1HZ");
        ESP_CHECK_EQUAL(context, t.screen.pending_scroll(), 0);
        ESP_CHECK_EQUAL(context, static_cast<long long>(t.screen.changed_rows()), 0b0010);
    }

    context.set_case("screen: cursor movement");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("^[[2;3HX^[[99;99HY^[[1;1H^[[3C^[[BZ");
        ESP_CHECK_EQUAL(context, t.row(1), "  XZ");
        ESP_CHECK_EQUAL(context, t.row(3), "         Y");
        t.write("^[[4;5H^[[2FA^[[EB^[[3GC^[[1dD");
        ESP_CHECK_EQUAL(context, t.row(0), "   D");
        ESP_CHECK_EQUAL(context, t.row(1), "A XZ");
        ESP_CHECK_EQUAL(context, t.row(2), "B C");
        t.write("^[[1;1Ha\tb");
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 8).code_point, 'b');
    }

    context.set_case("screen: erase");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("0123456789\r\nabcdefghij\r\nABCDEFGHIJ");
        t.write("^[[2;4H^[[K");
        ESP_CHECK_EQUAL(context, t.row(1), "abc");
        t.write("^[[3;4H^[[1K");
        ESP_CHECK_EQUAL(context, t.row(2), "    EFGHIJ");
        t.write("^[[1;5H^[[44m^[[J");
        ESP_CHECK_EQUAL(context, t.row(0), "0123");
        ESP_CHECK_EQUAL(context, t.row(2), "");
        ESP_CHECK_EQUAL(context, t.screen.cell(3, 0).style.background, 4);
        t.write("^[[2J");
        ESP_CHECK_EQUAL(context, t.row(0), "");
    }

    context.set_case("screen: insert and delete characters");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("abcdef\r^[[2@");
        ESP_CHECK_EQUAL(context, t.row(0), "  abcdef");
        t.write("^[[2P");
        ESP_CHECK_EQUAL(context, t.row(0), "abcdef");
        t.write("^[[3X");
        ESP_CHECK_EQUAL(context, t.row(0), "   def");
        t.write("^[[1;9H^[[5@");
        ESP_CHECK_EQUAL(context, t.row(0), "   def");
    }

    context.set_case("screen: insert and delete lines");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("1\r\n2\r\n3\r\n4^[[2;1H^[[L");
        ESP_CHECK_EQUAL(context, t.row(1), "");
        ESP_CHECK_EQUAL(context, t.row(2), "2");
        ESP_CHECK_EQUAL(context, t.row(3), "3");
        t.write("^[[2M");
        ESP_CHECK_EQUAL(context, t.row(1), "3");
        ESP_CHECK_EQUAL(context, t.row(2), "");
    }

    context.set_case("screen: scroll region");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("top^[[4;1Hbottom^[[2;3r");
        ESP_CHECK_EQUAL(context, t.screen.cursor_row(), 0);
        t.screen.clear_changes();
        t.write("^[[2;1Ha\r\nb\r\nc\r\nd");
        ESP_CHECK_EQUAL(context, t.row(0), "top");
        ESP_CHECK_EQUAL(context, t.row(1), "c");
        ESP_CHECK_EQUAL(context, t.row(2), "d");
        ESP_CHECK_EQUAL(context, t.row(3), "bottom");
        ESP_CHECK_EQUAL(context, t.screen.pending_scroll(), 0);
        ESP_CHECK_EQUAL(context, static_cast<long long>(t.screen.changed_rows()), 0b0110);
        t.write("^[[2;1H^[M");
        ESP_CHECK_EQUAL(context, t.row(1), "");
        ESP_CHECK_EQUAL(context, t.row(2), "c");
        t.write("^[[r^[[4;1H\n");
        ESP_CHECK_EQUAL(context, t.row(0), "");
        ESP_CHECK_EQUAL(context, t.row(2), "bottom");
    }

    context.set_case("screen: SGR");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("^[[1;31;44mA^[[0mB^[[3;4;7;8mC^[[22;23;24;27;28mD");
        const esp::vt::style a = t.screen.cell(0, 0).style;
        ESP_CHECK(context, a.bold && !a.dim && a.foreground == 1 && a.background == 4);
        ESP_CHECK(context, t.screen.cell(0, 1).style == esp::vt::style{});
        const esp::vt::style c = t.screen.cell(0, 2).style;
        ESP_CHECK(context, c.italic && c.underlined && c.inverse && c.hidden);
        ESP_CHECK(context, t.screen.cell(0, 3).style == esp::vt::style{});
        t.write("^[[38;5;196mE^[[38;5;3mF^[[48;5;232mG");
        t.write("^[[38;2;250;250;250mH^[[95;105mI^[[39;49mJ");
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 4).style.foreground, 1);
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 5).style.foreground, 3);
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 6).style.background, 0);
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 7).style.foreground, 15);
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 8).style.foreground, 13);
        ESP_CHECK_EQUAL(context, t.screen.cell(0, 8).style.background, 13);
        ESP_CHECK(context, t.screen.cell(0, 9).style == esp::vt::style{});
    }

    context.set_case("screen: SGR keeps the pending wrap");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("012345678^[[31m9^[[32m");
        ESP_CHECK(context, t.screen.is_wrap_pending());
        ESP_CHECK_EQUAL(context, t.screen.cursor_column(), 9);
        t.write("X");
        ESP_CHECK(context, !t.screen.is_wrap_pending());
        ESP_CHECK_EQUAL(context, t.row(0), "0123456789");
        ESP_CHECK_EQUAL(context, t.row(1), "X");
        ESP_CHECK_EQUAL(context, t.screen.cell(1, 0).style.foreground, 2);
    }

    context.set_case("screen: private and intermediate sequences are skipped");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("^[[>4;2mA^[[2 qB^[(BC");
        ESP_CHECK_EQUAL(context, t.row(0), "ABC");
        ESP_CHECK(context, t.screen.cell(0, 0).style == esp::vt::style{});
    }

    context.set_case("screen: save and restore cursor");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("ab^[7^[[3;3H^[[31mX^[8Y");
        ESP_CHECK_EQUAL(context, t.row(0), "abY");
        ESP_CHECK(context, t.screen.cell(0, 2).style == esp::vt::style{});
        t.write("^[[s^[[4;4H^[[uZ");
        ESP_CHECK_EQUAL(context, t.row(0), "abYZ");
    }

    context.set_case("screen: alternate screen");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("main^[[2;3r^[[1;5H^[[?1049h");
        ESP_CHECK(context, t.screen.is_alternate());
        ESP_CHECK_EQUAL(context, t.row(0), "");
        t.write("^[[Halt^[[?25l");
        ESP_CHECK_EQUAL(context, t.row(0), "alt");
        t.screen.clear_changes();
        t.write("^[[?1049l");
        ESP_CHECK(context, !t.screen.is_alternate());
        ESP_CHECK_EQUAL(context, t.row(0), "main");
        ESP_CHECK_EQUAL(context, t.screen.cursor_row(), 0);
        ESP_CHECK_EQUAL(context, t.screen.cursor_column(), 4);
        ESP_CHECK(context, !t.screen.is_cursor_visible());
        ESP_CHECK_EQUAL(context, static_cast<long long>(t.screen.changed_rows()), 0b1111);
        t.write("^[[?1049l");
        ESP_CHECK_EQUAL(context, t.row(0), "main");
    }

    context.set_case("screen: bell and reset");
    {
        esp::vt::test::p::terminal t{10, 4};
        ESP_CHECK(context, !t.screen.take_bell());
        t.write("x\a");
        ESP_CHECK(context, t.screen.take_bell());
        ESP_CHECK(context, !t.screen.take_bell());
        t.write("^[[31m^[[?1049h^[c");
        ESP_CHECK(context, !t.screen.is_alternate());
        ESP_CHECK_EQUAL(context, t.row(0), "");
        t.write("y");
        ESP_CHECK(context, t.screen.cell(0, 0).style == esp::vt::style{});
    }

    context.set_case("screen: history");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("1\r\n2\r\n3\r\n4\r\n5\r\n6");
        ESP_CHECK_EQUAL(context, t.screen.scrollback().rows(), 2);
        ESP_CHECK_EQUAL(context, t.view_row(0), "3");
        t.screen.clear_changes();
        t.screen.scroll_view(1);
        ESP_CHECK_EQUAL(context, t.screen.view_offset(), 1);
        ESP_CHECK_EQUAL(context, t.view_row(0), "2");
        ESP_CHECK_EQUAL(context, t.view_row(1), "3");
        ESP_CHECK_EQUAL(context, t.view_row(3), "5");
        ESP_CHECK_EQUAL(context, static_cast<long long>(t.screen.changed_rows()), 0b1111);
        t.screen.scroll_view(10);
        ESP_CHECK_EQUAL(context, t.screen.view_offset(), 2);
        ESP_CHECK_EQUAL(context, t.view_row(0), "1");
        ESP_CHECK_EQUAL(context, t.view_row(1), "2");
        t.screen.scroll_view(-1);
        ESP_CHECK_EQUAL(context, t.screen.view_offset(), 1);
        t.write("x");
        ESP_CHECK_EQUAL(context, t.screen.view_offset(), 0);
        ESP_CHECK_EQUAL(context, t.view_row(3), "6x");
    }

    context.set_case("screen: what stays out of the history");
    {
        esp::vt::test::p::terminal t{10, 4};
        t.write("1\r\n2\r\n3\r\n4^[[1;1H^[[M^[[2;4r^[[4;1H\n\n");
        ESP_CHECK_EQUAL(context, t.screen.scrollback().rows(), 0);
        t.write("^[[r^[[?1049h^[[4;1H\n\n\n");
        ESP_CHECK_EQUAL(context, t.screen.scrollback().rows(), 0);
        t.screen.scroll_view(1);
        ESP_CHECK_EQUAL(context, t.screen.view_offset(), 0);
        t.write("^[[?1049l^[[4;1H\nz");
        ESP_CHECK_EQUAL(context, t.screen.scrollback().rows(), 1);
        t.write("^[[3J");
        ESP_CHECK_EQUAL(context, t.screen.scrollback().rows(), 0);
        ESP_CHECK_EQUAL(context, t.row(3), "z");
    }
}
