// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_non_copyable.h"
#include "esp_ps2_ps2_device_type.h"

#include <cstdint>


namespace esp::ps2
{

class ps2_device : esp::non_copyable
{

public:

    inline ps2_device_type identify();

    bool lock(int time_out_ms);
    void unlock();

    bool parity_error();
    bool sync_error();
    bool clk_time_out_error();

    bool send_command(std::uint8_t cmd, std::uint8_t expected_reply);
    void send_command(std::uint8_t cmd);

    void suspend_port();
    void resume_port();

    inline std::uint16_t device_id() const;

protected:

    ps2_device();
    ~ps2_device();

    void quick_check_hardware();

    void begin(int ps2_port);

    int data_available();
    int get_data(int time_out_ms);

    void request_to_resend_last_byte();

    bool send_cmd_leds(bool num_lock, bool caps_lock, bool scroll_lock);
    bool send_cmd_echo();
    bool send_cmd_get_scancode_set(std::uint8_t *result);
    bool send_cmd_set_scancode_set(std::uint8_t scancode_set);
    bool send_cmd_identify(ps2_device_type *result);
    bool send_cmd_disable_scanning();
    bool send_cmd_enable_scanning();
    bool send_cmd_typematic_rate_and_delay(int repeat_rate_ms, int repeat_delay_ms);
    bool send_cmd_set_sample_rate(int sample_rate);
    bool send_cmd_set_default_params();
    bool send_cmd_reset();
    bool send_cmd_set_resolution(int resolution);
    bool send_cmd_set_scaling(int scaling);

private:

    /** @brief Порт PS/2 устройства: 0 или 1; задаёт begin(). */
    std::int16_t _ps2_port;

    /**
     * @brief Время ожидания ответа на команду, мс.
     *
     * По умолчанию 500 мс; quick_check_hardware() уменьшает его до 50 мс,
     * чтобы быстро понять, что устройства нет.
     */
    std::int16_t _cmd_time_out;

    /** @brief Половина @c _cmd_time_out, мс; сейчас не используется. */
    std::int16_t _cmd_sub_time_out;

    /** @brief Код устройства из ответа на команду опознания (send_cmd_identify()). */
    std::uint16_t _device_id;
};

}


// Реализация

// Открытые

/**
 * @brief Опознаёт устройство на порту.
 *
 * На время опознания сканирование выключается, затем снова включается.
 *
 * @return Тип устройства; @c ps2_device_type::unknown_ps2_device, если оно
 *         не ответило.
 */
esp::ps2::ps2_device_type esp::ps2::ps2_device::identify()
{
    ps2_device_type result;
    send_cmd_identify(&result);
    return result;
}

/**
 * @brief Возвращает код устройства, полученный при опознании.
 * @return Два байта ответа на команду опознания: первый — в младшем байте.
 */
std::uint16_t esp::ps2::ps2_device::device_id() const
{
    return _device_id;
}
