/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "freertos/FreeRTOS.h"

#include "esp_ps2_ps2_device.h"
#include "esp_ps2_utils.h"
#include "esp_tools.h"


#pragma GCC optimize("O2")


namespace esp::ps2
{


#define PS2_CMD_SETLEDS 0xED
#define PS2_CMD_ECHO 0xEE
#define PS2_CMD_GETSET_CURRENT_SCANCODE_SET 0xF0 // keyboard specific
#define PS2_CMD_SET_REMOTE_MODE 0xF0 // mouse specific
#define PS2_CMD_IDENTIFY 0xF2
#define PS2_CMD_SET_TYPEMATIC_RATE_AND_DELAY 0xF3 // keyboard specific
#define PS2_CMD_SET_SAMPLE_RATE 0xF3 // mouse specific
#define PS2_CMD_ENABLE_SCANNING 0xF4
#define PS2_CMD_DISABLE_SCANNING 0xF5
#define PS2_CMD_SET_DEFAULT_PARAMS 0xF6
#define PS2_CMD_RESEND_LAST_BYTE 0xFE
#define PS2_CMD_RESET 0xFF
#define PS2_CMD_SET_STREAM_MODE 0xEA // mouse specific
#define PS2_CMD_STATUS_REQUEST 0xE9 // mouse specific
#define PS2_CMD_SET_RESOLUTION 0xE8 // mouse specific
#define PS2_CMD_SET_SCALING 0xE6 // mouse specific

#define PS2_REPLY_ERROR1 0x00
#define PS2_REPLY_ERROR2 0xFF
#define PS2_REPLY_SELFTEST_OK 0xAA
#define PS2_REPLY_ECHO 0xEE
#define PS2_REPLY_ACK 0xFA
#define PS2_REPLY_SELFTEST_FAILED1 0xFC
#define PS2_REPLY_SELFTEST_FAILED2 0xFD
#define PS2_REPLY_RESEND 0xFE

#define PS2_DEFAULT_CMD_TIMEOUT 500
#define PS2_DEFAULT_CMD_SUBTIMEOUT (PS2_DEFAULT_CMD_TIMEOUT / 2)

#define PS2_QUICK_CMD_TIMEOUT 50
#define PS2_QUICK_CMD_SUBTIMEOUT (PS2_QUICK_CMD_TIMEOUT / 2)


ps2_device::ps2_device()
{
    _cmd_time_out = PS2_DEFAULT_CMD_TIMEOUT;
    _cmd_sub_time_out = PS2_DEFAULT_CMD_SUBTIMEOUT;
}


ps2_device::~ps2_device() {}


void ps2_device::quick_check_hardware()
{
    _cmd_time_out = PS2_QUICK_CMD_TIMEOUT;
    _cmd_sub_time_out = PS2_QUICK_CMD_SUBTIMEOUT;
}


bool ps2_device::lock(int time_out_ms)
{
    return ps2_controller::lock(_ps2_port, time_out_ms);
}


void ps2_device::unlock()
{
    ps2_controller::unlock(_ps2_port);
}


void ps2_device::begin(int ps2_port)
{
    _ps2_port = ps2_port;
}


int ps2_device::data_available()
{
    return ps2_controller::data_available(_ps2_port);
}


bool ps2_device::parity_error()
{
    return ps2_controller::parity_error(_ps2_port);
}


bool ps2_device::sync_error()
{
    return ps2_controller::sync_error(_ps2_port);
}


bool ps2_device::clk_time_out_error()
{
    return ps2_controller::clk_time_out_error(_ps2_port);
}


void ps2_device::suspend_port()
{
    ps2_controller::disable_rx(_ps2_port);
}


void ps2_device::resume_port()
{
    ps2_controller::enable_rx(_ps2_port);
}


int ps2_device::get_data(int time_out_ms)
{
    constexpr int INTER_GETDATA_TIMEOUT_MS = 100;
    constexpr int INTER_GETDATA_PAUSE_MS = 10;

    int inter_time_out = time_out_ms > -1 ? esp::imin(time_out_ms, INTER_GETDATA_TIMEOUT_MS)
                                          : INTER_GETDATA_TIMEOUT_MS;

    int ret = -1;
    time_out timeout;
    while (true)
    {
        lock(-1);
        ret = ps2_controller::get_data(_ps2_port, inter_time_out);
        unlock();
        if (ret > -1 || parity_error() || sync_error() || clk_time_out_error()
            || timeout.expired(time_out_ms))
            break;
        // give the opportunity for other sends
        vTaskDelay(INTER_GETDATA_PAUSE_MS / portTICK_PERIOD_MS);
    }
    return ret;
}


bool ps2_device::send_command(uint8_t cmd, uint8_t expected_reply)
{
    constexpr int INTER_WAITREPLY_TIMEOUT_MS = 10;

    ps2_device_lock device_lock(this);

    // temporary disable RX for the other port
    ps2_port_auto_disable_rx auto_disable_rx(!_ps2_port);

    ps2_controller::send_data(cmd, _ps2_port);
    time_out timeout;
    do
    {
        if (ps2_controller::get_data(_ps2_port, INTER_WAITREPLY_TIMEOUT_MS) == expected_reply)
            return true;
    } while (!timeout.expired(_cmd_time_out));
    return false;
}


void ps2_device::send_command(uint8_t cmd)
{
    ps2_controller::send_data(cmd, _ps2_port);
}


void ps2_device::request_to_resend_last_byte()
{
    ps2_controller::send_data(PS2_CMD_RESEND_LAST_BYTE, _ps2_port);
}


bool ps2_device::send_cmd_leds(bool num_lock, bool caps_lock, bool scroll_lock)
{
    ps2_device_lock device_lock(this);
    return send_command(PS2_CMD_SETLEDS, PS2_REPLY_ACK)
        && send_command((scroll_lock << 0) | (num_lock << 1) | (caps_lock << 2), PS2_REPLY_ACK);
}


bool ps2_device::send_cmd_echo()
{
    return send_command(PS2_CMD_ECHO, PS2_REPLY_ECHO);
}


bool ps2_device::send_cmd_get_scancode_set(uint8_t *result)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_GETSET_CURRENT_SCANCODE_SET, PS2_REPLY_ACK))
        return false;
    if (!send_command(0, PS2_REPLY_ACK))
        return false;
    *result = get_data(_cmd_time_out);
    return (*result >= 1 || *result <= 3);
}


bool ps2_device::send_cmd_set_scancode_set(uint8_t scancode_set)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_GETSET_CURRENT_SCANCODE_SET, PS2_REPLY_ACK))
        return false;
    return send_command(scancode_set, PS2_REPLY_ACK);
}


// return value is always valid
bool ps2_device::send_cmd_identify(ps2_device_type *result)
{
    ps2_device_lock device_lock(this);
    *result = ps2_device_type::unknown_ps2_device;
    if (!send_cmd_disable_scanning())
        return false;
    if (!send_command(PS2_CMD_IDENTIFY, PS2_REPLY_ACK))
        return false;
    int b1 = get_data(_cmd_time_out);
    int b2 = get_data(_cmd_time_out);
    _device_id = (uint8_t)b1 | ((uint8_t)b2 << 8);
    if (b1 == -1 && b2 == -1)
        *result = ps2_device_type::old_at_keyboard;
    else if (b1 == 0x00 && b2 == -1)
        *result = ps2_device_type::mouse_standard;
    else if (b1 == 0x03 && b2 == -1)
        *result = ps2_device_type::mouse_with_scroll_wheel;
    else if (b1 == 0x04 && b2 == -1)
        *result = ps2_device_type::mouse_5_buttons;
    else if ((b1 == 0xAB && b2 == 0x41) || (b1 == 0xAB && b2 == 0xC1))
        *result = ps2_device_type::mf2_keyboard_with_translation;
    else if (b1 == 0xAB && b2 == 0x83)
        *result = ps2_device_type::m2_keyboard;
    return send_cmd_enable_scanning();
}


bool ps2_device::send_cmd_disable_scanning()
{
    return send_command(PS2_CMD_DISABLE_SCANNING, PS2_REPLY_ACK);
}


bool ps2_device::send_cmd_enable_scanning()
{
    return send_command(PS2_CMD_ENABLE_SCANNING, PS2_REPLY_ACK);
}


const int16_t REPEATRATES[32] = {33,  37,  41,  45,  50,  54,  58,  62,  66,  75,  83,
                                 91,  100, 108, 125, 125, 133, 149, 166, 181, 200, 217,
                                 232, 250, 270, 303, 333, 370, 400, 434, 476, 500};


// repeatRateMS  :  33 ms ...  500 ms (in steps as above REPEATRATES table)
// repeatDelayMS : 250 ms ... 1000 ms (in steps of 250 ms)
bool ps2_device::send_cmd_typematic_rate_and_delay(int repeat_rate_ms, int repeat_delay_ms)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_SET_TYPEMATIC_RATE_AND_DELAY, PS2_REPLY_ACK))
        return false;
    uint8_t byte_to_send = 0b01011; // default repeat rate 10.9 characters per seconds (91ms)
    for (int i = 0; i < 32; ++i)
        if (REPEATRATES[i] >= repeat_rate_ms)
        {
            byte_to_send = i;
            break;
        }
    byte_to_send |= (repeat_delay_ms / 250 - 1) << 5;
    return send_command(byte_to_send, PS2_REPLY_ACK);
}


// sampleRate: valid values are 10, 20, 40, 60, 80, 100, and 200 (samples/sec)
bool ps2_device::send_cmd_set_sample_rate(int sample_rate)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_SET_SAMPLE_RATE, PS2_REPLY_ACK))
        return false;
    return send_command(sample_rate, PS2_REPLY_ACK);
}


// resolution:
//   0 = 1 count/mm
//   1 = 2 count/mm
//   2 = 4 count/mm
//   3 = 8 count/mm
bool ps2_device::send_cmd_set_resolution(int resolution)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_SET_RESOLUTION, PS2_REPLY_ACK))
        return false;
    return send_command(resolution, PS2_REPLY_ACK);
}


// scaling:
//   1 -> 1:1
//   2 -> 1:2
bool ps2_device::send_cmd_set_scaling(int scaling)
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_SET_SCALING, PS2_REPLY_ACK))
        return false;
    return send_command(scaling, PS2_REPLY_ACK);
}


bool ps2_device::send_cmd_set_default_params()
{
    return send_command(PS2_CMD_SET_DEFAULT_PARAMS, PS2_REPLY_ACK);
}


bool ps2_device::send_cmd_reset()
{
    ps2_device_lock device_lock(this);
    if (!send_command(PS2_CMD_RESET, PS2_REPLY_ACK))
        return false;
    return get_data(500)
        == PS2_REPLY_SELFTEST_OK; // timout 500ms should be enough for PS2 device to reset and do self test
}


} // end of namespace
