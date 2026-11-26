#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_key_event.h"

#include <array>
#include <cstdint>
#include <optional>


namespace esp::vt
{

class input_parser : esp::non_copyable
{

public:

    input_parser();

    std::optional<esp::vt::key_event> feed(std::uint8_t byte);
    std::optional<esp::vt::key_event> time_out();
    inline bool is_pending() const;
    void reset();

private:

    /** @brief Состояние разбора. */
    enum class state
    {
        /** @brief Знаки UTF-8 и управляющие знаки. */
        ground,

        /** @brief Пришёл ESC: дальше — последовательность, знак с Alt или ничего (клавиша Esc). */
        escape,

        /** @brief Пришло <tt>ESC [</tt>: параметры и последний байт последовательности CSI. */
        csi,

        /** @brief Пришло <tt>ESC O</tt>: признаки и последний байт последовательности SS3. */
        ss3,
    };

    std::optional<esp::vt::key_event> step_ground(std::uint8_t byte);
    std::optional<esp::vt::key_event> step_escape(std::uint8_t byte);
    std::optional<esp::vt::key_event> step_csi(std::uint8_t byte);
    std::optional<esp::vt::key_event> step_ss3(std::uint8_t byte);
    std::optional<esp::vt::key_event> csi_key(std::uint8_t final_byte) const;
    std::uint32_t modifiers() const;
    std::optional<esp::vt::key_event> emit(esp::vt::key_event key);
    void start_ground();

    /** @brief Текущее состояние; сохраняется между вызовами feed(). */
    esp::vt::input_parser::state _state;

    /** @brief Признаки собираемого знака: @c esp::vt::key_event::ALT, если перед ним пришёл ESC. */
    std::uint32_t _modifiers;

    /** @brief Собираемый код знака UTF-8: значащие разряды уже пришедших байтов. */
    std::uint32_t _code_point;

    /** @brief Наименьший код для собираемой длины UTF-8; меньший код — избыточная запись. */
    std::uint32_t _min_code_point;

    /** @brief Сколько продолжающих байтов UTF-8 ещё ждать; 0 — знак не начат. */
    int _utf8_remaining;

    /**
     * @brief Первые два числовых параметра последовательности; @c -1 — параметр пропущен.
     *
     * Первый — номер клавиши в <tt>ESC [номер~</tt>, второй — 1 плюс признаки
     * (Shift — 1, Alt — 2, Ctrl — 4). У SS3 признаки — во втором.
     */
    std::array<int, 2> _params;

    /** @brief Номер собираемого параметра: сколько @c ';' уже пришло. */
    int _param_index;

    /**
     * @brief Пропустить ли последовательность.
     *
     * В ней знак частной последовательности (<tt>< = > ?</tt>), @c ':' или
     * промежуточный байт: такие шлют мышь и ответы терминала, но не клавиши.
     */
    bool _is_ignored;

    /** @brief Был ли последний знак CR: LF сразу после него отдельной клавишей не считается. */
    bool _is_after_cr;
};

}


// Реализация

// Открытые

/**
 * @brief Сообщает, ждёт ли разборщик продолжения.
 *
 * Продолжения ждут после ESC, внутри последовательности и внутри знака
 * UTF-8. Тот, кто читает ввод, ждёт тогда новый байт не дольше
 * @c esp::vt::ESCAPE_TIME_OUT_MS и вызывает time_out(), если не дождался.
 *
 * @return @c true, если начатое событие ещё не готово.
 */
bool esp::vt::input_parser::is_pending() const
{
    return _state != esp::vt::input_parser::state::ground || _utf8_remaining > 0;
}
