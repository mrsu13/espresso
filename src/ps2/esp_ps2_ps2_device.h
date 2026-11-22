// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_ps2_ps2_controller.h"
#include "esp_tools.h"


/**
 * @file
 *
 * @brief This file contains esp::ps2::PS2Device definition.
 */


namespace esp::ps2
{


/** \ingroup Enumerations
 * @brief Represents the type of device attached to PS/2 port.
 */
enum class ps2_device_type
{
    unknown_ps2_device, /**< Unknown device or unable to connect to the device */
    old_at_keyboard, /**< Old AT keyboard */
    mouse_standard, /**< Standard mouse */
    mouse_with_scroll_wheel, /**< Mouse with Scroll Wheel */
    mouse_5_buttons, /**< Mouse with 5 buttons */
    mf2_keyboard_with_translation, /**< Standard MF2 keyboard with translation */
    m2_keyboard, /**< Standard MF2 keyboard. This is the most common value returned by USB/PS2 modern keyboards */
};


/**
 * @brief Base class for PS2 devices (like mouse or keyboard).
 *
 * PS2Device connects to one port of the PS2 Controller class (esp::ps2::PS2Controller).<br>
 * The PS2 controller uses ULP coprocessor and RTC slow memory to communicate with the PS2 device.<br>
 */
class ps2_device
{

public:

    /**
   * @brief Identifies the device attached to the PS2 port.
   *
   * @return The identification ID sent by keyboard.
   */
    ps2_device_type identify()
    {
        ps2_device_type result;
        send_cmd_identify(&result);
        return result;
    };

    /**
   * @brief Gets exclusive access to the device.
   *
   * @param timeOutMS Timeout in milliseconds to wait before fail.
   *
   * @return True if the device has been locked.
   */
    bool lock(int time_out_ms);

    /**
   * @brief Releases device from exclusive access.
   */
    void unlock();

    bool parity_error();

    bool sync_error();

    bool clk_time_out_error();

    /**
   * @brief Sends a raw command to the PS/2 device and wait for reply
   *
   * @param cmd The command byte
   * @param expectedReply Expected reply from PS/2 device
   *
   * @return True on success
   */
    bool send_command(uint8_t cmd, uint8_t expected_reply);

    /**
   * @brief Sends a raw command to the PS/2 device
   *
   * @param cmd The command byte
   */
    void send_command(uint8_t cmd);

    /**
   * @brief Suspends PS/2 port driving the CLK line Low
   *
   * Use resumePort() to release CLK line.
   */
    void suspend_port();

    /**
   * @brief Resumes PS/2 port releasing CLK line
   *
   * Use suspendPort() to suspend.
   */
    void resume_port();

    uint16_t device_id()
    {
        return _device_id;
    }


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
    bool send_cmd_get_scancode_set(uint8_t *result);
    bool send_cmd_set_scancode_set(uint8_t scancode_set);
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

    int16_t _ps2_port;
    int16_t _cmd_time_out;
    int16_t _cmd_sub_time_out;
    uint16_t _device_id; // read by send_cmdIdentify()
};


struct ps2_device_lock
{
    ps2_device_lock(esp::ps2::ps2_device *ps2_device)
        : ps2_device(ps2_device)
    {
        ps2_device->lock(-1);
    }
    ~ps2_device_lock()
    {
        ps2_device->unlock();
    }

    esp::ps2::ps2_device *ps2_device;
};


} // end of namespace
