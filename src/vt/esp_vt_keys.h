#pragma once

#include "esp_ps2_virtual_key_item.h"

#include <array>


namespace esp::vt
{

/**
 * @brief Наибольшая длина последовательности одной клавиши, байты.
 *
 * Самая длинная — особая клавиша с признаками, например Ctrl+Shift+F12:
 * @c ESC @c [24;6~, 7 байтов.
 */
constexpr int MAX_KEY_SEQUENCE = 8;

int key_sequence(const esp::ps2::virtual_key_item &item,
                 std::array<char, esp::vt::MAX_KEY_SEQUENCE> &bytes);

}
