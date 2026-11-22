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


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::ps2::Keyboard definition.
 */


#include "freertos/FreeRTOS.h"

#include "esp_ps2_codepages.h"
#include "esp_ps2_config.h"
#include "esp_ps2_keyboard_layouts.h"
#include "esp_ps2_ps2_device.h"
#include "esp_tools.h"


namespace esp::ps2
{


/**
 * @brief The PS2 Keyboard controller class.
 *
 * Keyboard connects to one port of the PS2 Controller class (esp::ps2::PS2Controller) and provides the logic
 * that converts scancodes to virtual keys or ASCII (and ANSI) codes.<br>
 * It optionally creates a task that waits for scan codes from the PS2 device and puts virtual keys in a queue.<br>
 * The PS2 controller uses ULP coprocessor and RTC slow memory to communicate with the PS2 device.<br>
 * <br>
 * It is possible to specify an international keyboard layout. The default is US-layout.<br>
 * There are several predefined kayboard layouts: US (USA), UK (United Kingdom), DE (German), IT (Italian), ES (Spanish), FR (French), BE (Belgian) and NO (Norwegian). Other layout can be added
 * inheriting from US or from any other layout.
 *
 * Applications do not need to create an instance of Keyboard because an instance named Keyboard is created automatically.
 *
 * Example:
 *
 *     esp::ps2::Keyboard Keyboard;
 *     
 *     // Setup pins GPIO33 for CLK and GPIO32 for DATA
 *     Keyboard.begin(GPIO_NUM_33, GPIO_NUM_32);  // clk, dat
 *
 *     // Prints name of received virtual keys
 *     while (true)
 *       Serial.printf("VirtualKey = %s\n", Keyboard.virtualKeyToString(Keyboard.getNextVirtualKey()));
 *
 */
class keyboard : public ps2_device
{

public:

    keyboard();
    ~keyboard();

    /**
   * @brief Initializes Keyboard specifying CLOCK and DATA GPIOs.
   *
   * A reset command (Keyboard.reset() method) is automatically sent to the keyboard.<br>
   * This method also initializes the PS2Controller to use port 0 only.
   *
   * Because PS/2 ports are handled by the ULP processor, just few GPIO ports are actually usable. They are:
   * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
   *
   * @param clkGPIO The GPIO number of Clock line
   * @param dataGPIO The GPIO number of Data line
   * @param generateVirtualKeys If true creates a task which consumes scancodes to produce virtual keys,
   *                            so you can call Keyboard.isVKDown().
   * @param createVKQueue If true creates a task which consunes scancodes and produces virtual keys
   *                      and put them in a queue, so you can call Keyboard.isVKDown(), Keyboard.virtualKeyAvailable()
   *                      and Keyboard.getNextVirtualKey().
   *
   * Example:
   *
   *     // Setup pins GPIO33 for CLK and GPIO32 for DATA
   *     Keyboard.begin(GPIO_NUM_33, GPIO_NUM_32);  // clk, dat
   */
    void begin(gpio_num_t clk_gpio, gpio_num_t data_gpio, bool generate_virtual_keys = true,
               bool create_vk_queue = true);

    /**
   * @brief Initializes Keyboard without initializing the PS/2 controller.
   *
   * A reset command (Keyboard.reset() method) is automatically sent to the keyboard.<br>
   * This method does not initialize the PS2Controller.
   *
   * @param generateVirtualKeys If true creates a task which consumes scancodes and produces virtual keys,
   *                            so you can call Keyboard.isVKDown().
   * @param createVKQueue If true creates a task which consunes scancodes to produce virtual keys
   *                      and put them in a queue, so you can call Keyboard.isVKDown(), Keyboard.virtualKeyAvailable()
   *                      and Keyboard.getNextVirtualKey().
   * @param PS2Port The PS/2 port to use (0 or 1).
   *
   * Example:
   *
   *     // Setup pins GPIO33 for CLK and GPIO32 for DATA on port 0
   *     PS2Controller.begin(GPIO_NUM_33, GPIO_NUM_32); // clk, dat
   *     Keyboard.begin(true, true, 0); // port 0
   */
    void begin(bool generate_virtual_keys, bool create_vk_queue, int ps2_port);

    /**
   * @brief Dynamically enables or disables Virtual Keys generation
   *
   * This method can be called after begin().
   *
   * @param generateVirtualKeys If true creates a task which consumes scancodes and produces virtual keys,
   *                            so you can call Keyboard.isVKDown(). If False destroys the task.
   * @param createVKQueue If true creates a task which consunes scancodes to produce virtual keys
   *                      and put them in a queue, so you can call Keyboard.isVKDown(), Keyboard.virtualKeyAvailable()
   *                      and Keyboard.getNextVirtualKey(). I False destroys the queue.
   */
    void enable_virtual_keys(bool generate_virtual_keys, bool create_vk_queue);

    /**
   * @brief Sends a Reset command to the keyboard.
   *
   * @return True if the keyboard is correctly initialized.
   */
    bool reset();

    /**
   * @brief Checks if keyboard has been detected and correctly initialized.
   *
   * isKeyboardAvailable() returns a valid value only after Keyboard.begin() or Keyboard.reset() has been called.
   *
   * @return True if the keyboard is correctly initialized.
   */
    bool is_keyboard_available()
    {
        return _keyboard_available;
    }

    /**
   * @brief Sets keyboard layout.
   *
   * It is possible to specify an international keyboard layout. The default is US-layout.<br>
   * There are following predefined kayboard layouts: US (USA), UK (United Kingdom), DE (German), IT (Italian), ES (Spanish), FR (French), BE (Belgian) and NO (Norwegian). Other layout can be added
   * inheriting from US or from any other layout.
   *
   * @param layout A pointer to the layout structure.
   *
   * Example:
   *
   *     // Set German layout
   *     Keyboard.setLayout(&esp::ps2::GermanLayout);
   */
    void set_layout(keyboard_layout const *layout);

    /**
   * @brief Gets current keyboard layout.
   *
   * @return The default or last set keyboard layout.
   */
    keyboard_layout const *layout()
    {
        return _layout;
    }

    /**
   * @brief Gets the virtual keys status.
   *
   * This method allows to know the status of each virtual key (Down or Up).<br>
   * Virtual keys are generated from scancodes only if generateVirtualKeys parameter of Keyboard.begin() method is true (default).
   *
   * @param virtualKey The Virtual Key to test.
   *
   * @return True if the specified virtual key is down.
   */
    bool is_vk_down(virtual_key virtual_key);

    /**
   * @brief Gets the number of virtual keys available in the queue.
   *
   * Virtual keys are generated from scancodes only if generateVirtualKeys parameter is true (default)
   * and createVKQueue parameter is true (default) of Keyboard.begin() method.
   *
   * @return The number of virtual keys available to read.
   */
    int virtual_key_available();

    /**
   * @brief Gets a virtual key from the queue.
   *
   * Virtual keys are generated from scancodes only if generateVirtualKeys parameter is true (default)
   * and createVKQueue parameter is true (default) of Keyboard.begin() method.
   *
   * @param keyDown A pointer to boolean variable which will contain if the virtual key is depressed (true) or released (false).
   * @param timeOutMS Timeout in milliseconds. -1 means no timeout (infinite time).
   *
   * @return The first virtual key of the queue (VK_NONE if no data is available in the timeout period).
   */
    virtual_key get_next_virtual_key(bool *key_down = nullptr, int time_out_ms = -1);

    /**
   * @brief Gets a virtual key from the queue, including associated scan code.
   *
   * Virtual keys are generated from scancodes only if generateVirtualKeys parameter is true (default)
   * and createVKQueue parameter is true (default) of Keyboard.begin() method.
   *
   * @param item Pointer to VirtualKeyItem structure which receive virtual key info.
   * @param timeOutMS Timeout in milliseconds. -1 means no timeout (infinite time).
   *
   * @return True if item has been received. False if no data is available in the timeout period.
   */
    bool get_next_virtual_key(virtual_key_item *item, int time_out_ms = -1);

    /**
   * @brief Adds or inserts a virtual key into the virtual keys queue
   *
   * @param virtualKey Virtual key to add or insert
   * @param keyDown True is the virtual key is down
   * @param insert If true virtual key is inserted as first item
   */
    void inject_virtual_key(virtual_key virtual_key, bool key_down, bool insert = false);

    /**
   * @brief Adds or inserts a virtual key info into the virtual keys queue
   *
   * @param item Virtual key info add or insert
   * @param insert If true virtual key is inserted as first item
   */
    void inject_virtual_key(virtual_key_item const &item, bool insert = false);

    /**
   * @brief Empties the virtual keys queue
   */
    void empty_virtual_key_queue();

    /**
   * @brief Converts virtual key to ASCII.
   *
   * This method converts the specified virtual key to ASCII, if possible.<br>
   * <br>For example VK_A is converted to 'A' (ASCII 0x41), CTRL  + VK_SPACE produces ASCII NUL (0x00), CTRL + letter produces
   * ASCII control codes from SOH (0x01) to SUB (0x1A), CTRL + VK_BACKSLASH produces ASCII FS (0x1C), CTRL + VK_QUESTION produces
   * ASCII US (0x1F), CTRL + VK_LEFTBRACKET produces ASCII ESC (0x1B), CTRL + VK_RIGHTBRACKET produces ASCII GS (0x1D),
   * CTRL + VK_TILDE produces ASCII RS (0x1E) and VK_SCROLLLOCK produces XON or XOFF.<br>
   * <br>This method uses current codepage set using Keyboard.setCodePage()<br>
   *
   * @param virtualKey The virtual key to convert.
   *
   * @return The ASCII code of virtual key or -1 if virtual key cannot be translated to ASCII.
   */
    int virtual_key_to_ascii(virtual_key virtual_key);

    /**
   * @brief Sets font codepage for virtual key to ASCII conversion
   *
   * Sets current codepage for virtualkey to ASCII conversions. This method is called automatically by Terminal and UI interface.
   *
   * @param codepage Codepage to use
   *
   * Example:
   *
   *     // Set codepage 437
   *     Keyboard.setCodePage(CodePages::get(437));
   */
    void set_code_page(code_page const *codepage)
    {
        _codepage = codepage;
    }

    /**
   * @brief Gets the number of scancodes available in the queue.
   *
   * Scancodes are always generated but they can be consumed by the scancode-to-virtualkeys task. So, in order to use this
   * method Keyboard.begin() method should be called with generateVirtualKeys = false and createVKQueue = false.<br>
   *
   * @return The number of scancodes available to read.
   */
    int scancode_available();

    /**
   * @brief Gets a scancode from the queue.
   *
   * Scancodes are always generated but they can be consumed by the scancode-to-virtualkeys task. So, in order to use this
   * method Keyboard.begin() method should be called with generateVirtualKeys = false and createVKQueue = false.<br>
   *
   * @param timeOutMS Timeout in milliseconds. -1 means no timeout (infinite time).
   * @param requestResendOnTimeOut If true and timeout has expired then asks the keyboard to resend the scancode.
   *
   * @return The first scancode of the queue (-1 if no data is available in the timeout period).
   */
    int get_next_scancode(int time_out_ms = -1, bool request_resend_on_time_out = false);

    /**
   * @brief Sets keyboard LEDs status.
   *
   * Use this method to switch-on or off the NUMLOCK, CAPSLOCK and SCROLLLOCK LEDs.
   *
   * @param numLock When true the NUMLOCK LED is switched on.
   * @param capsLock When true the CAPSLOCK LED is switched on.
   * @param scrollLock When true the SCROLLLOCK LED is switched on.
   *
   * @return True if command has been successfully delivered to the keyboard.
   */
    bool set_leds(bool num_lock, bool caps_lock, bool scroll_lock);

    /**
   * @brief Gets keyboard LEDs status.
   *
   * Use this method to know the current status of NUMLOCK, CAPSLOCK and SCROLLLOCK LEDs.
   *
   * @param numLock When true the NUMLOCK LED is switched on.
   * @param capsLock When true the CAPSLOCK LED is switched on.
   * @param scrollLock When true the SCROLLLOCK LED is switched on.
   */
    void get_leds(bool *num_lock, bool *caps_lock, bool *scroll_lock);

    /**
   * @brief Sets typematic rate and delay.
   *
   * If the key is kept pressed down, after repeatDelayMS keyboard starts periodically sending codes with frequency repeatRateMS.
   *
   * @param repeatRateMS Repeat rate in milliseconds (in range 33 ms ... 500 ms).
   * @param repeatDelayMS Repeat delay in milliseconds (in range 250 ms ... 1000 ms, steps of 250 ms).
   *
   * @return True if command has been successfully delivered to the keyboard.
   */
    bool set_typematic_rate_and_delay(int repeat_rate_ms, int repeat_delay_ms)
    {
        return send_cmd_typematic_rate_and_delay(repeat_rate_ms, repeat_delay_ms);
    }

    /**
   * @brief Sets the scancode set
   *
   * Scancode set 1 is used on original IBM PC XT (https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1)
   * Scancode set 2 is used on IBM PC AT (https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_2)
   * Scancode set 3 is used on IBM 3270 (https://web.archive.org/web/20170108131104/http://www.computer-engineering.org/ps2keyboard/scancodes3.html)
   *
   * When virtual keys are enabled only set 1 and set 2 are available.
   *
   * @param value Scancode set (1, 2 or 3).
   *
   * @return True if scancode has been set.
   */
    bool set_scancode_set(int value);

    /**
   * @brief Gets current scancode set
   *
   * @return Current scan code set (1, 2 or 3)
   */
    int scancode_set()
    {
        return _scancode_set;
    }

    static uint8_t conv_scancode_set_2_to_1(uint8_t code);

#if FABGLIB_HAS_VirtualKeyO_STRING
    static char const *virtual_key_to_string(virtual_key virtual_key);
#endif


    //// Delegates ////

    /**
   * @brief Delegate called whenever a new virtual key is decoded from scancodes
   *
   * First parameter is a pointer to the decoded virtual key
   * Second parameter specifies if the key is Down (true) or Up (false)
   */
    delegate<virtual_key *, bool> on_virtual_key;


    //// statics (used for common default properties) ////


    /**
   * @brief Stack size of the task that converts scancodes to Virtual Keys Keyboard.
   *
   * Application should change before begin() method.
   *
   * Default value is FABGLIB_DEFAULT_SCODETOVK_TASK_STACK_SIZE (defined in esp_ps2_config.h)
   */
    static int scancode_to_virtual_key_task_stack_size;


private:

    virtual_key scancode_to_vk(uint8_t scancode, bool is_extended,
                               keyboard_layout const *layout = nullptr);
    virtual_key v_kto_alternate_vk(virtual_key in_vk, bool down,
                                   keyboard_layout const *layout = nullptr);
    virtual_key manage_capslock(virtual_key vk);
    void update_leds();
    bool blocking_get_virtual_key(virtual_key_item *item);
    void convert_scancode2to_1(virtual_key_item *item);
    void post_virtual_key_item(virtual_key_item const &item);
    static int conv_keypad_vk_to_num(virtual_key vk);

    static void s_code_to_vk_converter_task(void *pv_parameters);


    bool _keyboard_available; // self test passed and support for scancode set 2

    TaskHandle_t
        _s_code_to_vk_converter_task; // Task that converts scancodes to virtual key and populates m_virtualKeyQueue
    QueueHandle_t _virtual_key_queue;

    uint8_t _vk_map[(int)(vk_last + 7) / 8];

    // allowed values: 1, 2 or 3
    // If virtual keys are enabled only 1 and 2 are possible. In case of scancode set 1 it is converted from scan code set 2, which is necessary
    // in order to decode virtual keys.
    uint8_t _scancode_set;

    keyboard_layout const *_layout;

    bool _ctrl;
    bool _lalt;
    bool _ralt;
    bool _shift;
    bool _capslock;
    bool _gui;
    bool _numlock;
    bool _scrolllock;

    virtual_key _last_dead_key;

    // store status of the three LEDs
    bool _num_lock_led;
    bool _caps_lock_led;
    bool _scroll_lock_led;

    code_page const *_codepage;
};


} // end of namespace
