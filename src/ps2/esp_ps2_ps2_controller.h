// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_ps2_kbd_mode.h"
#include "esp_ps2_ps2_preset.h"
#include "esp_tools.h"

#include <cstdint>
#include <memory>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>


namespace esp::ps2
{

class keyboard;
class mouse;

class ps2_controller
{

public:

    ps2_controller() = delete;

    static void begin(gpio_num_t port_0_clk_gpio, gpio_num_t port_0_dat_gpio,
                      gpio_num_t port_1_clk_gpio = GPIO_UNUSED,
                      gpio_num_t port_1_dat_gpio = GPIO_UNUSED);
    static void begin(ps2_preset preset = ps2_preset::keyboard_port_0_mouse_port_1,
                      kbd_mode keyboard_mode = kbd_mode::create_virtual_keys_queue);
    static void end();
    inline static bool initialized();

    static bool data_available(int ps2_port);
    static int get_data(int ps2_port, int time_out_ms);
    static void send_data(std::uint8_t data, int ps2_port);

    static void disable_rx(int ps2_port);
    static void enable_rx(int ps2_port);

    inline static esp::ps2::keyboard *keyboard();
    inline static void set_keyboard(esp::ps2::keyboard *value);
    inline static esp::ps2::mouse *mouse();
    inline static void set_mouse(esp::ps2::mouse *value);

    inline static bool parity_error(int ps2_port);
    inline static bool sync_error(int ps2_port);
    inline static bool clk_time_out_error(int ps2_port);

    static bool lock(int ps2_port, int time_out_ms);
    static void unlock(int ps2_port);

private:

    static void IRAM_ATTR ulp_wake_isr(void *arg);

    /**
     * @brief Клавиатура.
     *
     * Её создаёт begin() по набору устройств или задаёт set_keyboard();
     * @c nullptr — клавиатуры нет.
     */
    static esp::ps2::keyboard *_keyboard;

    /**
     * @brief Мышь.
     *
     * Её создаёт begin() по набору устройств или задаёт set_mouse();
     * @c nullptr — мыши нет.
     */
    static esp::ps2::mouse *_mouse;

    /** @brief Клавиатура, созданная begin() по набору устройств; её удаляет end(). */
    static std::unique_ptr<esp::ps2::keyboard> _own_keyboard;

    /** @brief Мышь, созданная begin() по набору устройств; её удаляет end(). */
    static std::unique_ptr<esp::ps2::mouse> _own_mouse;

    /** @brief Настроены ли порты 0 и 1. */
    static bool _port_enabled[2];

    /** @brief Обработчик прерывания, которым сопроцессор ULP будит процессор. */
    static intr_handle_t _ulp_wake_isr_handle;

    /** @brief Была ли ошибка чётности в последнем байте, полученном get_data(). */
    static bool _parity_error[2];

    /** @brief Не было ли стартового или стопового разряда в последнем байте, полученном get_data(). */
    static bool _sync_error[2];

    /** @brief Не ответила ли линия CLK при последнем приёме get_data(). */
    static bool _clk_time_out_error[2];

    /** @brief Очереди портов из одного слова: последнее принятое слово. */
    static QueueHandle_t _data_in[2];

    /** @brief Рекурсивные мьютексы для монопольного доступа к портам. */
    static SemaphoreHandle_t _port_lock[2];

    /** @brief Запущен ли сопроцессор ULP; он работает и после end(). */
    static bool _init_done;
};

}


// Реализация

// Открытые

/**
 * @brief Сообщает, запущен ли контроллер.
 * @return @c true, если begin() уже вызывался.
 */
bool esp::ps2::ps2_controller::initialized()
{
    return _init_done;
}

/**
 * @brief Возвращает клавиатуру.
 * @return Клавиатура, созданная begin() или заданная set_keyboard();
 *         @c nullptr, если её нет.
 */
esp::ps2::keyboard *esp::ps2::ps2_controller::keyboard()
{
    return _keyboard;
}

/**
 * @brief Задаёт клавиатуру; контроллер ею не владеет.
 * @param value Клавиатура или @c nullptr.
 */
void esp::ps2::ps2_controller::set_keyboard(esp::ps2::keyboard *value)
{
    _keyboard = value;
}

/**
 * @brief Возвращает мышь.
 * @return Мышь, созданная begin() или заданная set_mouse(); @c nullptr,
 *         если её нет.
 */
esp::ps2::mouse *esp::ps2::ps2_controller::mouse()
{
    return _mouse;
}

/**
 * @brief Задаёт мышь; контроллер ею не владеет.
 * @param value Мышь или @c nullptr.
 */
void esp::ps2::ps2_controller::set_mouse(esp::ps2::mouse *value)
{
    _mouse = value;
}

/**
 * @brief Сообщает, была ли ошибка чётности в последнем байте порта.
 * @param ps2_port Порт: 0 или 1.
 * @return @c true, если была.
 */
bool esp::ps2::ps2_controller::parity_error(int ps2_port)
{
    return _parity_error[ps2_port];
}

/**
 * @brief Сообщает, были ли в последнем байте порта стартовый и стоповый разряды.
 * @param ps2_port Порт: 0 или 1.
 * @return @c true, если какого-то не было.
 */
bool esp::ps2::ps2_controller::sync_error(int ps2_port)
{
    return _sync_error[ps2_port];
}

/**
 * @brief Сообщает, ответила ли линия CLK при последнем приёме порта.
 * @param ps2_port Порт: 0 или 1.
 * @return @c true, если не ответила.
 */
bool esp::ps2::ps2_controller::clk_time_out_error(int ps2_port)
{
    return _clk_time_out_error[ps2_port];
}
