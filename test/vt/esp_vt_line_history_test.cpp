/**
 * @file esp_vt_line_history_test.cpp
 * @brief Проверки истории строк esp::vt::line_history и действий над UTF-8 esp::vt::utf8.
 */

#include "esp_vt_tests.h"

#include "esp_vt_config.h"
#include "esp_vt_line_history.h"
#include "esp_vt_utf8.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>


namespace esp::vt::test::p
{

/**
 * @brief Возвращает строки истории от самой старой к самой новой через «|».
 * @param history История.
 * @return Строки истории.
 */
std::string history_lines(const esp::vt::line_history &history)
{
    std::string text;
    for (int age = history.size() - 1; age >= 0; --age)
    {
        if (!text.empty())
            text += '|';
        text += history.line(age);
    }
    return text;
}

/**
 * @brief Записывает знак в UTF-8 через esp::vt::utf8::encode().
 * @param code_point Код знака.
 * @return Байты знака.
 */
std::string encoded(std::uint32_t code_point)
{
    std::array<char, esp::vt::utf8::MAX_BYTES> bytes{};
    const int count = esp::vt::utf8::encode(code_point, bytes);
    return std::string{bytes.data(), static_cast<std::size_t>(count)};
}

}


/**
 * @brief Проверяет действия над UTF-8.
 * @param context Счёт проверок.
 */
void esp::vt::test::utf8_tests(esp::test::context &context)
{
    context.set_case("encode");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded('a'), std::string{"a"});
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded(0x0436), std::string{"ж"});
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded(0x20AC), std::string{"€"});
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded(0x1F600), std::string{"\xF0\x9F\x98\x80"});
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded(0xD800), std::string{"\xEF\xBF\xBD"});
    ESP_CHECK_EQUAL(context, esp::vt::test::p::encoded(0x110000), std::string{"\xEF\xBF\xBD"});

    context.set_case("steps and length");
    const std::string_view text = "aжb€";
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::next(text, 0)), 1);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::next(text, 1)), 3);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::next(text, 4)), 7);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::next(text, 7)), 7);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::previous(text, 7)), 4);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::previous(text, 3)), 1);
    ESP_CHECK_EQUAL(context, static_cast<long long>(esp::vt::utf8::previous(text, 0)), 0);
    ESP_CHECK_EQUAL(context, esp::vt::utf8::length(text), 4);
    ESP_CHECK_EQUAL(context, esp::vt::utf8::length(""), 0);

    context.set_case("printable");
    ESP_CHECK(context, esp::vt::utf8::is_printable('a'));
    ESP_CHECK(context, esp::vt::utf8::is_printable(0x0436));
    ESP_CHECK(context, !esp::vt::utf8::is_printable(0x09));
    ESP_CHECK(context, !esp::vt::utf8::is_printable(0x7F));
    ESP_CHECK(context, !esp::vt::utf8::is_printable(0x85));
    ESP_CHECK(context, !esp::vt::utf8::is_printable(0xDC00));
    ESP_CHECK(context, esp::vt::utf8::is_printable_line("dup + ."));
    ESP_CHECK(context, esp::vt::utf8::is_printable_line("ёлка €"));
    ESP_CHECK(context, esp::vt::utf8::is_printable_line(""));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("tab\there"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\xD0"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\xD0z"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\xC0\xAF"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\xED\xA0\x80"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\x80"));
    ESP_CHECK(context, !esp::vt::utf8::is_printable_line("\xC2\x85"));
}

/**
 * @brief Проверяет историю строк.
 * @param context Счёт проверок.
 */
void esp::vt::test::line_history_tests(esp::test::context &context)
{
    context.set_case("order");
    {
        esp::vt::line_history history{256, 16};
        ESP_CHECK_EQUAL(context, history.size(), 0);
        ESP_CHECK(context, history.add("one"));
        ESP_CHECK(context, history.add("two"));
        ESP_CHECK(context, history.add("three"));
        ESP_CHECK_EQUAL(context, history.size(), 3);
        ESP_CHECK_EQUAL(context, std::string{history.line(0)}, std::string{"three"});
        ESP_CHECK_EQUAL(context, std::string{history.line(2)}, std::string{"one"});
        history.clear();
        ESP_CHECK_EQUAL(context, history.size(), 0);
    }

    context.set_case("rejected lines");
    {
        esp::vt::line_history history{256, 16};
        ESP_CHECK(context, !history.add(""));
        ESP_CHECK(context, !history.add("   "));
        ESP_CHECK(context, history.add("dup"));
        ESP_CHECK(context, !history.add("dup"));
        ESP_CHECK(context, history.add("swap"));
        ESP_CHECK(context, history.add("dup"));
        ESP_CHECK(context, !history.add("bad\r"));
        ESP_CHECK(context, !history.add("\xD0"));
        ESP_CHECK(context, !history.add(std::string(esp::vt::MAX_LINE_BYTES + 1, 'x')));
        ESP_CHECK(context, history.add(std::string(esp::vt::MAX_LINE_BYTES, 'y').substr(0, 200)));
        ESP_CHECK_EQUAL(context, history.size(), 4);
    }

    context.set_case("line capacity");
    {
        esp::vt::line_history history{256, 3};
        for (const char *line : {"a1", "a2", "a3", "a4", "a5"})
            history.add(line);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history), std::string{"a3|a4|a5"});
    }

    context.set_case("byte capacity and wrap");
    {
        // 16 байтов: строки по 5 байтов помещаются по три, четвёртая
        // уходит в начало буфера, а конец буфера пустует.
        esp::vt::line_history history{16, 8};
        for (const char *line : {"line1", "line2", "line3"})
            history.add(line);
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"line1|line2|line3"});
        history.add("line4");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"line2|line3|line4"});
        history.add("line5");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"line3|line4|line5"});
        history.add("line6");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"line4|line5|line6"});
        // Длинная строка вытесняет столько старых, сколько нужно.
        history.add("long line 12");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"long line 12"});
        history.add("ab");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"long line 12|ab"});
        history.add("cdef");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history), std::string{"ab|cdef"});
        // Строка во весь буфер.
        history.add("0123456789abcdef");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history),
                        std::string{"0123456789abcdef"});
        history.add("x");
        ESP_CHECK_EQUAL(context, esp::vt::test::p::history_lines(history), std::string{"x"});
    }

    context.set_case("many lines");
    {
        esp::vt::line_history history{esp::vt::HISTORY_BYTES, esp::vt::HISTORY_LINES};
        for (int i = 0; i < 2000; ++i)
            history.add("command number " + std::to_string(i));
        ESP_CHECK(context, history.size() > 100);
        ESP_CHECK_EQUAL(context, std::string{history.line(0)}, std::string{"command number 1999"});
        ESP_CHECK_EQUAL(context, std::string{history.line(1)}, std::string{"command number 1998"});
        const int oldest = 2000 - history.size();
        ESP_CHECK_EQUAL(context, std::string{history.line(history.size() - 1)},
                        "command number " + std::to_string(oldest));
    }

    context.set_case("find");
    {
        esp::vt::line_history history{256, 16};
        for (const char *line : {"2 3 + .", "words", ": sq dup * ;", "5 sq ."})
            history.add(line);
        ESP_CHECK_EQUAL(context, history.find("sq", 0), 0);
        ESP_CHECK_EQUAL(context, history.find("sq", 1), 1);
        ESP_CHECK_EQUAL(context, history.find("sq", 2), -1);
        ESP_CHECK_EQUAL(context, history.find("+", -5), 3);
        ESP_CHECK_EQUAL(context, history.find("", 2), 2);
        ESP_CHECK_EQUAL(context, history.find("none", 0), -1);
    }
}
