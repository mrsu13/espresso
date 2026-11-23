// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_ps2_code_page.h"
#include "esp_ps2_config.h"
#include "esp_ps2_delegate.h"
#include "esp_ps2_keyboard_layout.h"
#include "esp_ps2_ps2_device.h"
#include "esp_ps2_virtual_key.h"
#include "esp_ps2_virtual_key_item.h"

#include <cstdint>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


namespace esp::ps2
{

class keyboard : public ps2_device
{

public:

    keyboard();
    ~keyboard();

    void begin(gpio_num_t clk_gpio, gpio_num_t data_gpio, bool generate_virtual_keys = true,
               bool create_vk_queue = true);
    void begin(bool generate_virtual_keys, bool create_vk_queue, int ps2_port);
    void enable_virtual_keys(bool generate_virtual_keys, bool create_vk_queue);
    bool reset();
    inline bool is_keyboard_available() const;

    void set_layout(const keyboard_layout *layout);
    inline const keyboard_layout *layout() const;

    bool is_vk_down(virtual_key vk);
    int virtual_key_available() const;
    virtual_key get_next_virtual_key(bool *key_down = nullptr, int time_out_ms = -1);
    bool get_next_virtual_key(virtual_key_item *item, int time_out_ms = -1);
    void inject_virtual_key(virtual_key vk, bool key_down, bool insert = false);
    void inject_virtual_key(const virtual_key_item &item, bool insert = false);
    void empty_virtual_key_queue();

    int virtual_key_to_ascii(virtual_key vk) const;
    inline void set_code_page(const code_page *codepage);

    int scancode_available();
    int get_next_scancode(int time_out_ms = -1, bool request_resend_on_time_out = false);

    bool set_leds(bool num_lock, bool caps_lock, bool scroll_lock);
    void get_leds(bool *num_lock, bool *caps_lock, bool *scroll_lock) const;
    inline bool set_typematic_rate_and_delay(int repeat_rate_ms, int repeat_delay_ms);

    bool set_scancode_set(int value);
    inline int scancode_set() const;

    static std::uint8_t conv_scancode_set_2_to_1(std::uint8_t code);

#if ESP_PS2_VIRTUAL_KEY_NAMES
    static const char *virtual_key_to_string(virtual_key vk);
#endif

    /**
     * @brief Обработчик каждой виртуальной клавиши, полученной из скан-кодов.
     *
     * Вызывается задачей клавиатуры до того, как клавиша попадёт в очередь.
     * Первый параметр — указатель на клавишу: обработчик может её заменить,
     * а заменив на @c virtual_key::vk_none, — отбросить. Второй — нажата ли
     * клавиша (@c true) или отпущена.
     */
    delegate<virtual_key *, bool> on_virtual_key;

    /**
     * @brief Размер стека задачи, переводящей скан-коды в виртуальные клавиши, байты.
     *
     * Менять до begin(); по умолчанию esp::ps2::SCANCODE_TASK_STACK_SIZE.
     */
    static int scancode_to_virtual_key_task_stack_size;

private:

    virtual_key scancode_to_vk(std::uint8_t scancode, bool is_extended,
                               const keyboard_layout *layout = nullptr);
    virtual_key vk_to_alternate_vk(virtual_key in_vk, bool down,
                                   const keyboard_layout *layout = nullptr);
    virtual_key manage_capslock(virtual_key vk) const;
    void update_leds();
    bool blocking_get_virtual_key(virtual_key_item *item);
    void convert_scancode_2_to_1(virtual_key_item *item);
    void post_virtual_key_item(const virtual_key_item &item);
    static int conv_keypad_vk_to_num(virtual_key vk);

    static void scancode_to_vk_task(void *arg);

    /** @brief Прошла ли клавиатура самопроверку при последнем сбросе. */
    bool _keyboard_available;

    /** @brief Задача, переводящая скан-коды в виртуальные клавиши; @c nullptr, если её нет. */
    TaskHandle_t _scancode_to_vk_task;

    /** @brief Очередь виртуальных клавиш; @c nullptr, если её нет. */
    QueueHandle_t _virtual_key_queue;

    /** @brief Нажатые виртуальные клавиши: по разряду на клавишу. */
    std::uint8_t _vk_map[(static_cast<int>(virtual_key::vk_last) + 7) / 8];

    /**
     * @brief Набор скан-кодов: 1, 2 или 3.
     *
     * Пока получаются виртуальные клавиши, допустимы только 1 и 2:
     * клавиатура работает в наборе 2, а набор 1 получается переводом из
     * него (convert_scancode_2_to_1()).
     */
    std::uint8_t _scancode_set;

    /** @brief Текущая раскладка. */
    const keyboard_layout *_layout;

    /** @brief Нажат ли CTRL. */
    bool _ctrl;

    /** @brief Нажат ли левый ALT. */
    bool _lalt;

    /** @brief Нажат ли правый ALT (ALTGR). */
    bool _ralt;

    /** @brief Нажат ли SHIFT. */
    bool _shift;

    /** @brief Включён ли CAPS LOCK. */
    bool _capslock;

    /** @brief Нажата ли клавиша GUI (Windows). */
    bool _gui;

    /** @brief Включён ли NUM LOCK. */
    bool _numlock;

    /** @brief Включён ли SCROLL LOCK. */
    bool _scrolllock;

    /** @brief Последняя нажатая мёртвая клавиша; @c virtual_key::vk_none, если её нет. */
    virtual_key _last_dead_key;

    /** @brief Горит ли индикатор NUM LOCK. */
    bool _num_lock_led;

    /** @brief Горит ли индикатор CAPS LOCK. */
    bool _caps_lock_led;

    /** @brief Горит ли индикатор SCROLL LOCK. */
    bool _scroll_lock_led;

    /** @brief Кодовая страница для знаков за пределами ASCII; @c nullptr — такие знаки не переводятся. */
    const code_page *_codepage;
};

}


// Реализация

// Открытые

/**
 * @brief Сообщает, найдена ли клавиатура.
 *
 * Значение верно после begin() или reset().
 *
 * @return @c true, если клавиатура прошла самопроверку.
 */
bool esp::ps2::keyboard::is_keyboard_available() const
{
    return _keyboard_available;
}

/**
 * @brief Возвращает текущую раскладку.
 * @return Раскладка, заданная set_layout(); после сброса — US.
 */
const esp::ps2::keyboard_layout *esp::ps2::keyboard::layout() const
{
    return _layout;
}

/**
 * @brief Задаёт кодовую страницу для перевода виртуальных клавиш в ASCII.
 * @param codepage Кодовая страница, например &esp::ps2::CODE_PAGE_437.
 */
void esp::ps2::keyboard::set_code_page(const code_page *codepage)
{
    _codepage = codepage;
}

/**
 * @brief Задаёт автоповтор клавиш.
 *
 * Если клавишу держать, через @p repeat_delay_ms клавиатура начинает
 * повторять её код каждые @p repeat_rate_ms.
 *
 * @param repeat_rate_ms Период повтора, мс: от 33 до 500.
 * @param repeat_delay_ms Задержка до повтора, мс: от 250 до 1000 с шагом 250.
 * @return @c true, если клавиатура приняла команду.
 */
bool esp::ps2::keyboard::set_typematic_rate_and_delay(int repeat_rate_ms, int repeat_delay_ms)
{
    return send_cmd_typematic_rate_and_delay(repeat_rate_ms, repeat_delay_ms);
}

/**
 * @brief Возвращает текущий набор скан-кодов.
 * @return 1, 2 или 3.
 */
int esp::ps2::keyboard::scancode_set() const
{
    return _scancode_set;
}
