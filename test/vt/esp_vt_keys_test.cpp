/**
 * @file esp_vt_keys_test.cpp
 * @brief Проверки перевода клавиш PS/2 в байты VT220, esp::vt::key_sequence().
 */

#include "esp_vt_tests.h"

#include "esp_ps2_virtual_key.h"
#include "esp_ps2_virtual_key_item.h"
#include "esp_vt_keys.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>


namespace esp::vt::test::p
{

/** @brief Признаки клавиатуры, нажатые вместе с клавишей. */
enum class with
{
    /** @brief Без признаков. */
    nothing,

    /** @brief Shift. */
    shift,

    /** @brief Alt. */
    alt,

    /** @brief Ctrl. */
    ctrl,

    /** @brief Ctrl и Shift. */
    ctrl_shift,
};

/**
 * @brief Возвращает байты, которые даёт нажатие клавиши.
 * @param vk Виртуальная клавиша.
 * @param ascii Знак, который драйвер клавиатуры дал бы этой клавише; 0xFF — нет знака.
 * @param modifiers Нажатые вместе с ней признаки.
 * @param is_down @c true — нажатие, @c false — отпускание.
 * @return Байты последовательности.
 */
std::string sequence(esp::ps2::virtual_key vk, int ascii,
                     esp::vt::test::p::with modifiers = esp::vt::test::p::with::nothing,
                     bool is_down = true)
{
    esp::ps2::virtual_key_item item{};
    item.vk = vk;
    item.down = is_down ? 1 : 0;
    item.ascii = static_cast<std::uint8_t>(ascii);
    item.shift = modifiers == esp::vt::test::p::with::shift
        || modifiers == esp::vt::test::p::with::ctrl_shift;
    item.lalt = modifiers == esp::vt::test::p::with::alt;
    item.ctrl = modifiers == esp::vt::test::p::with::ctrl
        || modifiers == esp::vt::test::p::with::ctrl_shift;

    std::array<char, esp::vt::MAX_KEY_SEQUENCE> bytes{};
    const int length = esp::vt::key_sequence(item, bytes);
    return std::string(bytes.data(), static_cast<std::size_t>(length));
}

}


// Открытые

void esp::vt::test::keys_tests(esp::test::context &context)
{
    constexpr int NO_ASCII = 0xFF;

    context.set_case("keys: characters");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_a, 'a'), "a");
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_a, 'a', esp::vt::test::p::with::alt),
        esp::vt::test::caret("^[a"));
    ESP_CHECK_EQUAL(
        context,
        esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_a, 0x01, esp::vt::test::p::with::ctrl),
        "\x01");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_return, '\r'),
                    "\r");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_escape, 0x1B),
                    esp::vt::test::caret("^["));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_space, 0x00,
                                               esp::vt::test::p::with::ctrl),
                    std::string(1, '\0'));

    context.set_case("keys: backspace and tab");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_backspace, 0x08),
                    "\x7f");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_backspace, 0x7F,
                                               esp::vt::test::p::with::ctrl),
                    "\b");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_tab, '\t'), "\t");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_tab, NO_ASCII,
                                               esp::vt::test::p::with::shift),
                    esp::vt::test::caret("^[[Z"));

    context.set_case("keys: special keys");
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_up, NO_ASCII),
                    esp::vt::test::caret("^[[A"));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_kp_end, NO_ASCII),
                    esp::vt::test::caret("^[[F"));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_delete, 0x7F),
                    esp::vt::test::caret("^[[3~"));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_f1, NO_ASCII),
                    esp::vt::test::caret("^[OP"));
    ESP_CHECK_EQUAL(context, esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_f12, NO_ASCII),
                    esp::vt::test::caret("^[[24~"));

    context.set_case("keys: special keys with modifiers");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_left, NO_ASCII,
                                               esp::vt::test::p::with::ctrl),
                    esp::vt::test::caret("^[[1;5D"));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_f1, NO_ASCII,
                                               esp::vt::test::p::with::shift),
                    esp::vt::test::caret("^[[1;2P"));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_f5, NO_ASCII,
                                               esp::vt::test::p::with::shift),
                    esp::vt::test::caret("^[[15;2~"));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_pageup, NO_ASCII,
                                               esp::vt::test::p::with::alt),
                    esp::vt::test::caret("^[[5;3~"));
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_f12, NO_ASCII,
                                               esp::vt::test::p::with::ctrl_shift),
                    esp::vt::test::caret("^[[24;6~"));

    context.set_case("keys: nothing to send");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_a, 'a',
                                               esp::vt::test::p::with::nothing, false),
                    "");
    ESP_CHECK_EQUAL(context,
                    esp::vt::test::p::sequence(esp::ps2::virtual_key::vk_lshift, NO_ASCII,
                                               esp::vt::test::p::with::shift),
                    "");
}
