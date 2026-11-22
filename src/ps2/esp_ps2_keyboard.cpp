// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_ps2_keyboard.h"


#pragma GCC optimize("O2")


namespace esp::ps2
{


int keyboard::scancode_to_virtual_key_task_stack_size = SCANCODE_TASK_STACK_SIZE;


keyboard::keyboard()
    : _keyboard_available(false)
    , _s_code_to_vk_converter_task(nullptr)
    , _virtual_key_queue(nullptr)
    , _scancode_set(2)
    , _last_dead_key(vk_none)
    , _codepage(nullptr)
{
}


keyboard::~keyboard()
{
    enable_virtual_keys(false, false);
}


void keyboard::begin(bool generate_virtual_keys, bool create_vk_queue, int ps2_port)
{
    ps2_device::begin(ps2_port);

    _ctrl = false;
    _lalt = false;
    _ralt = false;
    _shift = false;
    _capslock = false;
    _gui = false;
    _numlock = false;
    _scrolllock = false;

    _num_lock_led = false;
    _caps_lock_led = false;
    _scroll_lock_led = false;

    reset();

    enable_virtual_keys(generate_virtual_keys, create_vk_queue);
}


void keyboard::begin(gpio_num_t clk_gpio, gpio_num_t data_gpio, bool generate_virtual_keys,
                     bool create_vk_queue)
{
    ps2_controller::begin(clk_gpio, data_gpio);
    ps2_controller::set_keyboard(this);
    begin(generate_virtual_keys, create_vk_queue, 0);
}


void keyboard::enable_virtual_keys(bool generate_virtual_keys, bool create_vk_queue)
{
    ps2_device_lock lock(this);

    if (create_vk_queue)
        generate_virtual_keys = true;

    // create task and queue?

    if (!_virtual_key_queue && create_vk_queue)
        _virtual_key_queue = xQueueCreate(VIRTUAL_KEY_QUEUE_SIZE, sizeof(virtual_key_item));

    if (!_s_code_to_vk_converter_task && generate_virtual_keys)
        xTaskCreate(&s_code_to_vk_converter_task, "",
                    keyboard::scancode_to_virtual_key_task_stack_size, this, SCANCODE_TASK_PRIORITY,
                    &_s_code_to_vk_converter_task);

    // destroy in reverse order

    if (_s_code_to_vk_converter_task && !generate_virtual_keys)
    {
        vTaskDelete(_s_code_to_vk_converter_task);
        _s_code_to_vk_converter_task = nullptr;
    }

    if (_virtual_key_queue && !create_vk_queue)
    {
        vQueueDelete(_virtual_key_queue);
        _virtual_key_queue = nullptr;
    }
}


// reset keyboard, set scancode 2 and US layout
bool keyboard::reset()
{
    memset(_vk_map, 0, sizeof(_vk_map));

    // sets default layout
    set_layout(&US_LAYOUT);

    // 350ms keyboard poweron delay (look at NXP M68HC08 designer reference manual)
    vTaskDelay(350 / portTICK_PERIOD_MS);

    // tries up to three times to reset keyboard
    for (int i = 0; i < 3; ++i)
    {
        _keyboard_available = send_cmd_reset();
        if (_keyboard_available)
            break;
        vTaskDelay(350 / portTICK_PERIOD_MS);
    }
    // give the time to the device to be fully initialized
    vTaskDelay(200 / portTICK_PERIOD_MS);

    send_cmd_set_scancode_set(2);

    return _keyboard_available;
}


bool keyboard::set_scancode_set(int value)
{
    if (_s_code_to_vk_converter_task)
    {
        // virtual keys enabled, just 1 and 2 are allowed
        if (value == 1 || value == 2)
        {
            _scancode_set = value;
            return true;
        }
    }
    else
    {
        // no virtual keys enabled, just try to tell keyboard which set we need
        if (send_cmd_set_scancode_set(value))
        {
            _scancode_set = value;
            return true;
        }
    }
    return false;
}


bool keyboard::set_leds(bool num_lock, bool caps_lock, bool scroll_lock)
{
    _num_lock_led = num_lock;
    _caps_lock_led = caps_lock;
    _scroll_lock_led = scroll_lock;
    return send_cmd_leds(num_lock, caps_lock, scroll_lock);
}


void keyboard::get_leds(bool *num_lock, bool *caps_lock, bool *scroll_lock)
{
    *num_lock = _num_lock_led;
    *caps_lock = _caps_lock_led;
    *scroll_lock = _scroll_lock_led;
}


void keyboard::update_leds()
{
    send_cmd_leds(_numlock, _capslock, _scrolllock);
    _num_lock_led = _numlock;
    _caps_lock_led = _capslock;
    _scroll_lock_led = _scrolllock;
}


int keyboard::scancode_available()
{
    return data_available();
}


int keyboard::get_next_scancode(int time_out_ms, bool request_resend_on_time_out)
{
    while (true)
    {
        int r = get_data(time_out_ms);
        if (r == -1 && clk_time_out_error())
        {
            // try to recover a stall sending a re-enable scanning command
            send_cmd_enable_scanning();
        }
        if (r == -1 && request_resend_on_time_out)
        {
            request_to_resend_last_byte();
            continue;
        }
        return r;
    }
}


void keyboard::set_layout(const keyboard_layout *layout)
{
    _layout = layout;
}


#if ESP_PS2_VIRTUAL_KEY_NAMES
char const *keyboard::virtual_key_to_string(virtual_key virtual_key)
{
    char const *vktostr[] = {
        "VK_NONE",
        "VK_SPACE",
        "VK_0",
        "VK_1",
        "VK_2",
        "VK_3",
        "VK_4",
        "VK_5",
        "VK_6",
        "VK_7",
        "VK_8",
        "VK_9",
        "VK_KP_0",
        "VK_KP_1",
        "VK_KP_2",
        "VK_KP_3",
        "VK_KP_4",
        "VK_KP_5",
        "VK_KP_6",
        "VK_KP_7",
        "VK_KP_8",
        "VK_KP_9",
        "VK_a",
        "VK_b",
        "VK_c",
        "VK_d",
        "VK_e",
        "VK_f",
        "VK_g",
        "VK_h",
        "VK_i",
        "VK_j",
        "VK_k",
        "VK_l",
        "VK_m",
        "VK_n",
        "VK_o",
        "VK_p",
        "VK_q",
        "VK_r",
        "VK_s",
        "VK_t",
        "VK_u",
        "VK_v",
        "VK_w",
        "VK_x",
        "VK_y",
        "VK_z",
        "VK_A",
        "VK_B",
        "VK_C",
        "VK_D",
        "VK_E",
        "VK_F",
        "VK_G",
        "VK_H",
        "VK_I",
        "VK_J",
        "VK_K",
        "VK_L",
        "VK_M",
        "VK_N",
        "VK_O",
        "VK_P",
        "VK_Q",
        "VK_R",
        "VK_S",
        "VK_T",
        "VK_U",
        "VK_V",
        "VK_W",
        "VK_X",
        "VK_Y",
        "VK_Z",
        "VK_GRAVEACCENT",
        "VK_ACUTEACCENT",
        "VK_QUOTE",
        "VK_QUOTEDBL",
        "VK_EQUALS",
        "VK_MINUS",
        "VK_KP_MINUS",
        "VK_PLUS",
        "VK_KP_PLUS",
        "VK_KP_MULTIPLY",
        "VK_ASTERISK",
        "VK_BACKSLASH",
        "VK_KP_DIVIDE",
        "VK_SLASH",
        "VK_KP_PERIOD",
        "VK_PERIOD",
        "VK_COLON",
        "VK_COMMA",
        "VK_SEMICOLON",
        "VK_AMPERSAND",
        "VK_VERTICALBAR",
        "VK_HASH",
        "VK_AT",
        "VK_CARET",
        "VK_DOLLAR",
        "VK_POUND",
        "VK_EURO",
        "VK_PERCENT",
        "VK_EXCLAIM",
        "VK_QUESTION",
        "VK_LEFTBRACE",
        "VK_RIGHTBRACE",
        "VK_LEFTBRACKET",
        "VK_RIGHTBRACKET",
        "VK_LEFTPAREN",
        "VK_RIGHTPAREN",
        "VK_LESS",
        "VK_GREATER",
        "VK_UNDERSCORE",
        "VK_DEGREE",
        "VK_SECTION",
        "VK_TILDE",
        "VK_NEGATION",
        "VK_LSHIFT",
        "VK_RSHIFT",
        "VK_LALT",
        "VK_RALT",
        "VK_LCTRL",
        "VK_RCTRL",
        "VK_LGUI",
        "VK_RGUI",
        "VK_ESCAPE",
        "VK_PRINTSCREEN",
        "VK_SYSREQ",
        "VK_INSERT",
        "VK_KP_INSERT",
        "VK_DELETE",
        "VK_KP_DELETE",
        "VK_BACKSPACE",
        "VK_HOME",
        "VK_KP_HOME",
        "VK_END",
        "VK_KP_END",
        "VK_PAUSE",
        "VK_BREAK",
        "VK_SCROLLLOCK",
        "VK_NUMLOCK",
        "VK_CAPSLOCK",
        "VK_TAB",
        "VK_RETURN",
        "VK_KP_ENTER",
        "VK_APPLICATION",
        "VK_PAGEUP",
        "VK_KP_PAGEUP",
        "VK_PAGEDOWN",
        "VK_KP_PAGEDOWN",
        "VK_UP",
        "VK_KP_UP",
        "VK_DOWN",
        "VK_KP_DOWN",
        "VK_LEFT",
        "VK_KP_LEFT",
        "VK_RIGHT",
        "VK_KP_RIGHT",
        "VK_KP_CENTER",
        "VK_F1",
        "VK_F2",
        "VK_F3",
        "VK_F4",
        "VK_F5",
        "VK_F6",
        "VK_F7",
        "VK_F8",
        "VK_F9",
        "VK_F10",
        "VK_F11",
        "VK_F12",
        "VK_GRAVE_a",
        "VK_GRAVE_e",
        "VK_ACUTE_e",
        "VK_GRAVE_i",
        "VK_GRAVE_o",
        "VK_GRAVE_u",
        "VK_CEDILLA_c",
        "VK_ESZETT",
        "VK_UMLAUT_u",
        "VK_UMLAUT_o",
        "VK_UMLAUT_a",
        "VK_CEDILLA_C",
        "VK_TILDE_n",
        "VK_TILDE_N",
        "VK_UPPER_a",
        "VK_ACUTE_a",
        "VK_ACUTE_i",
        "VK_ACUTE_o",
        "VK_ACUTE_u",
        "VK_UMLAUT_i",
        "VK_EXCLAIM_INV",
        "VK_QUESTION_INV",
        "VK_ACUTE_A",
        "VK_ACUTE_E",
        "VK_ACUTE_I",
        "VK_ACUTE_O",
        "VK_ACUTE_U",
        "VK_GRAVE_A",
        "VK_GRAVE_E",
        "VK_GRAVE_I",
        "VK_GRAVE_O",
        "VK_GRAVE_U",
        "VK_INTERPUNCT",
        "VK_DIAERESIS",
        "VK_UMLAUT_e",
        "VK_UMLAUT_A",
        "VK_UMLAUT_E",
        "VK_UMLAUT_I",
        "VK_UMLAUT_O",
        "VK_UMLAUT_U",
        "VK_CARET_a",
        "VK_CARET_e",
        "VK_CARET_i",
        "VK_CARET_o",
        "VK_CARET_u",
        "VK_CARET_A",
        "VK_CARET_E",
        "VK_CARET_I",
        "VK_CARET_O",
        "VK_CARET_U",
        "VK_ASCII",
    };
    return vktostr[virtual_key];
}
#endif


// -1 = virtual key cannot be translated to ASCII
int keyboard::virtual_key_to_ascii(virtual_key virtual_key)
{
    virtual_key_item item;
    item.vk = virtual_key;
    item.down = true;
    item.ctrl = _ctrl;
    item.lalt = _lalt;
    item.ralt = _ralt;
    item.shift = _shift;
    item.gui = _gui;
    item.capslock = _capslock;
    item.numlock = _numlock;
    item.scrolllock = _scrolllock;
    return esp::ps2::virtual_key_to_ascii(item, _codepage);
}


virtual_key keyboard::scancode_to_vk(uint8_t scancode, bool is_extended,
                                     keyboard_layout const *layout)
{
    virtual_key vk = vk_none;

    if (layout == nullptr)
        layout = _layout;

    virtual_key_def const *def = is_extended ? layout->ex_scancode_to_vk : layout->scancode_to_vk;
    for (; def->scancode; ++def)
        if (def->scancode == scancode)
        {
            vk = def->virtual_key;
            break;
        }

    if (vk == vk_none && layout->inherited)
        vk = scancode_to_vk(scancode, is_extended, layout->inherited);

    // manage keypad
    // NUMLOCK ON, SHIFT OFF => generate VK_KP_number
    // NUMLOCK ON, SHIFT ON  => generate VK_KP_cursor_control (as when NUMLOCK is OFF)
    // NUMLOCK OFF           => generate VK_KP_cursor_control
    if (_numlock & !_shift)
    {
        switch (vk)
        {
            case vk_kp_delete:
                vk = vk_kp_period;
                break;
            case vk_kp_insert:
                vk = vk_kp_0;
                break;
            case vk_kp_end:
                vk = vk_kp_1;
                break;
            case vk_kp_down:
                vk = vk_kp_2;
                break;
            case vk_kp_pagedown:
                vk = vk_kp_3;
                break;
            case vk_kp_left:
                vk = vk_kp_4;
                break;
            case vk_kp_center:
                vk = vk_kp_5;
                break;
            case vk_kp_right:
                vk = vk_kp_6;
                break;
            case vk_kp_home:
                vk = vk_kp_7;
                break;
            case vk_kp_up:
                vk = vk_kp_8;
                break;
            case vk_kp_pageup:
                vk = vk_kp_9;
                break;
            default:
                break;
        }
    }

    return vk;
}


virtual_key keyboard::manage_capslock(virtual_key vk)
{
    if (_capslock)
    {
        // inverts letters case
        if (vk >= vk_a && vk <= vk_z)
            vk = (virtual_key)(vk - vk_a + vk_A);
        else if (vk >= vk_A && vk <= vk_Z)
            vk = (virtual_key)(vk - vk_A + vk_a);
    }
    return vk;
}


virtual_key keyboard::v_kto_alternate_vk(virtual_key in_vk, bool down,
                                         keyboard_layout const *layout)
{
    virtual_key vk = vk_none;

    if (layout == nullptr)
        layout = _layout;

    // this avoids releasing a required key when SHIFT has been pressed after the key but before releasing
    if (!down && is_vk_down(in_vk))
        vk = in_vk;

    if (vk == vk_none)
    {
        // handle this case:
        //   - derived KEY up without any SHIFT (because released before the KEY, ie SHIFT+"1" => "!", but you release the SHIFT before "1")
        // this avoid to maintain a KEY DOWN when you release the SHIFT key before the KEY ()
        for (alt_virtual_key_def const *def = layout->alternate_vk; def->req_virtual_key != vk_none;
             ++def)
        {
            if (def->req_virtual_key == in_vk && is_vk_down(def->virtual_key))
            {
                vk = def->virtual_key;
                break;
            }
        }
    }

    if (vk == vk_none)
    {
        // handle these cases:
        //   - KEY down with SHIFTs already down
        //   - KEY up with SHIFTs still down
        for (alt_virtual_key_def const *def = layout->alternate_vk; def->req_virtual_key != vk_none;
             ++def)
        {
            if (def->req_virtual_key == in_vk && def->ctrl == _ctrl && def->lalt == _lalt
                && def->ralt == _ralt && def->shift == _shift)
            {
                vk = def->virtual_key;
                break;
            }
        }
    }

    if (vk == vk_none && layout->inherited)
        vk = v_kto_alternate_vk(in_vk, down, layout->inherited);

    return vk == vk_none ? in_vk : vk;
}


bool keyboard::blocking_get_virtual_key(virtual_key_item *item)
{
    item->vk = vk_none;
    item->down = true;
    item->ctrl = _ctrl;
    item->lalt = _lalt;
    item->ralt = _ralt;
    item->shift = _shift;
    item->gui = _gui;
    item->capslock = _capslock;
    item->numlock = _numlock;
    item->scrolllock = _scrolllock;

    uint8_t *scode = item->scancode;

    *scode = get_next_scancode();
    if (*scode == 0xE0)
    {
        // two bytes scancode
        *(++scode) = get_next_scancode(100, true);
        if (*scode == 0xF0)
        {
            // two bytes scancode key up
            *(++scode) = get_next_scancode(100, true);
            item->vk = scancode_to_vk(*scode, true);
            item->down = false;
        }
        else
        {
            // two bytes scancode key down
            item->vk = scancode_to_vk(*scode, true);
        }
    }
    else if (*scode == 0xE1)
    {
        // special case: "PAUSE" : 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77
        static const uint8_t PAUSECODES[] = {0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
        for (int i = 0; i < sizeof(PAUSECODES); ++i)
        {
            *(++scode) = get_next_scancode(100, true);
            if (*scode != PAUSECODES[i])
                break;
            else if (i == sizeof(PAUSECODES) - 1)
                item->vk = vk_pause;
        }
    }
    else if (*scode == 0xF0)
    {
        // one byte scancode, key up
        *(++scode) = get_next_scancode(100, true);
        item->vk = scancode_to_vk(*scode, false);
        item->down = false;
    }
    else
    {
        // one byte scancode, key down
        item->vk = scancode_to_vk(*scode, false);
    }

    if (item->vk != vk_none)
    {

        // manage CAPSLOCK
        item->vk = manage_capslock(item->vk);

        // alternate VK (virtualkeys modified by shift, alt, ...)
        item->vk = v_kto_alternate_vk(item->vk, item->down);

        // update shift, alt, ctrl, capslock, numlock and scrollock states and LEDs
        switch (item->vk)
        {
            case vk_lctrl:
            case vk_rctrl:
                _ctrl = item->down;
                break;
            case vk_lalt:
                _lalt = item->down;
                break;
            case vk_ralt:
                _ralt = item->down;
                break;
            case vk_lshift:
            case vk_rshift:
                _shift = item->down;
                break;
            case vk_lgui:
            case vk_rgui:
                _gui = item->down;
                break;
            case vk_capslock:
                if (!item->down)
                {
                    _capslock = !_capslock;
                    update_leds();
                }
                break;
            case vk_numlock:
                if (!item->down)
                {
                    _numlock = !_numlock;
                    update_leds();
                }
                break;
            case vk_scrolllock:
                if (!item->down)
                {
                    _scrolllock = !_scrolllock;
                    update_leds();
                }
                break;
            default:
                break;
        }
    }

    // manage dead keys - Implemented by Carles Oriol (https://github.com/carlesoriol)
    for (virtual_key const *dk = _layout->dead_keys_vk; *dk != vk_none; ++dk)
    {
        if (item->vk == *dk)
        {
            _last_dead_key = item->vk;
            item->vk = vk_none;
        }
    }
    if (item->vk != _last_dead_key && item->vk != vk_none)
    {
        for (dead_key_virtual_key_def const *dk = _layout->deadkeys_to_vk; dk->dead_key != vk_none;
             ++dk)
        {
            if (item->vk == dk->req_virtual_key && _last_dead_key == dk->dead_key)
            {
                item->vk = dk->virtual_key;
                break;
            }
        }
        if (!item->down && (item->vk != _last_dead_key) && (item->vk != vk_rshift)
            && (item->vk != vk_lshift))
            _last_dead_key = vk_none;
    }

    // ending zero to item->scancode
    if (scode < item->scancode + sizeof(virtual_key_item::scancode) - 1)
        *(++scode) = 0;

    // fill ASCII field
    int ascii = esp::ps2::virtual_key_to_ascii(*item, _codepage);
    item->ascii = ascii > -1 ? ascii : 0;

    return item->vk != vk_none;
}


void keyboard::inject_virtual_key(virtual_key_item const &item, bool insert)
{
    // update m_VKMap
    if (item.down)
        _vk_map[(int)item.vk >> 3] |= 1 << ((int)item.vk & 7);
    else
        _vk_map[(int)item.vk >> 3] &= ~(1 << ((int)item.vk & 7));

    // has VK queue? Insert VK into it.
    if (_virtual_key_queue)
    {
        auto ticks_to_wait = portMAX_DELAY;
        if (insert)
            xQueueSendToFront(_virtual_key_queue, &item, ticks_to_wait);
        else
            xQueueSendToBack(_virtual_key_queue, &item, ticks_to_wait);
    }
}


void keyboard::inject_virtual_key(virtual_key virtual_key, bool key_down, bool insert)
{
    virtual_key_item item;
    item.vk = virtual_key;
    item.down = key_down;
    item.scancode[0] = 0; // this is a manual insert, not scancode associated
    item.ascii = virtual_key_to_ascii(virtual_key);
    item.ctrl = _ctrl;
    item.lalt = _lalt;
    item.ralt = _ralt;
    item.shift = _shift;
    item.gui = _gui;
    item.capslock = _capslock;
    item.numlock = _numlock;
    item.scrolllock = _scrolllock;
    inject_virtual_key(item, insert);
}


// inject a virtual key item into virtual key queue calling injectVirtualKey()
void keyboard::post_virtual_key_item(virtual_key_item const &item)
{
    // add into m_virtualKeyQueue and update m_VKMap
    inject_virtual_key(item, false);
}


// converts keypad virtual key to number (VK_KP_1 = 1, VK_KP_DOWN = 2, etc...)
// -1 = no convertible
int keyboard::conv_keypad_vk_to_num(virtual_key vk)
{
    switch (vk)
    {
        case vk_kp_0:
        case vk_kp_insert:
            return 0;
        case vk_kp_1:
        case vk_kp_end:
            return 1;
        case vk_kp_2:
        case vk_kp_down:
            return 2;
        case vk_kp_3:
        case vk_kp_pagedown:
            return 3;
        case vk_kp_4:
        case vk_kp_left:
            return 4;
        case vk_kp_5:
        case vk_kp_center:
            return 5;
        case vk_kp_6:
        case vk_kp_right:
            return 6;
        case vk_kp_7:
        case vk_kp_home:
            return 7;
        case vk_kp_8:
        case vk_kp_up:
            return 8;
        case vk_kp_9:
        case vk_kp_pageup:
            return 9;
        default:
            return -1;
    };
}


void keyboard::s_code_to_vk_converter_task(void *pv_parameters)
{
    keyboard *keyboard = (esp::ps2::keyboard *)pv_parameters;

    // manage ALT + Keypad num
    uint8_t altnum_value = 0; // current value (0 = no value, 0 is not allowed)

    while (true)
    {

        virtual_key_item item;

        if (keyboard->blocking_get_virtual_key(&item))
        {

            // onVirtualKey may set item.vk = VK_NONE!
            keyboard->on_virtual_key(&item.vk, item.down);

            if (item.vk != vk_none)
            {

                // manage left-ALT + NUM
                if (!is_alt(item.vk) && keyboard->_lalt)
                {
                    // ALT was down, is this a keypad number?
                    int num = conv_keypad_vk_to_num(item.vk);
                    if (num >= 0)
                    {
                        // yes this is a keypad num, if down update ALTNUMValue
                        if (item.down)
                            altnum_value = (altnum_value * 10 + num) & 0xff;
                    }
                    else
                    {
                        // no, back to normal case
                        altnum_value = 0;
                        keyboard->post_virtual_key_item(item);
                    }
                }
                else if (altnum_value > 0 && is_alt(item.vk) && !item.down)
                {
                    // ALT is up and ALTNUMValue contains a valid value, add it
                    keyboard->post_virtual_key_item(item); // post ALT up
                    item.vk = vk_ascii;
                    item.down = true;
                    item.scancode[0] = 0;
                    item.ascii = altnum_value;
                    keyboard->post_virtual_key_item(item); // ascii key down
                    item.down = false;
                    keyboard->post_virtual_key_item(item); // ascii key up
                    altnum_value = 0;
                }
                else
                {
                    // normal case
                    keyboard->post_virtual_key_item(item);
                }
            }
        }
    }
}


bool keyboard::is_vk_down(virtual_key virtual_key)
{
    bool r = _vk_map[(int)virtual_key >> 3] & (1 << ((int)virtual_key & 7));

    // VK_PAUSE is never released (no scancode sent from keyboard on key up), so when queried it is like released
    if (virtual_key == vk_pause)
        _vk_map[(int)virtual_key >> 3] &= ~(1 << ((int)virtual_key & 7));

    return r;
}


bool keyboard::get_next_virtual_key(virtual_key_item *item, int time_out_ms)
{
    bool r = (_s_code_to_vk_converter_task && item
              && xQueueReceive(_virtual_key_queue, item, esp::ms_to_ticks(time_out_ms)) == pdTRUE);
    if (r && _scancode_set == 1)
        convert_scancode2to_1(item);
    return r;
}


virtual_key keyboard::get_next_virtual_key(bool *key_down, int time_out_ms)
{
    virtual_key_item item;
    if (get_next_virtual_key(&item, time_out_ms))
    {
        if (key_down)
            *key_down = item.down;
        return item.vk;
    }
    return vk_none;
}


int keyboard::virtual_key_available()
{
    return _virtual_key_queue ? uxQueueMessagesWaiting(_virtual_key_queue) : 0;
}


void keyboard::empty_virtual_key_queue()
{
    xQueueReset(_virtual_key_queue);
}


void keyboard::convert_scancode2to_1(virtual_key_item *item)
{
    uint8_t *rpos = item->scancode;
    uint8_t *wpos = rpos;
    uint8_t *epos = rpos + sizeof(virtual_key_item::scancode);
    while (*rpos && rpos < epos)
    {
        if (*rpos == 0xf0)
        {
            ++rpos;
            *wpos++ = 0x80 | conv_scancode_set_2_to_1(*rpos++);
        }
        else
            *wpos++ = conv_scancode_set_2_to_1(*rpos++);
    }
    if (wpos < epos)
        *wpos = 0;
}


uint8_t keyboard::conv_scancode_set_2_to_1(uint8_t code)
{
    // 8042 scancodes set 2 to 1 translation table
    static const uint8_t S2TOS1[256] = {
        0xff, 0x43, 0x41, 0x3f, 0x3d, 0x3b, 0x3c, 0x58, 0x64, 0x44, 0x42, 0x40, 0x3e, 0x0f, 0x29,
        0x59, 0x65, 0x38, 0x2a, 0x70, 0x1d, 0x10, 0x02, 0x5a, 0x66, 0x71, 0x2c, 0x1f, 0x1e, 0x11,
        0x03, 0x5b, 0x67, 0x2e, 0x2d, 0x20, 0x12, 0x05, 0x04, 0x5c, 0x68, 0x39, 0x2f, 0x21, 0x14,
        0x13, 0x06, 0x5d, 0x69, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, 0x5e, 0x6a, 0x72, 0x32, 0x24,
        0x16, 0x08, 0x09, 0x5f, 0x6b, 0x33, 0x25, 0x17, 0x18, 0x0b, 0x0a, 0x60, 0x6c, 0x34, 0x35,
        0x26, 0x27, 0x19, 0x0c, 0x61, 0x6d, 0x73, 0x28, 0x74, 0x1a, 0x0d, 0x62, 0x6e, 0x3a, 0x36,
        0x1c, 0x1b, 0x75, 0x2b, 0x63, 0x76, 0x55, 0x56, 0x77, 0x78, 0x79, 0x7a, 0x0e, 0x7b, 0x7c,
        0x4f, 0x7d, 0x4b, 0x47, 0x7e, 0x7f, 0x6f, 0x52, 0x53, 0x50, 0x4c, 0x4d, 0x48, 0x01, 0x45,
        0x57, 0x4e, 0x51, 0x4a, 0x37, 0x49, 0x46, 0x54, 0x80, 0x81, 0x82, 0x41, 0x54, 0x85, 0x86,
        0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
        0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
        0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2,
        0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
        0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0,
        0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe,
        0xff,
    };
    return S2TOS1[code];
}


} // end of namespace
