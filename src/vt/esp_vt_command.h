#pragma once

#include "esp_vt_config.h"

#include <array>
#include <cstdint>


namespace esp::vt
{

struct command
{
    void clear();

    int param(int index) const;
    int param_or_0(int index) const;
    int param_or_1(int index) const;

    /**
     * @brief Числовые параметры последовательности по порядку.
     *
     * Пропущенный параметр — например, второй в @c CSI 1;H — хранится как
     * @c -1, чтобы его можно было отличить от явного нуля. Действительны
     * первые @c param_count элементов; читать их удобнее методами param(),
     * param_or_0() и param_or_1().
     */
    std::array<int, esp::vt::MAX_PARAMS> params{};

    /** @brief Сколько элементов @c params заполнено, 0 — @c esp::vt::MAX_PARAMS. */
    int param_count = 0;

    /**
     * @brief Промежуточные байты 0x20–0x2F между параметрами и последним байтом.
     *
     * У последовательностей, которые понимает экран, промежуточных байтов
     * нет; последовательность с ними экран пропускает. Действительны первые
     * @c intermediate_count элементов.
     */
    std::array<std::uint8_t, esp::vt::MAX_INTERMEDIATES> intermediates{};

    /** @brief Сколько элементов @c intermediates заполнено. */
    int intermediate_count = 0;

    /**
     * @brief Знак частной последовательности: @c '?', @c '<', @c '=' или @c '>'; 0 — его нет.
     *
     * Стоит сразу после @c CSI: @c CSI ?25h — частный режим DEC с номером 25.
     */
    std::uint8_t private_marker = 0;

    /**
     * @brief Последний байт, завершивший последовательность.
     *
     * У CSI — 0x40–0x7E, у ESC — 0x30–0x7E; по нему исполнитель узнаёт, что
     * делать: @c 'H' — поставить курсор, @c 'm' — сменить вид текста.
     */
    std::uint8_t final_byte = 0;
};

}
