// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_bitmapped_display_controller.h"
#include "esp_gfx_size.h"
#include "esp_ps2_mouse_delta.h"
#include "esp_ps2_mouse_packet.h"
#include "esp_ps2_mouse_status.h"
#include "esp_ps2_mouse_type.h"
#include "esp_ps2_ps2_device.h"

#include <cstdint>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


namespace esp::ps2
{

class mouse : public ps2_device
{

public:

    mouse();
    ~mouse();

    void begin(gpio_num_t clk_gpio, gpio_num_t data_gpio);
    void begin(int ps2_port);
    bool reset();
    inline bool is_mouse_available() const;

    int packet_size();
    bool packet_available();
    bool get_next_packet(mouse_packet *packet, int time_out_ms = -1,
                         bool request_resend_on_time_out = false);
    bool delta_available();
    bool get_next_delta(mouse_delta *delta, int time_out_ms = -1,
                        bool request_resend_on_time_out = false);

    inline bool set_sample_rate(int value);
    inline bool set_resolution(int value);
    inline bool set_scaling(int value);

    void setup_absolute_positioner(
        int width, int height, bool create_absolute_positions_queue,
        esp::gfx::bitmapped_display_controller *update_display_controller = nullptr);
    void terminate_absolute_positioner();
    void update_absolute_position(mouse_delta *delta);
    inline mouse_status &status();
    int available_status();
    mouse_status get_next_status(int time_out_ms = -1);
    inline int &movement_acceleration();
    inline int &wheel_acceleration();
    void empty_queue();

    inline static void quick_check_hardware();

private:

    static void mouse_update_task(void *arg);
    bool decode_mouse_packet(mouse_packet *packet, mouse_delta *delta);

    /**
     * @brief Не повторять сброс, если мышь не нашлась.
     *
     * Устанавливает quick_check_hardware(): тогда сброс делается один раз
     * с коротким временем ожидания, и отсутствие мыши выясняется быстро.
     */
    static bool _quick_check_hardware;

    /** @brief Найдена ли мышь при последнем сбросе. */
    bool _mouse_available;

    /** @brief Тип мыши, определённый при сбросе; от него зависит размер пакета. */
    mouse_type _mouse_type;

    /** @brief Задача обновления абсолютного положения; @c nullptr, если её нет. */
    TaskHandle_t _mouse_update_task;

    /** @brief Очередь из одного пакета мыши, собранного из принятых байтов. */
    QueueHandle_t _received_packet;

    /** @brief Включено ли обновление абсолютного положения. */
    bool _absolute_update;

    /** @brief Область абсолютного положения: от (0, 0) до (ширина − 1, высота − 1). */
    esp::gfx::size _area;

    /** @brief Текущее состояние: абсолютное положение, колесо, кнопки. */
    mouse_status _status;

    /** @brief Предыдущее состояние; по нему видно, что изменилось. */
    mouse_status _prev_status;

    /** @brief Время предыдущего перемещения, мкс; нужно для ускорения. */
    std::int64_t _prev_delta_time;

    /** @brief Коэффициент ускорения перемещения; разумные значения — от 0 до 2000, по умолчанию 180. */
    int _movement_acceleration;

    /** @brief Коэффициент ускорения колеса; разумные значения — от 0 до 100000, по умолчанию 60000. */
    int _wheel_acceleration;

    /** @brief Очередь состояний, которые даёт update_absolute_position(); @c nullptr, если её нет. */
    QueueHandle_t _absolute_queue;

    /** @brief Контроллер дисплея, курсор которого следует за мышью; @c nullptr — не следует. */
    esp::gfx::bitmapped_display_controller *_update_display_controller;
};

}


// Реализация

// Открытые

/**
 * @brief Сообщает, найдена ли мышь.
 *
 * Значение верно после begin() или reset().
 *
 * @return @c true, если мышь найдена и готова.
 */
bool esp::ps2::mouse::is_mouse_available() const
{
    return _mouse_available;
}

/**
 * @brief Задаёт наибольшую частоту сообщений о перемещении.
 * @param value Сообщений в секунду: 10, 20, 40, 60, 80, 100 или 200; по умолчанию 60.
 * @return @c true, если мышь приняла команду.
 */
bool esp::ps2::mouse::set_sample_rate(int value)
{
    return send_cmd_set_sample_rate(value);
}

/**
 * @brief Задаёт разрешение — сколько отсчётов перемещения приходится на миллиметр.
 * @param value 0 — 1 отсчёт на мм (25 точек на дюйм), 1 — 2 (50), 2 — 4 (100, по умолчанию), 3 — 8 (200).
 * @return @c true, если мышь приняла команду.
 */
bool esp::ps2::mouse::set_resolution(int value)
{
    return send_cmd_set_resolution(value);
}

/**
 * @brief Задаёт масштабирование перемещений.
 * @param value 1 — 1:1 (по умолчанию), 2 — 1:2.
 * @return @c true, если мышь приняла команду.
 */
bool esp::ps2::mouse::set_scaling(int value)
{
    return send_cmd_set_scaling(value);
}

/**
 * @brief Возвращает текущее состояние мыши; его можно и менять, например переносить указатель.
 * @return Ссылка на состояние.
 */
esp::ps2::mouse_status &esp::ps2::mouse::status()
{
    return _status;
}

/**
 * @brief Возвращает коэффициент ускорения перемещения; его можно менять.
 *
 * Ускорение вычисляется в update_absolute_position() по скорости движения:
 * чем больше коэффициент, тем сильнее быстрое движение уносит указатель.
 * Разумные значения — от 0 до 2000, по умолчанию 180.
 *
 * @return Ссылка на коэффициент.
 */
int &esp::ps2::mouse::movement_acceleration()
{
    return _movement_acceleration;
}

/**
 * @brief Возвращает коэффициент ускорения колеса; его можно менять.
 *
 * Разумные значения — от 0 до 100000, по умолчанию 60000.
 *
 * @return Ссылка на коэффициент.
 */
int &esp::ps2::mouse::wheel_acceleration()
{
    return _wheel_acceleration;
}

/**
 * @brief Отключает повторные попытки сброса, если мышь не нашлась.
 */
void esp::ps2::mouse::quick_check_hardware()
{
    _quick_check_hardware = true;
}
