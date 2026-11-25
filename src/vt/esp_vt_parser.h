#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_command.h"
#include "esp_vt_interpreter.h"

#include <cstdint>
#include <string_view>


namespace esp::vt
{

class parser : esp::non_copyable
{

public:

    explicit parser(esp::vt::interpreter &interpreter);

    void feed(std::string_view bytes);
    void reset();

private:

    /**
     * @brief Состояние автомата разбора.
     *
     * Состояния — узлы автомата Пола Уильямса для VT500 (vt100.net), только
     * строки DCS, SOS, PM и APC сведены в одно состояние: разборщик их
     * пропускает, не разбирая.
     */
    enum class state
    {
        /** @brief Обычный текст: знаки UTF-8 и управляющие байты. */
        ground,

        /** @brief Пришёл ESC: ждём промежуточный или последний байт либо начало CSI или строки. */
        escape,

        /** @brief После ESC пришёл промежуточный байт 0x20–0x2F: ждём ещё такие или последний байт. */
        escape_intermediate,

        /** @brief Пришло @c ESC @c [: ждём знак частной последовательности, параметр или последний байт. */
        csi_entry,

        /** @brief Идут параметры CSI: цифры и @c ';'. */
        csi_param,

        /** @brief После параметров CSI пришёл промежуточный байт: ждём последний байт. */
        csi_intermediate,

        /** @brief Испорченная последовательность CSI: байты пропускаются до последнего байта. */
        csi_ignore,

        /** @brief Строка OSC (@c ESC @c ]): пропускается до BEL или @c ESC @c \\. */
        osc_string,

        /** @brief Строка DCS, SOS, PM или APC: пропускается до @c ESC @c \\. */
        ignored_string,

        /** @brief Внутри строки пришёл ESC: @c \\ завершает строку, другой байт начинает новую последовательность. */
        string_escape,
    };

    void step(std::uint8_t byte);
    void step_ground(std::uint8_t byte);
    void step_escape(std::uint8_t byte);
    void step_csi(std::uint8_t byte);
    void step_string(std::uint8_t byte);
    void step_utf8(std::uint8_t byte);
    void flush_utf8();

    void start_sequence(esp::vt::parser::state state);
    void collect_intermediate(std::uint8_t byte);
    void collect_digit(std::uint8_t byte);
    void commit_param();
    void dispatch_csi(std::uint8_t final_byte);
    void dispatch_esc(std::uint8_t final_byte);

    /** @brief Исполнитель разобранного; живёт дольше разборщика. */
    esp::vt::interpreter &_interpreter;

    /** @brief Текущее состояние автомата; сохраняется между вызовами feed(). */
    esp::vt::parser::state _state;

    /**
     * @brief Собираемая последовательность CSI или ESC.
     *
     * Очищается в начале каждой последовательности и передаётся исполнителю,
     * когда приходит последний байт.
     */
    esp::vt::command _command;

    /**
     * @brief Собираемый числовой параметр; @c -1, пока цифр не было.
     *
     * Значение растёт с каждой цифрой и не превышает
     * @c esp::vt::MAX_PARAM_VALUE.
     */
    int _param;

    /**
     * @brief Есть ли незаписанный параметр, возможно пустой.
     *
     * Становится @c true с первой цифрой или после @c ';'. Последний байт
     * тогда записывает параметр в @c _command, так что у @c CSI 1;H два
     * параметра — 1 и пропущенный.
     */
    bool _is_param_open;

    /** @brief Собираемый код знака UTF-8: значащие разряды уже пришедших байтов. */
    std::uint32_t _code_point;

    /**
     * @brief Наименьший код, допустимый для собираемой длины UTF-8.
     *
     * Код меньше — избыточная запись, например U+002F двумя байтами; вместо
     * неё выводится знак замены.
     */
    std::uint32_t _min_code_point;

    /** @brief Сколько продолжающих байтов UTF-8 ещё ждать; 0 — знак не начат. */
    int _utf8_remaining;
};

}
