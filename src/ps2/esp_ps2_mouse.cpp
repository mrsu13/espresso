// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_mouse.h
 * @brief Объявление мыши PS/2 esp::ps2::mouse.
 */

/**
 * @file esp_ps2_mouse.cpp
 * @brief Мышь PS/2: пакеты, перемещения, абсолютное положение с ускорением.
 */

#include "esp_ps2_mouse.h"

#include "esp_ps2_config.h"
#include "esp_ps2_ps2_controller.h"
#include "esp_ps2_ps2_device_lock.h"
#include "esp_ps2_utils.h"
#include "esp_tools.h"

#include <cstdint>
#include <cstdlib>

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


#pragma GCC optimize("O2")


/**
 * @class esp::ps2::mouse
 * @brief Мышь PS/2: перемещения, колесо, кнопки и абсолютное положение указателя.
 *
 * Мышь подключена к одному из портов контроллера esp::ps2::ps2_controller.
 * Поддерживаются обычная мышь (две оси, три кнопки) и мышь, совместимая с
 * Microsoft IntelliMouse (ещё колесо); тип определяется при сбросе. Можно
 * задать частоту сообщений, разрешение и масштабирование.
 *
 * Из перемещений мышь может вести абсолютное положение указателя в
 * заданной области с ускорением, ставить состояния в очередь и двигать
 * курсор мыши контроллера дисплея (setup_absolute_positioner()). Обычно
 * мышь создаёт контроллер PS/2 в esp::ps2::ps2_controller::begin():
 * @code
 * esp::ps2::mouse *mouse = esp::ps2::ps2_controller::mouse();
 * esp::ps2::mouse_delta delta;
 * if (mouse->get_next_delta(&delta))
 *     printf("x %d, y %d, z %d\n", delta.delta_x, delta.delta_y, delta.delta_z);
 * @endcode
 */


bool esp::ps2::mouse::_quick_check_hardware = false;


// Открытые

/**
 * @brief Создаёт мышь без порта; ускорения по умолчанию.
 */
esp::ps2::mouse::mouse()
    : _mouse_available{false}
    , _mouse_type{mouse_type::legacy_mouse}
    , _mouse_update_task{nullptr}
    , _received_packet{nullptr}
    , _absolute_update{false}
    , _prev_delta_time{0}
    , _movement_acceleration{180}
    , _wheel_acceleration{60000}
    , _absolute_queue{nullptr}
    , _update_display_controller{nullptr}
{
}

/**
 * @brief Останавливает обновление положения и удаляет задачу и очереди мыши.
 */
esp::ps2::mouse::~mouse()
{
    const esp::ps2::ps2_device_lock lock{this};
    terminate_absolute_positioner();
    if (_mouse_update_task)
        vTaskDelete(_mouse_update_task);
    if (_received_packet)
        vQueueDelete(_received_packet);
}

/**
 * @brief Запускает мышь на выводах GPIO и сбрасывает её.
 *
 * Заодно настраивает контроллер PS/2 на один порт 0. Из-за сопроцессора
 * ULP годятся только выводы, доступные ему: GPIO2, 4, 12, 13, 14, 15, 25,
 * 26, 27, 32 и 33.
 *
 * @param clk_gpio Вывод линии CLK.
 * @param data_gpio Вывод линии DATA.
 */
void esp::ps2::mouse::begin(gpio_num_t clk_gpio, gpio_num_t data_gpio)
{
    esp::ps2::ps2_controller::begin(clk_gpio, data_gpio);
    esp::ps2::ps2_controller::set_mouse(this);
    begin(0);
}

/**
 * @brief Запускает мышь на порту уже настроенного контроллера PS/2 и сбрасывает её.
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::mouse::begin(int ps2_port)
{
    if (_quick_check_hardware)
        ps2_device::quick_check_hardware();
    ps2_device::begin(ps2_port);
    reset();
    _received_packet = xQueueCreate(1, sizeof(esp::ps2::mouse_packet));
    xTaskCreate(&mouse_update_task, "", 1600, this, 5, &_mouse_update_task);
    _area = esp::gfx::size{0, 0};
}

/**
 * @brief Сбрасывает мышь и определяет её тип.
 *
 * Сброс повторяется до трёх раз (один при quick_check_hardware()). Затем
 * мышь пробуют перевести в режим IntelliMouse с колесом.
 *
 * @return @c true, если мышь найдена и готова.
 */
bool esp::ps2::mouse::reset()
{
    if (_quick_check_hardware)
        _mouse_available = send_cmd_reset();
    else
    {
        // До трёх попыток сброса.
        for (int i = 0; i < 3; ++i)
        {
            _mouse_available = send_cmd_reset();
            if (_mouse_available)
                break;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        // Даём мыши время закончить самопроверку.
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // Согласуем режим и параметры.
    if (_mouse_available)
    {
        // Частоты 200, 100 и 80 подряд включают у мыши IntelliMouse колесо
        // и пакет из 4 байтов; тогда мышь опознаётся как мышь с колесом.
        if (send_cmd_set_sample_rate(200) && send_cmd_set_sample_rate(100)
            && send_cmd_set_sample_rate(80)
            && identify() == esp::ps2::ps2_device_type::mouse_with_scroll_wheel)
        {
            _mouse_type = esp::ps2::mouse_type::intellimouse;
        }

        set_sample_rate(60);
    }

    return _mouse_available;
}

/**
 * @brief Возвращает размер пакета мыши.
 * @return 4 байта у мыши с колесом, 3 у обычной.
 */
int esp::ps2::mouse::packet_size()
{
    return _mouse_type == esp::ps2::mouse_type::intellimouse ? 4 : 3;
}

/**
 * @brief Сообщает, есть ли собранный пакет для get_next_packet().
 *
 * Пока включено абсолютное положение (setup_absolute_positioner()), пакеты
 * разбирает задача мыши и сюда они не попадают.
 *
 * @return @c true, если пакет есть.
 */
bool esp::ps2::mouse::packet_available()
{
    return uxQueueMessagesWaiting(_received_packet) > 0;
}

/**
 * @brief Получает необработанный пакет мыши.
 *
 * Пакеты собирает из принятых байтов задача мыши; в очереди хранится
 * только последний пакет.
 *
 * @param packet Куда записать пакет.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @param request_resend_on_time_out Не используется.
 * @return @c true, если пакет получен.
 */
bool esp::ps2::mouse::get_next_packet(esp::ps2::mouse_packet *packet, int time_out_ms,
                                      bool request_resend_on_time_out)
{
    return xQueueReceive(_received_packet, packet, esp::ms_to_ticks(time_out_ms));
}

/**
 * @brief Сообщает, есть ли перемещение для get_next_delta().
 * @return @c true, если есть собранный пакет (см. packet_available()).
 */
bool esp::ps2::mouse::delta_available()
{
    return packet_available();
}

/**
 * @brief Получает перемещение мыши.
 * @param delta Куда записать перемещение; может быть @c nullptr — тогда
 *        обновляются только кнопки в status().
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @param request_resend_on_time_out Не используется.
 * @return @c true, если получен правильный пакет.
 */
bool esp::ps2::mouse::get_next_delta(esp::ps2::mouse_delta *delta, int time_out_ms,
                                     bool request_resend_on_time_out)
{
    esp::ps2::mouse_packet packet;
    return get_next_packet(&packet, time_out_ms, request_resend_on_time_out)
        && decode_mouse_packet(&packet, delta);
}

/**
 * @brief Включает расчёт абсолютного положения указателя.
 *
 * Положение ограничено областью от (0, 0) до (@p width − 1, @p height − 1).
 * Если нужна очередь состояний или курсор дисплея, создаётся задача,
 * которая сама вызывает update_absolute_position() на каждое перемещение.
 * Вызывать один раз.
 *
 * @param width Ширина области.
 * @param height Высота области.
 * @param create_absolute_positions_queue @c true — ставить состояния в
 *        очередь для get_next_status().
 * @param update_display_controller Контроллер дисплея, курсор которого
 *        должен следовать за мышью, или @c nullptr.
 */
void esp::ps2::mouse::setup_absolute_positioner(
    int width, int height, bool create_absolute_positions_queue,
    esp::gfx::bitmapped_display_controller *update_display_controller)
{
    if (_area != esp::gfx::size{width, height})
    {
        _area = esp::gfx::size{width, height};
        _status.x = width >> 1;
        _status.y = height >> 1;
    }
    _status.wheel_delta = 0;
    _status.buttons.left = 0;
    _status.buttons.middle = 0;
    _status.buttons.right = 0;
    _prev_status = _status;

    _update_display_controller = update_display_controller;

    if (create_absolute_positions_queue && !_absolute_queue)
    {
        _absolute_queue =
            xQueueCreate(esp::ps2::MOUSE_EVENTS_QUEUE_SIZE, sizeof(esp::ps2::mouse_status));
    }

    if (_update_display_controller)
    {
        // Курсор — в начальное положение.
        _update_display_controller->set_mouse_cursor_pos(_status.x, _status.y);
    }

    _absolute_update = (_update_display_controller || create_absolute_positions_queue);
}

/**
 * @brief Выключает расчёт абсолютного положения: удаляет задачу и очередь состояний.
 */
void esp::ps2::mouse::terminate_absolute_positioner()
{
    if (_absolute_queue)
    {
        vQueueDelete(_absolute_queue);
        _absolute_queue = nullptr;
    }
    _absolute_update = false;
    _update_display_controller = nullptr;
}

/**
 * @brief Обновляет абсолютное положение, колесо и кнопки по перемещению.
 *
 * Перемещение умножается на ускорение, которое растёт со скоростью
 * движения; поэтому вызывать метод нужно часто и регулярно. Если
 * настроены очередь состояний или курсор дисплея, метод вызывает задача
 * мыши.
 *
 * @param delta Перемещение мыши.
 */
void esp::ps2::mouse::update_absolute_position(esp::ps2::mouse_delta *delta)
{
    // Если с прошлого перемещения прошло больше 0,5 с, ускорения нет.
    constexpr int MAX_DELTA_TIME_US = 500000;

    int dx = delta->delta_x;
    int dy = delta->delta_y;
    int dz = delta->delta_z;

    const std::int64_t now = esp_timer_get_time();
    const int delta_time = static_cast<int>(now - _prev_delta_time); // Мкс.

    if (delta_time < MAX_DELTA_TIME_US)
    {
        // Ускорение перемещения: скорость v становится v + k·v².
        if (dx != 0 || dy != 0)
        {
            const int delta_dist = esp::ps2::isqrt(dx * dx + dy * dy); // Отсчёты мыши.
            const float vel = static_cast<float>(delta_dist) / delta_time; // Отсчёты за мкс.
            const float new_vel = vel + _movement_acceleration * vel * vel;
            const int new_delta_dist = static_cast<int>(new_vel * delta_time);
            dx = dx * new_delta_dist / delta_dist;
            dy = dy * new_delta_dist / delta_dist;
        }

        // Ускорение колеса — так же.
        if (dz != 0)
        {
            const int delta_dist = std::abs(dz);
            const float vel = static_cast<float>(delta_dist) / delta_time;
            const float new_vel = vel + _wheel_acceleration * vel * vel;
            const int new_delta_dist = static_cast<int>(new_vel * delta_time);
            dz = dz * new_delta_dist / delta_dist;
        }
    }

    _status.x = esp::tclamp(static_cast<int>(_status.x) + dx, 0, _area.width - 1);
    _status.y = esp::tclamp(static_cast<int>(_status.y) - dy, 0, _area.height - 1);
    _status.wheel_delta = dz;
    _prev_delta_time = now;
}

/**
 * @brief Возвращает число состояний в очереди.
 *
 * Очередь есть, только если setup_absolute_positioner() вызван с
 * @c create_absolute_positions_queue = @c true.
 *
 * @return Число состояний.
 */
int esp::ps2::mouse::available_status()
{
    return _absolute_queue ? uxQueueMessagesWaiting(_absolute_queue) : 0;
}

/**
 * @brief Берёт следующее состояние из очереди.
 *
 * @note Если очереди нет или время вышло, возвращается состояние по
 *       умолчанию — точка (0, 0), кнопки отпущены; отличить его от
 *       настоящего можно только через available_status().
 *
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return Состояние.
 */
esp::ps2::mouse_status esp::ps2::mouse::get_next_status(int time_out_ms)
{
    esp::ps2::mouse_status status;
    if (_absolute_queue)
        xQueueReceive(_absolute_queue, &status, esp::ms_to_ticks(time_out_ms));
    return status;
}

/**
 * @brief Отбрасывает непрочитанные байты порта и очищает очередь состояний.
 */
void esp::ps2::mouse::empty_queue()
{
    while (get_data(0) != -1)
    {
    }
    if (_absolute_queue)
        xQueueReset(_absolute_queue);
}

// Закрытые

/**
 * @brief Задача мыши: получает перемещения и обновляет положение, очередь и курсор.
 * @param arg Мышь.
 */
void esp::ps2::mouse::mouse_update_task(void *arg)
{
    // Наибольший промежуток между байтами одного пакета, мкс.
    constexpr int MAX_TIME_BETWEEN_DATA_US = 500000;

    auto *const self = static_cast<esp::ps2::mouse *>(arg);

    std::int64_t prev_data_time = 0;

    while (true)
    {
        esp::ps2::mouse_packet packet;
        int packet_len = 0;
        while (packet_len < self->packet_size())
        {
            const int r = self->get_data(-1);
            if (self->parity_error() || self->sync_error())
            {
                packet_len = 0;
                continue;
            }
            const std::int64_t now = esp_timer_get_time();
            if (packet_len > 0 && prev_data_time > 0
                && (now - prev_data_time) > MAX_TIME_BETWEEN_DATA_US)
            {
                // Байт пришёл слишком поздно: начинаем новый пакет.
                packet_len = 0;
            }
            if (r > -1)
            {
                packet.data[packet_len++] = r;
                prev_data_time = now;
            }
        }

        if (self->_absolute_update)
        {
            esp::ps2::mouse_delta delta;
            if (self->decode_mouse_packet(&packet, &delta))
            {
                self->update_absolute_position(&delta);

                // Курсор контроллера дисплея.
                if (self->_update_display_controller)
                {
                    self->_update_display_controller->set_mouse_cursor_pos(self->_status.x,
                                                                           self->_status.y);
                }

                // Очередь для available_status() и get_next_status().
                if (self->_absolute_queue)
                    xQueueSendToBack(self->_absolute_queue, &self->_status, 0);
            }
        }
        else
        {
            xQueueOverwrite(self->_received_packet, &packet);
        }
    }
}

/**
 * @brief Разбирает пакет мыши и обновляет кнопки в status().
 *
 * Байт 0: разряд 0 — левая кнопка, 1 — правая, 2 — средняя, 3 — всегда 1,
 * 4 — знак X, 5 — знак Y, 6 — переполнение X, 7 — переполнение Y.
 * Байт 1 — перемещение по X, байт 2 — по Y, байт 3 — поворот колеса.
 *
 * @param packet Пакет.
 * @param delta Куда записать перемещение или @c nullptr.
 * @return @c true, если пакет правильный (разряд 3 первого байта равен 1).
 */
bool esp::ps2::mouse::decode_mouse_packet(esp::ps2::mouse_packet *packet,
                                          esp::ps2::mouse_delta *delta)
{
    if ((packet->data[0] & 0x08) == 0)
        return false;

    _prev_status = _status;

    _status.buttons.left = (packet->data[0] & 0x01 ? 1 : 0);
    _status.buttons.middle = (packet->data[0] & 0x04 ? 1 : 0);
    _status.buttons.right = (packet->data[0] & 0x02 ? 1 : 0);
    if (delta)
    {
        // Знаки перемещений — в первом байте: расширяем до 16 разрядов.
        delta->delta_x = static_cast<std::int16_t>(packet->data[0] & 0x10 ? 0xFF00 | packet->data[1]
                                                                          : packet->data[1]);
        delta->delta_y = static_cast<std::int16_t>(packet->data[0] & 0x20 ? 0xFF00 | packet->data[2]
                                                                          : packet->data[2]);
        delta->delta_z = static_cast<std::int8_t>(packet_size() > 3 ? packet->data[3] : 0);
        delta->overflow_x = (packet->data[0] & 0x40 ? 1 : 0);
        delta->overflow_y = (packet->data[0] & 0x80 ? 1 : 0);
        delta->buttons = _status.buttons;
    }

    return true;
}
