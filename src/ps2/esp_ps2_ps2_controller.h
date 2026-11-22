// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::ps2::PS2Controller definition.
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_tools.h"


namespace esp::ps2
{


/** \ingroup Enumerations
 * @brief This enum defines what is connected to PS/2 ports
 */
enum class ps2_preset
{
    keyboard_port_0_mouse_port_1, /**< Keyboard on Port 0 and Mouse on Port 1 */
    keyboard_port_1_mouse_port_0, /**< Keyboard on Port 1 and Mouse on Port 0 */
    keyboard_port_0, /**< Keyboard on Port 0 (no mouse) */
    keyboard_port_1, /**< Keyboard on Port 1 (no mouse) */
    mouse_port_0, /**< Mouse on port 0 (no keyboard) */
    mouse_port_1, /**< Mouse on port 1 (no keyboard) */
};


/** \ingroup Enumerations
 * @brief This enum defines how handle keyboard virtual keys
 */
enum class kbd_mode
{
    no_virtual_keys, /**< No virtual keys are generated */
    generate_virtual_keys, /**< Virtual keys are generated. You can call Keyboard.isVKDown() only. */
    create_virtual_keys_queue, /**< Virtual keys are generated and put on a queue. You can call Keyboard.isVKDown(), Keyboard.virtualKeyAvailable() and Keyboard.getNextVirtualKey() */
};


class keyboard;
class mouse;


/**
 * @brief The PS2 device controller class.
 *
 * The PS2 controller uses ULP coprocessor and RTC slow memory to communicate with up to two PS2 devices.<br>
 * The ULP coprocessor continuously monitor CLK and DATA lines for incoming data. Optionally can send commands to the PS2 devices.
 */
class ps2_controller
{

public:

    ps2_controller();
    ~ps2_controller();

    // unwanted methods
    ps2_controller(ps2_controller const &) = delete;
    void operator=(ps2_controller const &) = delete;

    /**
  * @brief Initializes PS2 device controller.
  *
  * Initializes the PS2 controller assigning GPIOs to DAT and CLK lines.
  *
  * Because PS/2 ports are handled by the ULP processor, just few GPIO ports are actually usable. They are:
  * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
  *
  * @param port0_clkGPIO The GPIO number of Clock line for PS/2 port 0.
  * @param port0_datGPIO The GPIO number of Data line for PS/2 port 0.
  * @param port1_clkGPIO The GPIO number of Clock line for PS/2 port 1 (GPIO_UNUSED to disable).
  * @param port1_datGPIO The GPIO number of Data line for PS/2 port 1 (GPIO_UNUSED to disable).
  */
    static void begin(gpio_num_t port_0_clk_gpio, gpio_num_t port_0_dat_gpio,
                      gpio_num_t port_1_clk_gpio = GPIO_UNUSED,
                      gpio_num_t port_1_dat_gpio = GPIO_UNUSED);

    /**
  * @brief Initializes PS2 device controller using default GPIOs.
  *
  * Initializes the PS2 controller assigning:
  *   - GPIO_NUM_33 (CLK) and GPIO_NUM_32 (DATA) as PS/2 Port 0
  *   - GPIO_NUM_26 (CLK) and GPIO_NUM_27 (DATA) as PS/2 Port 1
  *
  * @param preset Specifies what is connected to PS/2 ports (mouse, keyboard or boths).
  * @param keyboardMode Specifies how handle keyboard virtual keys.
  *
  * Example:
  *
  *     // Keyboard connected to port 0 and mouse to port1
  *     PSController.begin(PS2Preset::KeyboardPort0_MousePort1);
  */
    static void begin(ps2_preset preset = ps2_preset::keyboard_port_0_mouse_port_1,
                      kbd_mode keyboard_mode = kbd_mode::create_virtual_keys_queue);

    static void end();

    static bool initialized()
    {
        return _init_done;
    }

    /**
   * @brief Determines if one byte has been received from the specified port
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   *
   * @return True if one byte is available
   */
    static bool data_available(int ps2_port);

    /**
   * @brief Gets a scancode from the queue.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   * @param timeOutMS
   *
   * @return The first scancode of the queue (-1 if no data is available).
   */
    static int get_data(int ps2_port, int time_out_ms);

    /**
   * @brief Sends a command to the device.
   *
   * @param data Byte to send to the PS2 device.
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
    static void send_data(uint8_t data, int ps2_port);

    /**
   * @brief Disables inputs from PS/2 port driving the CLK line Low
   *
   * Use enableRX() to release CLK line.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
    static void disable_rx(int ps2_port);

    /**
   * @brief Enables inputs from PS/2 port releasing CLK line
   *
   * Use disableRX() to disable inputs from PS/2 port.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
    static void enable_rx(int ps2_port);

    /**
   * @brief Returns the instance of Keyboard object automatically created by PS2Controller.
   *
   * @return A pointer to a Keyboard object
   */
    static esp::ps2::keyboard *keyboard()
    {
        return _keyboard;
    }

    static void set_keyboard(esp::ps2::keyboard *value)
    {
        _keyboard = value;
    }

    /**
   * @brief Returns the instance of Mouse object automatically created by PS2Controller.
   *
   * @return A pointer to a Mouse object
   */
    static esp::ps2::mouse *mouse()
    {
        return _mouse;
    }

    static void set_mouse(esp::ps2::mouse *value)
    {
        _mouse = value;
    }

    /**
   * @brief Returns the singleton instance of PS2Controller class
   *
   * @return A pointer to PS2Controller singleton object
   */
    static ps2_controller *instance()
    {
        return _instance;
    }

    static bool parity_error(int ps2_port)
    {
        return _parity_error[ps2_port];
    }

    static bool sync_error(int ps2_port)
    {
        return _sync_error[ps2_port];
    }

    static bool clk_time_out_error(int ps2_port)
    {
        return _clk_time_out_error[ps2_port];
    }

    /**
   * @brief Gets exclusive access to the specified PS/2 port
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   * @param timeOutMS Timeout in milliseconds to wait before fail.
   *
   * @return True if the device has been locked.
   */
    static bool lock(int ps2_port, int time_out_ms);

    /**
   * @brief Releases port from exclusive access.
   *
   * @param PS2Port PS2 port number (0 = port 0, 1 = port1).
   */
    static void unlock(int ps2_port);


private:


    static void IRAM_ATTR ulp_wake_isr(void *arg);

    static ps2_controller *_instance;

    // Keyboard and Mouse instances can be created by PS2Controller in one of the begin() calls, or can be
    // set using setKeyboard() and setMouse() calls.
    static esp::ps2::keyboard *_keyboard;
    static esp::ps2::mouse *_mouse;

    static bool _keyboard_allocated;
    static bool _mouse_allocated;

    static bool _port_enabled[2];

    static intr_handle_t _ulp_wake_isr_handle;

    // true if last call to getData() had a parity, sync error (start or stop missing bits) or CLK timeout
    static bool _parity_error[2];
    static bool _sync_error[2];
    static bool _clk_time_out_error[2];

    // one word queue (contains just the last received word)
    static QueueHandle_t _data_in[2];

    static SemaphoreHandle_t _port_lock[2];

    static bool _init_done;
};


struct ps2_port_auto_disable_rx
{
    ps2_port_auto_disable_rx(int ps2_port)
        : _port(ps2_port)
    {
        ps2_controller::disable_rx(_port);
    }
    ~ps2_port_auto_disable_rx()
    {
        ps2_controller::enable_rx(_port);
    }

private:

    int _port;
};


} // end of namespace
