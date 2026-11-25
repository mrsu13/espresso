/**
 * @file esp_vt_parser_test.cpp
 * @brief Проверки разборщика потока вывода esp::vt::parser.
 */

#include "esp_vt_tests.h"

#include "esp_vt_command.h"
#include "esp_vt_interpreter.h"
#include "esp_vt_parser.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>


namespace esp::vt::test::p
{

/**
 * @brief Исполнитель, который записывает вызовы разборщика в строку.
 *
 * Запись: печатный знак ASCII — сам знак, другой знак — @c <U+XXXX>,
 * управляющий байт — @c <XX>, последовательность CSI —
 * @c CSI(знак частной последовательности, параметры через запятую,
 * пропущенный — @c -, после @c | — промежуточные байты)последний байт,
 * ESC — @c ESC(промежуточные байты)последний байт.
 */
class recorder : public esp::vt::interpreter
{

public:

    void print(std::uint32_t code_point) override;
    void execute(std::uint8_t control) override;
    void csi(const esp::vt::command &command) override;
    void esc(const esp::vt::command &command) override;

    /** @brief Записанные вызовы. */
    std::string log;

private:

    void append_hex(const char *format, unsigned value);
};

/**
 * @brief Разбирает запись новым разборщиком и возвращает запись вызовов.
 * @param text Поток вывода; @c ^[ — ESC (esp::vt::test::caret()).
 * @return Запись вызовов исполнителя.
 */
std::string parse(std::string_view text)
{
    esp::vt::test::p::recorder recorder;
    esp::vt::parser parser{recorder};
    parser.feed(esp::vt::test::caret(text));
    return recorder.log;
}

}


/**
 * @brief Записывает знак.
 * @param code_point Код знака.
 */
void esp::vt::test::p::recorder::print(std::uint32_t code_point)
{
    if (code_point >= 0x20 && code_point < 0x7F)
        log += static_cast<char>(code_point);
    else
        append_hex("<U+%04X>", code_point);
}

/**
 * @brief Записывает управляющий байт.
 * @param control Управляющий байт.
 */
void esp::vt::test::p::recorder::execute(std::uint8_t control)
{
    append_hex("<%02X>", control);
}

/**
 * @brief Записывает последовательность CSI.
 * @param command Последовательность.
 */
void esp::vt::test::p::recorder::csi(const esp::vt::command &command)
{
    log += "CSI(";
    if (command.private_marker != 0)
        log += static_cast<char>(command.private_marker);
    for (int i = 0; i < command.param_count; ++i)
    {
        if (i > 0)
            log += ',';
        log += command.param(i) < 0 ? "-" : std::to_string(command.param(i));
    }
    if (command.intermediate_count > 0)
        log += '|';
    for (int i = 0; i < command.intermediate_count; ++i)
        log += static_cast<char>(command.intermediates[static_cast<std::size_t>(i)]);
    log += ')';
    log += static_cast<char>(command.final_byte);
}

/**
 * @brief Записывает последовательность ESC.
 * @param command Последовательность.
 */
void esp::vt::test::p::recorder::esc(const esp::vt::command &command)
{
    log += "ESC(";
    for (int i = 0; i < command.intermediate_count; ++i)
        log += static_cast<char>(command.intermediates[static_cast<std::size_t>(i)]);
    log += ')';
    log += static_cast<char>(command.final_byte);
}

/**
 * @brief Дописывает к записи число в шестнадцатеричном виде.
 * @param format Формат @c printf с одним @c %X.
 * @param value Число.
 */
void esp::vt::test::p::recorder::append_hex(const char *format, unsigned value)
{
    char text[16];
    std::snprintf(text, sizeof(text), format, value);
    log += text;
}


// Открытые

void esp::vt::test::parser_tests(esp::test::context &context)
{
    context.set_case("parser: text");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("ab c"), "ab c");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("a\x7fz"), "az");

    context.set_case("parser: UTF-8");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("Ж▶"), "<U+0416><U+25B6>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xF0\x9F\x98\x80"), "<U+1F600>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\x80"), "<U+FFFD>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xD0z"), "<U+FFFD>z");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xC0\xAF"), "<U+FFFD><U+FFFD>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xE0\x80\xAF"), "<U+FFFD>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xED\xA0\x80"), "<U+FFFD>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xF5\x80"), "<U+FFFD><U+FFFD>");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("\xD0\n"), "<U+FFFD><0A>");
    {
        esp::vt::test::p::recorder recorder;
        esp::vt::parser parser{recorder};
        parser.feed("\xE2");
        parser.feed("\x96");
        parser.feed("\xB6");
        ESP_CHECK_EQUAL(context, recorder.log, "<U+25B6>");
    }

    context.set_case("parser: controls");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("a\r\nb\a"), "a<0D><0A>b<07>");

    context.set_case("parser: CSI");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1;2H"), "CSI(1,2)H");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[H"), "CSI()H");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[;5H"), "CSI(-,5)H");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1;H"), "CSI(1,-)H");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[?25l"), "CSI(?25)l");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[>4;2m"), "CSI(>4,2)m");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[2 q"), "CSI(2| )q");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[99999999m"), "CSI(65535)m");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse("^[[1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17m"),
                    "CSI(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)m");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[38:5:1mX"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1?2hX"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1\n2H"), "<0A>CSI(12)H");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1\x18z"), "<18>z");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[[1\xD0\x96"), "<U+0416>");
    {
        esp::vt::test::p::recorder recorder;
        esp::vt::parser parser{recorder};
        parser.feed("\x1b");
        parser.feed("[3");
        parser.feed("1m");
        ESP_CHECK_EQUAL(context, recorder.log, "CSI(31)m");
    }

    context.set_case("parser: ESC");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[7"), "ESC()7");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[(B"), "ESC(()B");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[^[[m"), "CSI()m");

    context.set_case("parser: strings");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[]0;title\aX"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[]0;title^[\\X"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[Pq#0;2;0;0;0^[\\X"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[_Gf=24;AAAA^[\\X"), "X");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[Pq\aX^[\\Y"), "Y");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse("^[]0;a^[[1mX"), "CSI(1)mX");

    context.set_case("parser: reset");
    {
        esp::vt::test::p::recorder recorder;
        esp::vt::parser parser{recorder};
        parser.feed("\x1b[1");
        parser.reset();
        parser.feed("A\xD0");
        parser.reset();
        parser.feed("B");
        ESP_CHECK_EQUAL(context, recorder.log, "AB");
    }
}
