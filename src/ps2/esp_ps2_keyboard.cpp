// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_keyboard.h
 * @brief Объявление клавиатуры PS/2 esp::ps2::keyboard.
 */

/**
 * @file esp_ps2_keyboard.cpp
 * @brief Клавиатура PS/2: скан-коды, виртуальные клавиши, знаки ASCII.
 */

#include "esp_ps2_keyboard.h"

#include "esp_ps2_codepages.h"
#include "esp_ps2_keyboard_layouts.h"
#include "esp_ps2_ps2_controller.h"
#include "esp_ps2_ps2_device_lock.h"
#include "esp_tools.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


#pragma GCC optimize("O2")


namespace esp::ps2::p
{

/**
 * @brief Перевод скан-кодов набора 2 в набор 1, как у контроллера 8042.
 *
 * Индекс — скан-код набора 2, значение — скан-код набора 1.
 */
constexpr std::uint8_t SCANCODE_SET_2_TO_1[256] = {
    0xff, 0x43, 0x41, 0x3f, 0x3d, 0x3b, 0x3c, 0x58, 0x64, 0x44, 0x42, 0x40, 0x3e, 0x0f, 0x29, 0x59,
    0x65, 0x38, 0x2a, 0x70, 0x1d, 0x10, 0x02, 0x5a, 0x66, 0x71, 0x2c, 0x1f, 0x1e, 0x11, 0x03, 0x5b,
    0x67, 0x2e, 0x2d, 0x20, 0x12, 0x05, 0x04, 0x5c, 0x68, 0x39, 0x2f, 0x21, 0x14, 0x13, 0x06, 0x5d,
    0x69, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, 0x5e, 0x6a, 0x72, 0x32, 0x24, 0x16, 0x08, 0x09, 0x5f,
    0x6b, 0x33, 0x25, 0x17, 0x18, 0x0b, 0x0a, 0x60, 0x6c, 0x34, 0x35, 0x26, 0x27, 0x19, 0x0c, 0x61,
    0x6d, 0x73, 0x28, 0x74, 0x1a, 0x0d, 0x62, 0x6e, 0x3a, 0x36, 0x1c, 0x1b, 0x75, 0x2b, 0x63, 0x76,
    0x55, 0x56, 0x77, 0x78, 0x79, 0x7a, 0x0e, 0x7b, 0x7c, 0x4f, 0x7d, 0x4b, 0x47, 0x7e, 0x7f, 0x6f,
    0x52, 0x53, 0x50, 0x4c, 0x4d, 0x48, 0x01, 0x45, 0x57, 0x4e, 0x51, 0x4a, 0x37, 0x49, 0x46, 0x54,
    0x80, 0x81, 0x82, 0x41, 0x54, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

/** @brief Скан-код PAUSE после первого байта 0xE1: у этой клавиши нет отпускания. */
constexpr std::uint8_t PAUSE_SCANCODE[] = {0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};

#if ESP_PS2_VIRTUAL_KEY_NAMES
/** @brief Имена виртуальных клавиш в порядке esp::ps2::virtual_key. */
constexpr const char *VIRTUAL_KEY_NAMES[] = {
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
    "VK_GRAVE_i",
    "VK_GRAVE_o",
    "VK_GRAVE_u",
    "VK_GRAVE_y",
    "VK_ACUTE_a",
    "VK_ACUTE_e",
    "VK_ACUTE_i",
    "VK_ACUTE_o",
    "VK_ACUTE_u",
    "VK_ACUTE_y",
    "VK_GRAVE_A",
    "VK_GRAVE_E",
    "VK_GRAVE_I",
    "VK_GRAVE_O",
    "VK_GRAVE_U",
    "VK_GRAVE_Y",
    "VK_ACUTE_A",
    "VK_ACUTE_E",
    "VK_ACUTE_I",
    "VK_ACUTE_O",
    "VK_ACUTE_U",
    "VK_ACUTE_Y",
    "VK_UMLAUT_a",
    "VK_UMLAUT_e",
    "VK_UMLAUT_i",
    "VK_UMLAUT_o",
    "VK_UMLAUT_u",
    "VK_UMLAUT_y",
    "VK_UMLAUT_A",
    "VK_UMLAUT_E",
    "VK_UMLAUT_I",
    "VK_UMLAUT_O",
    "VK_UMLAUT_U",
    "VK_UMLAUT_Y",
    "VK_CARET_a",
    "VK_CARET_e",
    "VK_CARET_i",
    "VK_CARET_o",
    "VK_CARET_u",
    "VK_CARET_y",
    "VK_CARET_A",
    "VK_CARET_E",
    "VK_CARET_I",
    "VK_CARET_O",
    "VK_CARET_U",
    "VK_CARET_Y",
    "VK_CEDILLA_c",
    "VK_CEDILLA_C",
    "VK_TILDE_a",
    "VK_TILDE_o",
    "VK_TILDE_n",
    "VK_TILDE_A",
    "VK_TILDE_O",
    "VK_TILDE_N",
    "VK_UPPER_a",
    "VK_ESZETT",
    "VK_EXCLAIM_INV",
    "VK_QUESTION_INV",
    "VK_INTERPUNCT",
    "VK_DIAERESIS",
    "VK_SQUARE",
    "VK_CURRENCY",
    "VK_MU",
    "VK_aelig",
    "VK_oslash",
    "VK_aring",
    "VK_AELIG",
    "VK_OSLASH",
    "VK_ARING",
    "VK_YEN",
    "VK_MUHENKAN",
    "VK_HENKAN",
    "VK_KATAKANA_HIRAGANA_ROMAJI",
    "VK_HANKAKU_ZENKAKU_KANJI",
    "VK_SHIFT_0",
    "VK_ASCII",
};

static_assert(std::size(VIRTUAL_KEY_NAMES)
                  == static_cast<std::size_t>(esp::ps2::virtual_key::vk_last),
              "every virtual key needs a name");
#endif

}


/**
 * @class esp::ps2::keyboard
 * @brief Клавиатура PS/2: скан-коды, виртуальные клавиши, знаки ASCII.
 *
 * Клавиатура подключена к одному из портов контроллера
 * esp::ps2::ps2_controller и переводит скан-коды в виртуальные клавиши и
 * знаки ASCII. По желанию создаётся задача, которая ждёт скан-коды и
 * ставит виртуальные клавиши в очередь; нажата ли клавиша, сообщает
 * is_vk_down().
 *
 * Раскладка по умолчанию — US; готовые раскладки объявлены в
 * esp_ps2_keyboard_layouts.h, новую можно унаследовать от любой из них.
 * Обычно клавиатуру создаёт контроллер PS/2 в
 * esp::ps2::ps2_controller::begin():
 * @code
 * esp::ps2::keyboard *keyboard = esp::ps2::ps2_controller::keyboard();
 * esp::ps2::virtual_key_item item;
 * if (keyboard->get_next_virtual_key(&item))
 *     printf("%s %s\n", esp::ps2::keyboard::virtual_key_to_string(item.vk),
 *            item.down ? "down" : "up");
 * @endcode
 */


int esp::ps2::keyboard::scancode_to_virtual_key_task_stack_size =
    esp::ps2::SCANCODE_TASK_STACK_SIZE;


// Открытые

/**
 * @brief Создаёт клавиатуру без порта: набор скан-кодов 2, без задачи и очереди.
 */
esp::ps2::keyboard::keyboard()
    : _keyboard_available{false}
    , _scancode_to_vk_task{nullptr}
    , _virtual_key_queue{nullptr}
    , _scancode_set{2}
    , _last_dead_key{virtual_key::vk_none}
    , _codepage{nullptr}
{
}

/**
 * @brief Удаляет задачу и очередь виртуальных клавиш.
 */
esp::ps2::keyboard::~keyboard()
{
    enable_virtual_keys(false, false);
}

/**
 * @brief Запускает клавиатуру на выводах GPIO и сбрасывает её.
 *
 * Заодно настраивает контроллер PS/2 на один порт 0. Из-за сопроцессора
 * ULP годятся только выводы, доступные ему: GPIO2, 4, 12, 13, 14, 15, 25,
 * 26, 27, 32 и 33.
 *
 * @param clk_gpio Вывод линии CLK.
 * @param data_gpio Вывод линии DATA.
 * @param generate_virtual_keys @c true — создать задачу, которая переводит
 *        скан-коды в виртуальные клавиши: тогда работает is_vk_down().
 * @param create_vk_queue @c true — ещё и ставить виртуальные клавиши в
 *        очередь: тогда работают virtual_key_available() и
 *        get_next_virtual_key().
 */
void esp::ps2::keyboard::begin(gpio_num_t clk_gpio, gpio_num_t data_gpio,
                               bool generate_virtual_keys, bool create_vk_queue)
{
    esp::ps2::ps2_controller::begin(clk_gpio, data_gpio);
    esp::ps2::ps2_controller::set_keyboard(this);
    begin(generate_virtual_keys, create_vk_queue, 0);
}

/**
 * @brief Запускает клавиатуру на порту уже настроенного контроллера PS/2 и сбрасывает её.
 * @param generate_virtual_keys @c true — создать задачу, которая переводит
 *        скан-коды в виртуальные клавиши: тогда работает is_vk_down().
 * @param create_vk_queue @c true — ещё и ставить виртуальные клавиши в
 *        очередь: тогда работают virtual_key_available() и
 *        get_next_virtual_key().
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::keyboard::begin(bool generate_virtual_keys, bool create_vk_queue, int ps2_port)
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

/**
 * @brief Включает или выключает получение виртуальных клавиш и их очередь.
 *
 * Можно вызывать после begin(). Очереди нужна задача, поэтому
 * @p create_vk_queue = @c true включает и её.
 *
 * @param generate_virtual_keys @c true — создать задачу перевода
 *        скан-кодов, @c false — удалить её.
 * @param create_vk_queue @c true — создать очередь виртуальных клавиш,
 *        @c false — удалить её.
 */
void esp::ps2::keyboard::enable_virtual_keys(bool generate_virtual_keys, bool create_vk_queue)
{
    const esp::ps2::ps2_device_lock lock{this};

    if (create_vk_queue)
        generate_virtual_keys = true;

    // Создаём очередь и задачу, если их ещё нет.

    if (!_virtual_key_queue && create_vk_queue)
    {
        _virtual_key_queue =
            xQueueCreate(esp::ps2::VIRTUAL_KEY_QUEUE_SIZE, sizeof(esp::ps2::virtual_key_item));
    }

    if (!_scancode_to_vk_task && generate_virtual_keys)
    {
        xTaskCreate(&scancode_to_vk_task, "", scancode_to_virtual_key_task_stack_size, this,
                    esp::ps2::SCANCODE_TASK_PRIORITY, &_scancode_to_vk_task);
    }

    // Удаляем в обратном порядке.

    if (_scancode_to_vk_task && !generate_virtual_keys)
    {
        vTaskDelete(_scancode_to_vk_task);
        _scancode_to_vk_task = nullptr;
    }

    if (_virtual_key_queue && !create_vk_queue)
    {
        vQueueDelete(_virtual_key_queue);
        _virtual_key_queue = nullptr;
    }
}

/**
 * @brief Сбрасывает клавиатуру, выбирает набор скан-кодов 2 и раскладку US.
 *
 * Сброс повторяется до трёх раз.
 *
 * @return @c true, если клавиатура прошла самопроверку.
 */
bool esp::ps2::keyboard::reset()
{
    std::memset(_vk_map, 0, sizeof(_vk_map));

    set_layout(&esp::ps2::US_LAYOUT);

    // Клавиатуре нужно 350 мс после включения питания (NXP M68HC08,
    // Designer Reference Manual).
    vTaskDelay(350 / portTICK_PERIOD_MS);

    // До трёх попыток сброса.
    for (int i = 0; i < 3; ++i)
    {
        _keyboard_available = send_cmd_reset();
        if (_keyboard_available)
            break;
        vTaskDelay(350 / portTICK_PERIOD_MS);
    }
    // Даём клавиатуре время закончить самопроверку.
    vTaskDelay(200 / portTICK_PERIOD_MS);

    send_cmd_set_scancode_set(2);

    return _keyboard_available;
}

/**
 * @brief Задаёт раскладку.
 * @param layout Раскладка, например &esp::ps2::GERMAN_LAYOUT.
 */
void esp::ps2::keyboard::set_layout(const keyboard_layout *layout)
{
    _layout = layout;
}

/**
 * @brief Сообщает, нажата ли виртуальная клавиша.
 *
 * Работает, только если виртуальные клавиши получаются (параметр
 * @c generate_virtual_keys в begin()). PAUSE не присылает отпускания,
 * поэтому после запроса она считается отпущенной.
 *
 * @param vk Виртуальная клавиша.
 * @return @c true, если клавиша нажата.
 */
bool esp::ps2::keyboard::is_vk_down(virtual_key vk)
{
    const int index = static_cast<int>(vk);
    const bool r = _vk_map[index >> 3] & (1 << (index & 7));

    // У PAUSE нет скан-кода отпускания: после запроса считаем её отпущенной.
    if (vk == virtual_key::vk_pause)
        _vk_map[index >> 3] &= ~(1 << (index & 7));

    return r;
}

/**
 * @brief Возвращает число виртуальных клавиш в очереди.
 * @return Число клавиш; 0, если очереди нет.
 */
int esp::ps2::keyboard::virtual_key_available() const
{
    return _virtual_key_queue ? uxQueueMessagesWaiting(_virtual_key_queue) : 0;
}

/**
 * @brief Берёт виртуальную клавишу из очереди.
 * @param key_down Куда записать, нажата клавиша (@c true) или отпущена;
 *        может быть @c nullptr.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return Виртуальная клавиша; @c virtual_key::vk_none, если время вышло.
 */
esp::ps2::virtual_key esp::ps2::keyboard::get_next_virtual_key(bool *key_down, int time_out_ms)
{
    esp::ps2::virtual_key_item item;
    if (get_next_virtual_key(&item, time_out_ms))
    {
        if (key_down)
            *key_down = item.down;
        return item.vk;
    }
    return virtual_key::vk_none;
}

/**
 * @brief Берёт из очереди событие клавиатуры вместе со скан-кодом.
 *
 * Если выбран набор скан-кодов 1, скан-код события переводится в него.
 *
 * @param item Куда записать событие.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return @c true, если событие получено.
 */
bool esp::ps2::keyboard::get_next_virtual_key(virtual_key_item *item, int time_out_ms)
{
    const bool r =
        (_virtual_key_queue && item
         && xQueueReceive(_virtual_key_queue, item, esp::ms_to_ticks(time_out_ms)) == pdTRUE);
    if (r && _scancode_set == 1)
        convert_scancode_2_to_1(item);
    return r;
}

/**
 * @brief Ставит в очередь виртуальную клавишу, как будто её нажали или отпустили.
 *
 * Модификаторы у события текущие, скан-кода нет.
 *
 * @param vk Виртуальная клавиша.
 * @param key_down @c true — клавиша нажата, @c false — отпущена.
 * @param insert @c true — поставить в начало очереди, @c false — в конец.
 */
void esp::ps2::keyboard::inject_virtual_key(virtual_key vk, bool key_down, bool insert)
{
    esp::ps2::virtual_key_item item;
    item.vk = vk;
    item.down = key_down;
    item.scancode[0] = 0; // Клавиша не с клавиатуры: скан-кода нет.
    item.ascii = virtual_key_to_ascii(vk);
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

/**
 * @brief Ставит в очередь событие клавиатуры и запоминает, нажата ли клавиша.
 *
 * Если очередь полна, ждёт, пока освободится место.
 *
 * @param item Событие.
 * @param insert @c true — поставить в начало очереди, @c false — в конец.
 */
void esp::ps2::keyboard::inject_virtual_key(const virtual_key_item &item, bool insert)
{
    const int index = static_cast<int>(item.vk);
    if (item.down)
        _vk_map[index >> 3] |= 1 << (index & 7);
    else
        _vk_map[index >> 3] &= ~(1 << (index & 7));

    if (_virtual_key_queue)
    {
        if (insert)
            xQueueSendToFront(_virtual_key_queue, &item, portMAX_DELAY);
        else
            xQueueSendToBack(_virtual_key_queue, &item, portMAX_DELAY);
    }
}

/**
 * @brief Очищает очередь виртуальных клавиш.
 */
void esp::ps2::keyboard::empty_virtual_key_queue()
{
    if (_virtual_key_queue)
        xQueueReset(_virtual_key_queue);
}

/**
 * @brief Переводит виртуальную клавишу в знак ASCII с учётом текущих модификаторов.
 *
 * Например, @c virtual_key::vk_A даёт «A» (0x41), CTRL + пробел — NUL
 * (0x00), CTRL + буква — управляющие знаки от SOH (0x01) до SUB (0x1A),
 * CTRL + «\» — FS (0x1C), CTRL + «?» — US (0x1F), CTRL + «[» — ESC
 * (0x1B), CTRL + «]» — GS (0x1D), CTRL + «~» — RS (0x1E), а SCROLL LOCK —
 * XON или XOFF. Знаки за пределами ASCII берутся из кодовой страницы
 * (set_code_page()).
 *
 * @param vk Виртуальная клавиша.
 * @return Код знака или −1, если клавиша в ASCII не переводится.
 */
int esp::ps2::keyboard::virtual_key_to_ascii(virtual_key vk) const
{
    esp::ps2::virtual_key_item item;
    item.vk = vk;
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

/**
 * @brief Сообщает, есть ли непрочитанный скан-код.
 *
 * Скан-коды читает и задача виртуальных клавиш, поэтому читать их самому
 * имеет смысл, только если её нет (@c generate_virtual_keys = @c false в
 * begin()).
 *
 * @return 1, если скан-код есть, иначе 0.
 */
int esp::ps2::keyboard::scancode_available()
{
    return data_available();
}

/**
 * @brief Получает скан-код.
 *
 * Если линия CLK перестала отвечать, клавиатуре посылается команда
 * включить сканирование — так снимается зависание.
 *
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @param request_resend_on_time_out @c true — если время вышло, попросить
 *        клавиатуру повторить байт и ждать снова.
 * @return Скан-код или −1, если время вышло.
 */
int esp::ps2::keyboard::get_next_scancode(int time_out_ms, bool request_resend_on_time_out)
{
    while (true)
    {
        const int r = get_data(time_out_ms);
        if (r == -1 && clk_time_out_error())
        {
            // Клавиатура зависла: включаем сканирование заново.
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

/**
 * @brief Зажигает или гасит индикаторы клавиатуры.
 * @param num_lock @c true — зажечь NUM LOCK.
 * @param caps_lock @c true — зажечь CAPS LOCK.
 * @param scroll_lock @c true — зажечь SCROLL LOCK.
 * @return @c true, если клавиатура приняла команду.
 */
bool esp::ps2::keyboard::set_leds(bool num_lock, bool caps_lock, bool scroll_lock)
{
    _num_lock_led = num_lock;
    _caps_lock_led = caps_lock;
    _scroll_lock_led = scroll_lock;
    return send_cmd_leds(num_lock, caps_lock, scroll_lock);
}

/**
 * @brief Сообщает, какие индикаторы клавиатуры горят.
 * @param num_lock Куда записать, горит ли NUM LOCK.
 * @param caps_lock Куда записать, горит ли CAPS LOCK.
 * @param scroll_lock Куда записать, горит ли SCROLL LOCK.
 */
void esp::ps2::keyboard::get_leds(bool *num_lock, bool *caps_lock, bool *scroll_lock) const
{
    *num_lock = _num_lock_led;
    *caps_lock = _caps_lock_led;
    *scroll_lock = _scroll_lock_led;
}

/**
 * @brief Задаёт набор скан-кодов.
 *
 * Набор 1 — от IBM PC XT, 2 — от IBM PC AT, 3 — от терминалов IBM 3270.
 * Пока получаются виртуальные клавиши, доступны только наборы 1 и 2:
 * клавиатура остаётся в наборе 2, а скан-коды событий переводятся в
 * набор 1 при чтении из очереди.
 *
 * @param value Набор: 1, 2 или 3.
 * @return @c true, если набор выбран.
 */
bool esp::ps2::keyboard::set_scancode_set(int value)
{
    if (_scancode_to_vk_task)
    {
        // Виртуальные клавиши получаются: допустимы только наборы 1 и 2.
        if (value == 1 || value == 2)
        {
            _scancode_set = value;
            return true;
        }
    }
    else
    {
        // Виртуальных клавиш нет: просим клавиатуру переключить набор.
        if (send_cmd_set_scancode_set(value))
        {
            _scancode_set = value;
            return true;
        }
    }
    return false;
}

/**
 * @brief Переводит скан-код из набора 2 в набор 1, как контроллер 8042.
 * @param code Скан-код набора 2.
 * @return Скан-код набора 1.
 */
std::uint8_t esp::ps2::keyboard::conv_scancode_set_2_to_1(std::uint8_t code)
{
    return esp::ps2::p::SCANCODE_SET_2_TO_1[code];
}

#if ESP_PS2_VIRTUAL_KEY_NAMES
/**
 * @brief Возвращает имя виртуальной клавиши, например «VK_A».
 *
 * Есть, только если ESP_PS2_VIRTUAL_KEY_NAMES не 0.
 *
 * @param vk Виртуальная клавиша.
 * @return Имя клавиши.
 */
const char *esp::ps2::keyboard::virtual_key_to_string(virtual_key vk)
{
    return esp::ps2::p::VIRTUAL_KEY_NAMES[static_cast<int>(vk)];
}
#endif

// Закрытые

/**
 * @brief Находит виртуальную клавишу скан-кода в раскладке.
 *
 * Если в раскладке скан-кода нет, он ищется в унаследованной. Клавиши
 * цифровой панели при включённом NUM LOCK и отпущенном SHIFT становятся
 * цифрами.
 *
 * @param scancode Скан-код без префиксов 0xE0 и 0xF0.
 * @param is_extended @c true — у скан-кода был префикс 0xE0.
 * @param layout Раскладка; @c nullptr — текущая.
 * @return Виртуальная клавиша; @c virtual_key::vk_none, если её нет.
 */
esp::ps2::virtual_key esp::ps2::keyboard::scancode_to_vk(std::uint8_t scancode, bool is_extended,
                                                         const keyboard_layout *layout)
{
    virtual_key vk = virtual_key::vk_none;

    if (layout == nullptr)
        layout = _layout;

    const esp::ps2::virtual_key_def *def =
        is_extended ? layout->ex_scancode_to_vk : layout->scancode_to_vk;
    for (; def->scancode; ++def)
    {
        if (def->scancode == scancode)
        {
            vk = def->virtual_key;
            break;
        }
    }

    if (vk == virtual_key::vk_none && layout->inherited)
        vk = scancode_to_vk(scancode, is_extended, layout->inherited);

    // Цифровая панель: при включённом NUM LOCK и отпущенном SHIFT — цифры,
    // иначе (NUM LOCK выключен или SHIFT нажат) — клавиши управления курсором.
    if (_numlock && !_shift)
    {
        switch (vk)
        {
            case virtual_key::vk_kp_delete:
                vk = virtual_key::vk_kp_period;
                break;
            case virtual_key::vk_kp_insert:
                vk = virtual_key::vk_kp_0;
                break;
            case virtual_key::vk_kp_end:
                vk = virtual_key::vk_kp_1;
                break;
            case virtual_key::vk_kp_down:
                vk = virtual_key::vk_kp_2;
                break;
            case virtual_key::vk_kp_pagedown:
                vk = virtual_key::vk_kp_3;
                break;
            case virtual_key::vk_kp_left:
                vk = virtual_key::vk_kp_4;
                break;
            case virtual_key::vk_kp_center:
                vk = virtual_key::vk_kp_5;
                break;
            case virtual_key::vk_kp_right:
                vk = virtual_key::vk_kp_6;
                break;
            case virtual_key::vk_kp_home:
                vk = virtual_key::vk_kp_7;
                break;
            case virtual_key::vk_kp_up:
                vk = virtual_key::vk_kp_8;
                break;
            case virtual_key::vk_kp_pageup:
                vk = virtual_key::vk_kp_9;
                break;
            default:
                break;
        }
    }

    return vk;
}

/**
 * @brief Заменяет клавишу той, что получается с текущими модификаторами.
 *
 * Например, SHIFT + «1» даёт «!». Отпускание клавиши получает ту же
 * замену, что и нажатие, даже если модификатор отпустили раньше.
 *
 * @param in_vk Клавиша без модификаторов.
 * @param down @c true — клавиша нажата.
 * @param layout Раскладка; @c nullptr — текущая.
 * @return Заменённая клавиша или @p in_vk, если замены нет.
 */
esp::ps2::virtual_key esp::ps2::keyboard::vk_to_alternate_vk(virtual_key in_vk, bool down,
                                                             const keyboard_layout *layout)
{
    virtual_key vk = virtual_key::vk_none;

    if (layout == nullptr)
        layout = _layout;

    // Клавиша нажата без замены, а SHIFT нажали уже после неё: отпускаем её же.
    if (!down && is_vk_down(in_vk))
        vk = in_vk;

    if (vk == virtual_key::vk_none)
    {
        // Отпускание клавиши, нажатой с заменой, когда SHIFT отпустили раньше
        // неё (SHIFT + «1» дало «!»): отпускаем замену, иначе она так и
        // останется нажатой.
        for (const esp::ps2::alt_virtual_key_def *def = layout->alternate_vk;
             def->req_virtual_key != virtual_key::vk_none; ++def)
        {
            if (def->req_virtual_key == in_vk && is_vk_down(def->virtual_key))
            {
                vk = def->virtual_key;
                break;
            }
        }
    }

    if (vk == virtual_key::vk_none)
    {
        // Нажатие при уже нажатых модификаторах или отпускание при ещё
        // нажатых.
        for (const esp::ps2::alt_virtual_key_def *def = layout->alternate_vk;
             def->req_virtual_key != virtual_key::vk_none; ++def)
        {
            if (def->req_virtual_key == in_vk && def->ctrl == _ctrl && def->lalt == _lalt
                && def->ralt == _ralt && def->shift == _shift)
            {
                vk = def->virtual_key;
                break;
            }
        }
    }

    if (vk == virtual_key::vk_none && layout->inherited)
        vk = vk_to_alternate_vk(in_vk, down, layout->inherited);

    return vk == virtual_key::vk_none ? in_vk : vk;
}

/**
 * @brief Меняет регистр буквы, если включён CAPS LOCK.
 * @param vk Виртуальная клавиша.
 * @return Клавиша буквы другого регистра или @p vk.
 */
esp::ps2::virtual_key esp::ps2::keyboard::manage_capslock(virtual_key vk) const
{
    if (_capslock)
    {
        // Буквы в перечислении идут подряд: меняем регистр сдвигом.
        constexpr int CASE_SHIFT =
            static_cast<int>(virtual_key::vk_A) - static_cast<int>(virtual_key::vk_a);
        if (vk >= virtual_key::vk_a && vk <= virtual_key::vk_z)
            vk = static_cast<esp::ps2::virtual_key>(static_cast<int>(vk) + CASE_SHIFT);
        else if (vk >= virtual_key::vk_A && vk <= virtual_key::vk_Z)
            vk = static_cast<esp::ps2::virtual_key>(static_cast<int>(vk) - CASE_SHIFT);
    }
    return vk;
}

/**
 * @brief Зажигает индикаторы по состоянию NUM LOCK, CAPS LOCK и SCROLL LOCK.
 */
void esp::ps2::keyboard::update_leds()
{
    send_cmd_leds(_numlock, _capslock, _scrolllock);
    _num_lock_led = _numlock;
    _caps_lock_led = _capslock;
    _scroll_lock_led = _scrolllock;
}

/**
 * @brief Ждёт скан-код и переводит его в событие клавиатуры.
 *
 * Учитывает CAPS LOCK, модификаторы и мёртвые клавиши; обновляет
 * состояние модификаторов и индикаторов.
 *
 * @param item Куда записать событие.
 * @return @c true, если получилась виртуальная клавиша.
 */
bool esp::ps2::keyboard::blocking_get_virtual_key(virtual_key_item *item)
{
    item->vk = virtual_key::vk_none;
    item->down = true;
    item->ctrl = _ctrl;
    item->lalt = _lalt;
    item->ralt = _ralt;
    item->shift = _shift;
    item->gui = _gui;
    item->capslock = _capslock;
    item->numlock = _numlock;
    item->scrolllock = _scrolllock;

    std::uint8_t *scode = item->scancode;

    *scode = get_next_scancode();
    if (*scode == 0xE0)
    {
        // Расширенный скан-код.
        *(++scode) = get_next_scancode(100, true);
        if (*scode == 0xF0)
        {
            // Отпускание.
            *(++scode) = get_next_scancode(100, true);
            item->vk = scancode_to_vk(*scode, true);
            item->down = false;
        }
        else
        {
            // Нажатие.
            item->vk = scancode_to_vk(*scode, true);
        }
    }
    else if (*scode == 0xE1)
    {
        // PAUSE: 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77.
        for (int i = 0; i < sizeof(esp::ps2::p::PAUSE_SCANCODE); ++i)
        {
            *(++scode) = get_next_scancode(100, true);
            if (*scode != esp::ps2::p::PAUSE_SCANCODE[i])
                break;
            else if (i == sizeof(esp::ps2::p::PAUSE_SCANCODE) - 1)
                item->vk = virtual_key::vk_pause;
        }
    }
    else if (*scode == 0xF0)
    {
        // Отпускание.
        *(++scode) = get_next_scancode(100, true);
        item->vk = scancode_to_vk(*scode, false);
        item->down = false;
    }
    else
    {
        // Нажатие.
        item->vk = scancode_to_vk(*scode, false);
    }

    if (item->vk != virtual_key::vk_none)
    {
        item->vk = manage_capslock(item->vk);

        // Замена клавиши с учётом SHIFT, ALT и других модификаторов.
        item->vk = vk_to_alternate_vk(item->vk, item->down);

        // Состояние модификаторов и переключателей, индикаторы.
        switch (item->vk)
        {
            case virtual_key::vk_lctrl:
            case virtual_key::vk_rctrl:
                _ctrl = item->down;
                break;
            case virtual_key::vk_lalt:
                _lalt = item->down;
                break;
            case virtual_key::vk_ralt:
                _ralt = item->down;
                break;
            case virtual_key::vk_lshift:
            case virtual_key::vk_rshift:
                _shift = item->down;
                break;
            case virtual_key::vk_lgui:
            case virtual_key::vk_rgui:
                _gui = item->down;
                break;
            case virtual_key::vk_capslock:
                if (!item->down)
                {
                    _capslock = !_capslock;
                    update_leds();
                }
                break;
            case virtual_key::vk_numlock:
                if (!item->down)
                {
                    _numlock = !_numlock;
                    update_leds();
                }
                break;
            case virtual_key::vk_scrolllock:
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

    // Мёртвые клавиши (реализовал Carles Oriol, https://github.com/carlesoriol).
    for (const esp::ps2::virtual_key *dk = _layout->dead_keys_vk; *dk != virtual_key::vk_none; ++dk)
    {
        if (item->vk == *dk)
        {
            _last_dead_key = item->vk;
            item->vk = virtual_key::vk_none;
        }
    }
    if (item->vk != _last_dead_key && item->vk != virtual_key::vk_none)
    {
        for (const esp::ps2::dead_key_virtual_key_def *dk = _layout->deadkeys_to_vk;
             dk->dead_key != virtual_key::vk_none; ++dk)
        {
            if (item->vk == dk->req_virtual_key && _last_dead_key == dk->dead_key)
            {
                item->vk = dk->virtual_key;
                break;
            }
        }
        if (!item->down && (item->vk != _last_dead_key) && (item->vk != virtual_key::vk_rshift)
            && (item->vk != virtual_key::vk_lshift))
        {
            _last_dead_key = virtual_key::vk_none;
        }
    }

    // Завершающий 0 скан-кода, если есть место.
    if (scode < item->scancode + sizeof(esp::ps2::virtual_key_item::scancode) - 1)
        *(++scode) = 0;

    const int ascii = esp::ps2::virtual_key_to_ascii(*item, _codepage);
    item->ascii = ascii > -1 ? ascii : 0;

    return item->vk != virtual_key::vk_none;
}

/**
 * @brief Переводит скан-код события из набора 2 в набор 1.
 *
 * Префикс отпускания 0xF0 набора 2 становится старшим разрядом кода.
 *
 * @param item Событие.
 */
void esp::ps2::keyboard::convert_scancode_2_to_1(virtual_key_item *item)
{
    std::uint8_t *rpos = item->scancode;
    std::uint8_t *wpos = rpos;
    std::uint8_t *const epos = rpos + sizeof(esp::ps2::virtual_key_item::scancode);
    while (rpos < epos && *rpos)
    {
        if (*rpos == 0xF0)
        {
            // Отпускание: в наборе 1 вместо префикса — старший разряд кода.
            if (++rpos == epos)
                break;
            *wpos++ = 0x80 | conv_scancode_set_2_to_1(*rpos++);
        }
        else
            *wpos++ = conv_scancode_set_2_to_1(*rpos++);
    }
    if (wpos < epos)
        *wpos = 0;
}

/**
 * @brief Ставит событие клавиатуры в конец очереди (см. inject_virtual_key()).
 * @param item Событие.
 */
void esp::ps2::keyboard::post_virtual_key_item(const virtual_key_item &item)
{
    inject_virtual_key(item, false);
}

/**
 * @brief Переводит клавишу цифровой панели в цифру.
 * @param vk Клавиша, например @c virtual_key::vk_kp_1 или @c virtual_key::vk_kp_end.
 * @return Цифра от 0 до 9 или −1, если это не клавиша цифровой панели.
 */
int esp::ps2::keyboard::conv_keypad_vk_to_num(virtual_key vk)
{
    switch (vk)
    {
        case virtual_key::vk_kp_0:
        case virtual_key::vk_kp_insert:
            return 0;
        case virtual_key::vk_kp_1:
        case virtual_key::vk_kp_end:
            return 1;
        case virtual_key::vk_kp_2:
        case virtual_key::vk_kp_down:
            return 2;
        case virtual_key::vk_kp_3:
        case virtual_key::vk_kp_pagedown:
            return 3;
        case virtual_key::vk_kp_4:
        case virtual_key::vk_kp_left:
            return 4;
        case virtual_key::vk_kp_5:
        case virtual_key::vk_kp_center:
            return 5;
        case virtual_key::vk_kp_6:
        case virtual_key::vk_kp_right:
            return 6;
        case virtual_key::vk_kp_7:
        case virtual_key::vk_kp_home:
            return 7;
        case virtual_key::vk_kp_8:
        case virtual_key::vk_kp_up:
            return 8;
        case virtual_key::vk_kp_9:
        case virtual_key::vk_kp_pageup:
            return 9;
        default:
            return -1;
    }
}

/**
 * @brief Задача клавиатуры: переводит скан-коды в виртуальные клавиши.
 *
 * Вызывает on_virtual_key и ставит клавиши в очередь. Цифры, набранные на
 * цифровой панели при нажатом левом ALT, после отпускания ALT дают знак с
 * этим кодом (виртуальная клавиша @c virtual_key::vk_ascii).
 *
 * @param arg Клавиатура.
 */
void esp::ps2::keyboard::scancode_to_vk_task(void *arg)
{
    auto *const self = static_cast<esp::ps2::keyboard *>(arg);

    // Код знака, набираемый цифрами при нажатом левом ALT; 0 — кода нет
    // (сам код 0 так не набрать).
    std::uint8_t altnum_value = 0;

    while (true)
    {
        esp::ps2::virtual_key_item item;

        if (self->blocking_get_virtual_key(&item))
        {
            // Обработчик может заменить клавишу на vk_none.
            self->on_virtual_key(&item.vk, item.down);

            if (item.vk != virtual_key::vk_none)
            {
                if (!esp::ps2::is_alt(item.vk) && self->_lalt)
                {
                    // Левый ALT нажат: цифра ли это цифровой панели?
                    const int num = conv_keypad_vk_to_num(item.vk);
                    if (num >= 0)
                    {
                        // Цифра: дописываем её к коду при нажатии.
                        if (item.down)
                            altnum_value = (altnum_value * 10 + num) & 0xff;
                    }
                    else
                    {
                        // Не цифра: обычная клавиша.
                        altnum_value = 0;
                        self->post_virtual_key_item(item);
                    }
                }
                else if (altnum_value > 0 && esp::ps2::is_alt(item.vk) && !item.down)
                {
                    // ALT отпущен, код набран: после отпускания ALT ставим
                    // нажатие и отпускание vk_ascii с этим кодом.
                    self->post_virtual_key_item(item);
                    item.vk = virtual_key::vk_ascii;
                    item.down = true;
                    item.scancode[0] = 0;
                    item.ascii = altnum_value;
                    self->post_virtual_key_item(item);
                    item.down = false;
                    self->post_virtual_key_item(item);
                    altnum_value = 0;
                }
                else
                {
                    self->post_virtual_key_item(item);
                }
            }
        }
    }
}
