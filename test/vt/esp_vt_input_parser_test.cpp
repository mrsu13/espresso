/**
 * @file esp_vt_input_parser_test.cpp
 * @brief Проверки разборщика ввода esp::vt::input_parser.
 */

#include "esp_vt_tests.h"

#include "esp_ps2_virtual_key.h"
#include "esp_ps2_virtual_key_item.h"
#include "esp_vt_config.h"
#include "esp_vt_controls.h"
#include "esp_vt_input_parser.h"
#include "esp_vt_key_event.h"
#include "esp_vt_keys.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <vector>


namespace esp::vt::test::p
{

/**
 * @brief Разбирает байты и возвращает значения событий.
 * @param parser Разборщик.
 * @param text Байты; @c ^[ — ESC (esp::vt::test::caret()).
 * @param is_timed_out Вызвать ли в конце time_out(), если разборщик ждёт продолжения.
 * @return Значения событий по порядку.
 */
std::vector<std::uint32_t> parse_input(esp::vt::input_parser &parser, std::string_view text,
                                       bool is_timed_out)
{
    std::vector<std::uint32_t> keys;
    for (const char byte : esp::vt::test::caret(text))
    {
        if (const std::optional<esp::vt::key_event> key =
                parser.feed(static_cast<std::uint8_t>(byte)))
            keys.push_back(key->value());
    }
    if (is_timed_out && parser.is_pending())
    {
        if (const std::optional<esp::vt::key_event> key = parser.time_out())
            keys.push_back(key->value());
    }
    return keys;
}

/**
 * @brief Разбирает байты новым разборщиком и возвращает единственное событие.
 * @param text Байты; @c ^[ — ESC.
 * @param is_timed_out Вызвать ли в конце time_out().
 * @return Значение события; 0xFFFFFFFF — событий не одно.
 */
std::uint32_t parse_single(std::string_view text, bool is_timed_out = false)
{
    esp::vt::input_parser parser;
    const std::vector<std::uint32_t> keys =
        esp::vt::test::p::parse_input(parser, text, is_timed_out);
    return keys.size() == 1 ? keys[0] : 0xFFFFFFFF;
}

/**
 * @brief Возвращает значение события особой клавиши.
 * @param key Клавиша.
 * @param modifiers Признаки esp::vt::key_event.
 * @return Значение события.
 */
std::uint32_t special_value(esp::vt::special_key key, std::uint32_t modifiers = 0)
{
    return esp::vt::key_event{key, modifiers}.value();
}

/**
 * @brief Переводит нажатие клавиши PS/2 в байты (esp::vt::key_sequence()) и разбирает их.
 * @param vk Виртуальная клавиша.
 * @param ascii Знак, который дал бы драйвер клавиатуры; 0xFF — нет знака.
 * @param is_shift Нажат ли Shift.
 * @param is_alt Нажат ли Alt.
 * @param is_ctrl Нажат ли Ctrl.
 * @return Значение события; 0xFFFFFFFF — событий не одно.
 */
std::uint32_t parse_ps2_key(esp::ps2::virtual_key vk, int ascii, bool is_shift, bool is_alt,
                            bool is_ctrl)
{
    esp::ps2::virtual_key_item item{};
    item.vk = vk;
    item.down = 1;
    item.ascii = static_cast<std::uint8_t>(ascii);
    item.shift = is_shift ? 1 : 0;
    item.lalt = is_alt ? 1 : 0;
    item.ctrl = is_ctrl ? 1 : 0;
    std::array<char, esp::vt::MAX_KEY_SEQUENCE> bytes{};
    const int length = esp::vt::key_sequence(item, bytes);
    // Alt+знак — это ESC и знак: без time_out() ESC в конце ждал бы продолжения.
    return esp::vt::test::p::parse_single(
        std::string_view{bytes.data(), static_cast<std::size_t>(length)}, true);
}

}


/**
 * @brief Проверяет разборщик ввода.
 * @param context Счёт проверок.
 */
void esp::vt::test::input_parser_tests(esp::test::context &context)
{


    context.set_case("characters");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("a"), 'a');
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("\x01"), 0x01);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("\x7f"), esp::vt::DEL);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("\t"), esp::vt::HT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("ж"), 0x0436);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("€"), 0x20AC);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("\xF0\x9F\x98\x80"), 0x1F600);

    context.set_case("broken UTF-8");
    {
        esp::vt::input_parser parser;
        // Начатый знак обрывает ASCII: знак отбрасывается, буква остаётся.
        const std::vector<std::uint32_t> keys =
            esp::vt::test::p::parse_input(parser, "\xD0z", false);
        ESP_CHECK_EQUAL(context, static_cast<long long>(keys.size()), 1);
        ESP_CHECK(context, !keys.empty() && keys[0] == 'z');
    }
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_single("\xC0\x80"
                                                   "a"),
                    'a');
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_single("\xE0\x80\x80"
                                                   "b"),
                    'b');
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_single("\xED\xA0\x80"
                                                   "c"),
                    'c');
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_single("\x80"
                                                   "d"),
                    'd');
    {
        esp::vt::input_parser parser;
        parser.feed(0xD0);
        ESP_CHECK(context, parser.is_pending());
        ESP_CHECK(context, !parser.time_out());
        ESP_CHECK(context, !parser.is_pending());
    }

    context.set_case("Enter");
    {
        esp::vt::input_parser parser;
        const std::vector<std::uint32_t> keys =
            esp::vt::test::p::parse_input(parser, "\r\n\r\r\nx\n", false);
        const std::vector<std::uint32_t> expected{esp::vt::CR, esp::vt::CR, esp::vt::CR, 'x',
                                                  esp::vt::LF};
        ESP_CHECK(context, keys == expected);
    }
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("\n"), esp::vt::LF);

    context.set_case("Esc and Alt");
    {
        esp::vt::input_parser parser;
        ESP_CHECK(context, !parser.feed(esp::vt::ESC));
        ESP_CHECK(context, parser.is_pending());
        const std::optional<esp::vt::key_event> key = parser.time_out();
        ESP_CHECK(context, key && key->value() == esp::vt::ESC);
        ESP_CHECK(context, !parser.is_pending());
    }
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[b"), 'b' | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[\x7f"),
                    esp::vt::DEL | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[\r"),
                    esp::vt::CR | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[ж"),
                    0x0436 | esp::vt::key_event::ALT);
    {
        // Второй ESC делает первый клавишей Esc и сам ждёт продолжения.
        esp::vt::input_parser parser;
        const std::vector<std::uint32_t> keys = esp::vt::test::p::parse_input(parser, "^[^[", true);
        const std::vector<std::uint32_t> expected{esp::vt::ESC, esp::vt::ESC};
        ESP_CHECK(context, keys == expected);
    }
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[", true),
                    '[' | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[O", true),
                    'O' | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[1;", true), 0xFFFFFFFF);

    context.set_case("cursor keys");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[A"),
                    esp::vt::test::p::special_value(esp::vt::special_key::up));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[B"),
                    esp::vt::test::p::special_value(esp::vt::special_key::down));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[C"),
                    esp::vt::test::p::special_value(esp::vt::special_key::right));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[D"),
                    esp::vt::test::p::special_value(esp::vt::special_key::left));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[OA"),
                    esp::vt::test::p::special_value(esp::vt::special_key::up));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[1;5D"),
        esp::vt::test::p::special_value(esp::vt::special_key::left, esp::vt::key_event::CTRL));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[1;2C"),
        esp::vt::test::p::special_value(esp::vt::special_key::right, esp::vt::key_event::SHIFT));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[1;3A"),
        esp::vt::test::p::special_value(esp::vt::special_key::up, esp::vt::key_event::ALT));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[1;8B"),
                    esp::vt::test::p::special_value(esp::vt::special_key::down,
                                                    esp::vt::key_event::SHIFT
                                                        | esp::vt::key_event::ALT
                                                        | esp::vt::key_event::CTRL));

    context.set_case("Home and End variants");
    for (const std::string_view text : {"^[[H", "^[[1~", "^[[7~", "^[OH"})
        ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single(text),
                        esp::vt::test::p::special_value(esp::vt::special_key::home));
    for (const std::string_view text : {"^[[F", "^[[4~", "^[[8~", "^[OF"})
        ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single(text),
                        esp::vt::test::p::special_value(esp::vt::special_key::end));

    context.set_case("numbered keys");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[2~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::insert));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[3~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::del));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[3;5~"),
        esp::vt::test::p::special_value(esp::vt::special_key::del, esp::vt::key_event::CTRL));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[5~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::page_up));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[6~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::page_down));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[15~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f5));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[15;2~"),
        esp::vt::test::p::special_value(esp::vt::special_key::f5, esp::vt::key_event::SHIFT));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[21~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f10));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[24~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f12));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[11~"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f1));

    context.set_case("function keys F1-F4");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[OP"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f1));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[OS"),
                    esp::vt::test::p::special_value(esp::vt::special_key::f4));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[[1;2P"),
        esp::vt::test::p::special_value(esp::vt::special_key::f1, esp::vt::key_event::SHIFT));
    ESP_CHECK_EQUAL(
        context, esp::vt::test::p::parse_single("^[O2Q"),
        esp::vt::test::p::special_value(esp::vt::special_key::f2, esp::vt::key_event::SHIFT));

    context.set_case("Shift+Tab");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[Z"),
                    esp::vt::HT | esp::vt::key_event::SHIFT);

    context.set_case("unknown sequences");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[99~x"), 'x');
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[?1;2cx"), 'x');
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[<0;10;20Mx"), 'x');
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[ qx"), 'x');
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[OXx"), 'x');

    context.set_case("broken sequences");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[1\r"), esp::vt::CR);
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[^[[A"),
                    esp::vt::test::p::special_value(esp::vt::special_key::up));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::parse_single("^[[99999999999~x"), 'x');
    {
        esp::vt::input_parser parser;
        parser.feed(esp::vt::ESC);
        parser.feed('[');
        parser.reset();
        ESP_CHECK(context, !parser.is_pending());
        const std::optional<esp::vt::key_event> key = parser.feed('A');
        ESP_CHECK(context, key && key->value() == 'A');
    }

    context.set_case("PS/2 keys round trip");
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_left, 0xFF, false, false, true),
        esp::vt::test::p::special_value(esp::vt::special_key::left, esp::vt::key_event::CTRL));
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_f5, 0xFF, true, false, false),
        esp::vt::test::p::special_value(esp::vt::special_key::f5, esp::vt::key_event::SHIFT));
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_f1, 0xFF, false, false, false),
        esp::vt::test::p::special_value(esp::vt::special_key::f1));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_delete, 0xFF, false,
                                                    false, false),
                    esp::vt::test::p::special_value(esp::vt::special_key::del));
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_pageup, 0xFF, false, true, false),
        esp::vt::test::p::special_value(esp::vt::special_key::page_up, esp::vt::key_event::ALT));
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_x, 'x', false, true, false),
        'x' | esp::vt::key_event::ALT);
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_tab, '\t', true, false, false),
        esp::vt::HT | esp::vt::key_event::SHIFT);
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_backspace, 0x08,
                                                    false, false, false),
                    esp::vt::DEL);
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_backspace, 0x08,
                                                    false, false, true),
                    esp::vt::BS);
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::parse_ps2_key(esp::ps2::virtual_key::vk_escape, 0x1B, false,
                                                    false, false),
                    esp::vt::ESC);
}
