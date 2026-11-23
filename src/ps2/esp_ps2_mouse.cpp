// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include "freertos/FreeRTOS.h"

#include "esp_gfx_bitmapped_display_controller.h"
#include "esp_ps2_config.h"
#include "esp_ps2_mouse.h"
#include "esp_ps2_ps2_controller.h"


#pragma GCC optimize("O2")


namespace esp::ps2
{


bool mouse::_quick_check_hardware = false;


mouse::mouse()
    : _mouse_available(false)
    , _mouse_type(legacy_mouse)
    , _mouse_update_task(nullptr)
    , _received_packet(nullptr)
    , _absolute_update(false)
    , _prev_delta_time(0)
    , _movement_acceleration(180)
    , _wheel_acceleration(60000)
    , _absolute_queue(nullptr)
    , _update_display_controller(nullptr)
{
}


mouse::~mouse()
{
    ps2_device_lock lock(this);
    terminate_absolute_positioner();
    if (_mouse_update_task)
        vTaskDelete(_mouse_update_task);
    if (_received_packet)
        vQueueDelete(_received_packet);
}


void mouse::begin(int ps2_port)
{
    if (_quick_check_hardware)
        ps2_device::quick_check_hardware();
    ps2_device::begin(ps2_port);
    reset();
    _received_packet = xQueueCreate(1, sizeof(mouse_packet));
    xTaskCreate(&mouse_update_task, "", 1600, this, 5, &_mouse_update_task);
    _area = esp::gfx::size(0, 0);
}


void mouse::begin(gpio_num_t clk_gpio, gpio_num_t data_gpio)
{
    ps2_controller::begin(clk_gpio, data_gpio);
    ps2_controller::set_mouse(this);
    begin(0);
}


bool mouse::reset()
{
    if (_quick_check_hardware)
    {
        _mouse_available = send_cmd_reset();
    }
    else
    {
        // tries up to three times for mouse reset
        for (int i = 0; i < 3; ++i)
        {
            _mouse_available = send_cmd_reset();
            if (_mouse_available)
                break;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        // give the time to the device to be fully initialized
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // negotiate compatibility and default parameters
    if (_mouse_available)
    {
        // try Intellimouse (three buttons + scroll wheel, 4 bytes packet)
        if (send_cmd_set_sample_rate(200) && send_cmd_set_sample_rate(100)
            && send_cmd_set_sample_rate(80)
            && identify() == ps2_device_type::mouse_with_scroll_wheel)
        {
            // Intellimouse ok!
            _mouse_type = intellimouse;
        }

        set_sample_rate(60);
    }

    return _mouse_available;
}


int mouse::packet_size()
{
    return (_mouse_type == intellimouse ? 4 : 3);
}


bool mouse::packet_available()
{
    return uxQueueMessagesWaiting(_received_packet) > 0;
}


bool mouse::get_next_packet(mouse_packet *packet, int time_out_ms, bool request_resend_on_time_out)
{
    return xQueueReceive(_received_packet, packet, esp::ms_to_ticks(time_out_ms));
}


bool mouse::delta_available()
{
    return packet_available();
}


// Mouse packet format:
//    byte 0:
//       bit 0 = Left Button
//       bit 1 = Right Button
//       bit 2 = Middle Button
//       bit 3 = Always 1
//       bit 4 = X sign bit
//       bit 5 = Y sign bit
//       bit 6 = X overflow
//       bit 7 = Y overflow
//    byte 1:
//       X movement
//    byte 2:
//       Y movement
//    byte 3:
//       Z movement
bool mouse::decode_mouse_packet(mouse_packet *mouse_packet, mouse_delta *delta)
{
    // the bit 4 of first byte must be always 1
    if ((mouse_packet->data[0] & 8) == 0)
        return false;

    _prev_status = _status;

    // decode packet
    _status.buttons.left = (mouse_packet->data[0] & 0x01 ? 1 : 0);
    _status.buttons.middle = (mouse_packet->data[0] & 0x04 ? 1 : 0);
    _status.buttons.right = (mouse_packet->data[0] & 0x02 ? 1 : 0);
    if (delta)
    {
        delta->delta_x = (int16_t)(mouse_packet->data[0] & 0x10 ? 0xFF00 | mouse_packet->data[1]
                                                                : mouse_packet->data[1]);
        delta->delta_y = (int16_t)(mouse_packet->data[0] & 0x20 ? 0xFF00 | mouse_packet->data[2]
                                                                : mouse_packet->data[2]);
        delta->delta_z = (int8_t)(packet_size() > 3 ? mouse_packet->data[3] : 0);
        delta->overflow_x = (mouse_packet->data[0] & 0x40 ? 1 : 0);
        delta->overflow_y = (mouse_packet->data[0] & 0x80 ? 1 : 0);
        delta->buttons = _status.buttons;
    }

    return true;
}


bool mouse::get_next_delta(mouse_delta *delta, int time_out_ms, bool request_resend_on_time_out)
{
    mouse_packet mouse_packet;
    return get_next_packet(&mouse_packet, time_out_ms, request_resend_on_time_out)
        && decode_mouse_packet(&mouse_packet, delta);
}


void mouse::setup_absolute_positioner(
    int width, int height, bool create_absolute_positions_queue,
    esp::gfx::bitmapped_display_controller *update_display_controller)
{
    if (_area != esp::gfx::size(width, height))
    {
        _area = esp::gfx::size(width, height);
        _status.x = width >> 1;
        _status.y = height >> 1;
    }
    _status.wheel_delta = 0;
    _status.buttons.left = 0;
    _status.buttons.middle = 0;
    _status.buttons.right = 0;
    _prev_status = _status;

    _update_display_controller = update_display_controller;

    if (create_absolute_positions_queue && _absolute_queue == nullptr)
    {
        _absolute_queue = xQueueCreate(MOUSE_EVENTS_QUEUE_SIZE, sizeof(mouse_status));
    }

    if (_update_display_controller)
    {
        // setup initial position
        _update_display_controller->set_mouse_cursor_pos(_status.x, _status.y);
    }

    _absolute_update = (_update_display_controller || create_absolute_positions_queue);
}


void mouse::terminate_absolute_positioner()
{
    if (_absolute_queue)
    {
        vQueueDelete(_absolute_queue);
        _absolute_queue = nullptr;
    }
    _absolute_update = false;
    _update_display_controller = nullptr;
}


void mouse::update_absolute_position(mouse_delta *delta)
{
    const int maxDeltaTimeUS = 500000; // after 0.5s doesn't consider acceleration

    int dx = delta->delta_x;
    int dy = delta->delta_y;
    int dz = delta->delta_z;

    int64_t now = esp_timer_get_time();
    int delta_time = (int)(now - _prev_delta_time); // time in microseconds

    if (delta_time < maxDeltaTimeUS)
    {

        // calculate movement acceleration
        if (dx != 0 || dy != 0)
        {
            int delta_dist = isqrt(dx * dx + dy * dy); // distance in mouse points
            float vel = (float)delta_dist / delta_time; // velocity in mousepoints/microsecond
            float new_vel = vel + _movement_acceleration * vel * vel; // new velocity
            int new_delta_dist = new_vel * delta_time; // new distance
            dx = dx * new_delta_dist / delta_dist;
            dy = dy * new_delta_dist / delta_dist;
        }

        // calculate wheel acceleration
        if (dz != 0)
        {
            int delta_dist = abs(dz); // distance in wheel points
            float vel = (float)delta_dist / delta_time; // velocity in mousepoints/microsecond
            float new_vel = vel + _wheel_acceleration * vel * vel; // new velocity
            int new_delta_dist = new_vel * delta_time; // new distance
            dz = dz * new_delta_dist / delta_dist;
        }
    }

    _status.x = esp::tclamp((int)_status.x + dx, 0, _area.width - 1);
    _status.y = esp::tclamp((int)_status.y - dy, 0, _area.height - 1);
    _status.wheel_delta = dz;
    _prev_delta_time = now;
}


void mouse::mouse_update_task(void *arg)
{
    constexpr int MAX_TIME_BETWEEN_DATA_US =
        500000; // maximum time between data composing a delta frame

    mouse *mouse = (esp::ps2::mouse *)arg;

    int64_t prev_data_time = 0;

    while (true)
    {

        mouse_packet mouse_packet;
        int mouse_packet_len = 0;
        while (mouse_packet_len < mouse->packet_size())
        {
            int r = mouse->get_data(-1);
            if (mouse->parity_error() || mouse->sync_error())
            {
                mouse_packet_len = 0;
                continue;
            }
            int64_t now = esp_timer_get_time();
            if (mouse_packet_len > 0 && prev_data_time > 0
                && (now - prev_data_time) > MAX_TIME_BETWEEN_DATA_US)
            {
                // too much time elapsed since last byte, start a new delta
                mouse_packet_len = 0;
            }
            if (r > -1)
            {
                mouse_packet.data[mouse_packet_len++] = r;
                prev_data_time = now;
            }
        }

        if (mouse->_absolute_update)
        {
            mouse_delta delta;
            if (mouse->decode_mouse_packet(&mouse_packet, &delta))
            {
                mouse->update_absolute_position(&delta);

                // VGA Controller
                if (mouse->_update_display_controller)
                    mouse->_update_display_controller->set_mouse_cursor_pos(mouse->_status.x,
                                                                            mouse->_status.y);

                // queue (if you need availableStatus() or getNextStatus())
                if (mouse->_absolute_queue)
                {
                    xQueueSendToBack(mouse->_absolute_queue, &mouse->_status, 0);
                }
            }
        }
        else
        {

            xQueueOverwrite(mouse->_received_packet, &mouse_packet);
        }
    }
}


int mouse::available_status()
{
    return _absolute_queue ? uxQueueMessagesWaiting(_absolute_queue) : 0;
}


mouse_status mouse::get_next_status(int time_out_ms)
{
    mouse_status status;
    if (_absolute_queue)
        xQueueReceive(_absolute_queue, &status, esp::ms_to_ticks(time_out_ms));
    return status;
}


void mouse::empty_queue()
{
    while (get_data(0) != -1)
        ;
    if (_absolute_queue)
        xQueueReset(_absolute_queue);
}


} // end of namespace
