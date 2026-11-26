#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>


namespace esp::vt
{

class utf8
{

public:

    /** @brief Наибольшее число байтов одного знака в UTF-8. */
    static constexpr int MAX_BYTES = 4;

    utf8() = delete;

    static int encode(std::uint32_t code_point, std::array<char, MAX_BYTES> &bytes);
    static std::size_t next(std::string_view text, std::size_t index);
    static std::size_t previous(std::string_view text, std::size_t index);
    static int length(std::string_view text);
    static bool is_printable(std::uint32_t code_point);
    static bool is_printable_line(std::string_view text);
};

}
