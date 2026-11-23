// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_ps2_device.h
 * @brief Объявление устройства PS/2 esp::ps2::ps2_device.
 */

/**
 * @file esp_ps2_ps2_device.cpp
 * @brief Устройство PS/2: обмен байтами и команды протокола.
 */

#include "esp_ps2_ps2_device.h"

#include "esp_ps2_ps2_controller.h"
#include "esp_ps2_ps2_device_lock.h"
#include "esp_ps2_ps2_port_auto_disable_rx.h"
#include "esp_ps2_time_out.h"

#include <algorithm>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#pragma GCC optimize("O2")


namespace esp::ps2::p
{

/** @brief Команда «включить светодиоды» (клавиатура). */
constexpr std::uint8_t CMD_SET_LEDS = 0xED;

/** @brief Команда «эхо»: устройство отвечает тем же байтом. */
constexpr std::uint8_t CMD_ECHO = 0xEE;

/** @brief Команда «узнать или задать набор скан-кодов» (клавиатура). */
constexpr std::uint8_t CMD_GET_SET_SCANCODE_SET = 0xF0;

/** @brief Команда «режим опроса» (мышь); тот же код, что у @c CMD_GET_SET_SCANCODE_SET. */
constexpr std::uint8_t CMD_SET_REMOTE_MODE = 0xF0;

/** @brief Команда опознания устройства. */
constexpr std::uint8_t CMD_IDENTIFY = 0xF2;

/** @brief Команда «скорость и задержка автоповтора» (клавиатура). */
constexpr std::uint8_t CMD_SET_TYPEMATIC_RATE_AND_DELAY = 0xF3;

/** @brief Команда «частота опроса» (мышь); тот же код, что у автоповтора клавиатуры. */
constexpr std::uint8_t CMD_SET_SAMPLE_RATE = 0xF3;

/** @brief Команда «включить сканирование»: устройство начинает присылать данные. */
constexpr std::uint8_t CMD_ENABLE_SCANNING = 0xF4;

/** @brief Команда «выключить сканирование». */
constexpr std::uint8_t CMD_DISABLE_SCANNING = 0xF5;

/** @brief Команда «параметры по умолчанию». */
constexpr std::uint8_t CMD_SET_DEFAULT_PARAMS = 0xF6;

/** @brief Команда «повторить последний байт». */
constexpr std::uint8_t CMD_RESEND_LAST_BYTE = 0xFE;

/** @brief Команда сброса: устройство проводит самопроверку. */
constexpr std::uint8_t CMD_RESET = 0xFF;

/** @brief Команда «потоковый режим» (мышь). */
constexpr std::uint8_t CMD_SET_STREAM_MODE = 0xEA;

/** @brief Команда «запрос состояния» (мышь). */
constexpr std::uint8_t CMD_STATUS_REQUEST = 0xE9;

/** @brief Команда «разрешение» (мышь). */
constexpr std::uint8_t CMD_SET_RESOLUTION = 0xE8;

/** @brief Команда «масштабирование» (мышь). */
constexpr std::uint8_t CMD_SET_SCALING = 0xE6;

/** @brief Ответ «ошибка» (вариант 1). */
constexpr std::uint8_t REPLY_ERROR1 = 0x00;

/** @brief Ответ «ошибка» (вариант 2). */
constexpr std::uint8_t REPLY_ERROR2 = 0xFF;

/** @brief Ответ «самопроверка пройдена». */
constexpr std::uint8_t REPLY_SELFTEST_OK = 0xAA;

/** @brief Ответ на команду «эхо». */
constexpr std::uint8_t REPLY_ECHO = 0xEE;

/** @brief Ответ «команда принята». */
constexpr std::uint8_t REPLY_ACK = 0xFA;

/** @brief Ответ «самопроверка не пройдена» (вариант 1). */
constexpr std::uint8_t REPLY_SELFTEST_FAILED1 = 0xFC;

/** @brief Ответ «самопроверка не пройдена» (вариант 2). */
constexpr std::uint8_t REPLY_SELFTEST_FAILED2 = 0xFD;

/** @brief Ответ «повторите». */
constexpr std::uint8_t REPLY_RESEND = 0xFE;

/** @brief Время ожидания ответа на команду по умолчанию, мс. */
constexpr int DEFAULT_CMD_TIMEOUT = 500;

/** @brief Половина времени ожидания по умолчанию, мс. */
constexpr int DEFAULT_CMD_SUBTIMEOUT = DEFAULT_CMD_TIMEOUT / 2;

/** @brief Короткое время ожидания ответа для быстрой проверки наличия устройства, мс. */
constexpr int QUICK_CMD_TIMEOUT = 50;

/** @brief Половина короткого времени ожидания, мс. */
constexpr int QUICK_CMD_SUBTIMEOUT = QUICK_CMD_TIMEOUT / 2;

/**
 * @brief Периоды автоповтора клавиатуры, мс, по кодам команды автоповтора.
 *
 * Номер элемента — код скорости в команде
 * @c CMD_SET_TYPEMATIC_RATE_AND_DELAY.
 */
const std::int16_t REPEAT_RATES[32] = {33,  37,  41,  45,  50,  54,  58,  62,  66,  75,  83,
                                       91,  100, 108, 125, 125, 133, 149, 166, 181, 200, 217,
                                       232, 250, 270, 303, 333, 370, 400, 434, 476, 500};

}


/**
 * @class esp::ps2::ps2_device
 * @brief Основа устройств PS/2 — клавиатуры и мыши.
 *
 * Устройство подключено к одному из двух портов контроллера
 * esp::ps2::ps2_controller, который принимает и передаёт байты с помощью
 * сопроцессора ULP. Класс посылает команды протокола PS/2 и ждёт ответов,
 * захватывая порт, чтобы команды разных задач не перемешались.
 *
 * Устройство владеет задачами и очередями производных классов, поэтому
 * копировать его нельзя.
 *
 * @bug send_cmd_get_scancode_set() проверяет ответ условием «≥ 1 или ≤ 3»,
 *      которое верно всегда. Строгое «и» не подходит: клавиатуры с
 *      трансляцией отвечают кодами 0x43, 0x41 и 0x3F.
 */


// Открытые

/**
 * @brief Захватывает устройство для монопольного обмена.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return @c true, если устройство захвачено.
 */
bool esp::ps2::ps2_device::lock(int time_out_ms)
{
    return ps2_controller::lock(_ps2_port, time_out_ms);
}

/**
 * @brief Освобождает устройство.
 */
void esp::ps2::ps2_device::unlock()
{
    ps2_controller::unlock(_ps2_port);
}

/**
 * @brief Сообщает, была ли ошибка чётности в последнем принятом байте.
 * @return @c true при ошибке чётности.
 */
bool esp::ps2::ps2_device::parity_error()
{
    return ps2_controller::parity_error(_ps2_port);
}

/**
 * @brief Сообщает, были ли в последнем принятом байте неверные стартовый или стоповый биты.
 * @return @c true при ошибке синхронизации.
 */
bool esp::ps2::ps2_device::sync_error()
{
    return ps2_controller::sync_error(_ps2_port);
}

/**
 * @brief Сообщает, не дождался ли приём последнего байта такта CLK.
 * @return @c true, если такт не пришёл вовремя.
 */
bool esp::ps2::ps2_device::clk_time_out_error()
{
    return ps2_controller::clk_time_out_error(_ps2_port);
}

/**
 * @brief Посылает команду и ждёт ответа.
 *
 * На время команды приём с другого порта запрещается, чтобы второе
 * устройство не помешало ответу.
 *
 * @param cmd Байт команды.
 * @param expected_reply Ожидаемый ответ.
 * @return @c true, если ответ получен вовремя.
 */
bool esp::ps2::ps2_device::send_command(std::uint8_t cmd, std::uint8_t expected_reply)
{
    constexpr int INTER_WAITREPLY_TIMEOUT_MS = 10;

    const esp::ps2::ps2_device_lock device_lock{this};

    // На время команды выключаем приём другого порта.
    const esp::ps2::ps2_port_auto_disable_rx auto_disable_rx{!_ps2_port};

    ps2_controller::send_data(cmd, _ps2_port);
    const esp::ps2::time_out timeout;
    do
    {
        if (ps2_controller::get_data(_ps2_port, INTER_WAITREPLY_TIMEOUT_MS) == expected_reply)
            return true;
    } while (!timeout.expired(_cmd_time_out));
    return false;
}

/**
 * @brief Посылает байт, не дожидаясь ответа.
 * @param cmd Байт команды.
 */
void esp::ps2::ps2_device::send_command(std::uint8_t cmd)
{
    ps2_controller::send_data(cmd, _ps2_port);
}

/**
 * @brief Приостанавливает порт, держа линию CLK низкой.
 *
 * Снова разрешает приём resume_port().
 */
void esp::ps2::ps2_device::suspend_port()
{
    ps2_controller::disable_rx(_ps2_port);
}

/**
 * @brief Возобновляет порт, отпуская линию CLK.
 */
void esp::ps2::ps2_device::resume_port()
{
    ps2_controller::enable_rx(_ps2_port);
}

// Защищённые

/**
 * @brief Создаёт устройство с обычным временем ожидания ответов.
 */
esp::ps2::ps2_device::ps2_device()
    : _cmd_time_out{esp::ps2::p::DEFAULT_CMD_TIMEOUT}
    , _cmd_sub_time_out{esp::ps2::p::DEFAULT_CMD_SUBTIMEOUT}
{
}

/**
 * @brief Уничтожает устройство.
 */
esp::ps2::ps2_device::~ps2_device() {}

/**
 * @brief Сокращает время ожидания ответов, чтобы быстро проверить, есть ли устройство.
 */
void esp::ps2::ps2_device::quick_check_hardware()
{
    _cmd_time_out = esp::ps2::p::QUICK_CMD_TIMEOUT;
    _cmd_sub_time_out = esp::ps2::p::QUICK_CMD_SUBTIMEOUT;
}

/**
 * @brief Привязывает устройство к порту.
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::ps2_device::begin(int ps2_port)
{
    _ps2_port = ps2_port;
}

/**
 * @brief Сообщает, принят ли байт от устройства.
 * @return Ненулевое значение, если байт ждёт чтения.
 */
int esp::ps2::ps2_device::data_available()
{
    return ps2_controller::data_available(_ps2_port);
}

/**
 * @brief Читает байт от устройства.
 *
 * Ждёт короткими отрезками, захватывая порт на каждый, чтобы между ними
 * могли пройти команды других задач. Ошибка приёма прерывает ожидание.
 *
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return Принятый байт или −1, если его нет.
 */
int esp::ps2::ps2_device::get_data(int time_out_ms)
{
    constexpr int INTER_GETDATA_TIMEOUT_MS = 100;
    constexpr int INTER_GETDATA_PAUSE_MS = 10;

    const int inter_time_out = time_out_ms > -1 ? std::min(time_out_ms, INTER_GETDATA_TIMEOUT_MS)
                                                : INTER_GETDATA_TIMEOUT_MS;

    int ret = -1;
    const esp::ps2::time_out timeout;
    while (true)
    {
        lock(-1);
        ret = ps2_controller::get_data(_ps2_port, inter_time_out);
        unlock();
        if (ret > -1 || parity_error() || sync_error() || clk_time_out_error()
            || timeout.expired(time_out_ms))
            break;
        // Даём другим задачам передать свои команды.
        vTaskDelay(INTER_GETDATA_PAUSE_MS / portTICK_PERIOD_MS);
    }
    return ret;
}

/**
 * @brief Просит устройство повторить последний байт.
 */
void esp::ps2::ps2_device::request_to_resend_last_byte()
{
    ps2_controller::send_data(esp::ps2::p::CMD_RESEND_LAST_BYTE, _ps2_port);
}

/**
 * @brief Включает и выключает светодиоды клавиатуры.
 * @param num_lock Светодиод NUM LOCK.
 * @param caps_lock Светодиод CAPS LOCK.
 * @param scroll_lock Светодиод SCROLL LOCK.
 * @return @c true, если клавиатура подтвердила обе части команды.
 */
bool esp::ps2::ps2_device::send_cmd_leds(bool num_lock, bool caps_lock, bool scroll_lock)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    return send_command(esp::ps2::p::CMD_SET_LEDS, esp::ps2::p::REPLY_ACK)
        && send_command((scroll_lock << 0) | (num_lock << 1) | (caps_lock << 2),
                        esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Проверяет связь командой «эхо».
 * @return @c true, если устройство ответило.
 */
bool esp::ps2::ps2_device::send_cmd_echo()
{
    return send_command(esp::ps2::p::CMD_ECHO, esp::ps2::p::REPLY_ECHO);
}

/**
 * @brief Узнаёт текущий набор скан-кодов клавиатуры.
 * @param result Куда записать номер набора.
 * @return @c true, если клавиатура ответила.
 */
bool esp::ps2::ps2_device::send_cmd_get_scancode_set(std::uint8_t *result)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_GET_SET_SCANCODE_SET, esp::ps2::p::REPLY_ACK))
        return false;
    if (!send_command(0, esp::ps2::p::REPLY_ACK))
        return false;
    *result = get_data(_cmd_time_out);
    return (*result >= 1 || *result <= 3);
}

/**
 * @brief Задаёт набор скан-кодов клавиатуры.
 * @param scancode_set Номер набора: 1, 2 или 3.
 * @return @c true, если клавиатура подтвердила команду.
 */
bool esp::ps2::ps2_device::send_cmd_set_scancode_set(std::uint8_t scancode_set)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_GET_SET_SCANCODE_SET, esp::ps2::p::REPLY_ACK))
        return false;
    return send_command(scancode_set, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Опознаёт устройство по двум байтам ответа.
 *
 * На время опознания сканирование выключается. Код устройства
 * запоминается (device_id()).
 *
 * @param result Куда записать тип устройства; записывается всегда.
 * @return @c true, если сканирование снова включилось.
 */
bool esp::ps2::ps2_device::send_cmd_identify(ps2_device_type *result)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    *result = ps2_device_type::unknown_ps2_device;
    if (!send_cmd_disable_scanning())
        return false;
    if (!send_command(esp::ps2::p::CMD_IDENTIFY, esp::ps2::p::REPLY_ACK))
        return false;
    const int b1 = get_data(_cmd_time_out);
    const int b2 = get_data(_cmd_time_out);
    _device_id = static_cast<std::uint8_t>(b1) | (static_cast<std::uint8_t>(b2) << 8);
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

/**
 * @brief Выключает сканирование: устройство перестаёт присылать данные.
 * @return @c true, если устройство подтвердило команду.
 */
bool esp::ps2::ps2_device::send_cmd_disable_scanning()
{
    return send_command(esp::ps2::p::CMD_DISABLE_SCANNING, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Включает сканирование.
 * @return @c true, если устройство подтвердило команду.
 */
bool esp::ps2::ps2_device::send_cmd_enable_scanning()
{
    return send_command(esp::ps2::p::CMD_ENABLE_SCANNING, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Задаёт скорость и задержку автоповтора клавиатуры.
 *
 * Период выбирается из таблицы — первый не меньше заданного; задержка
 * округляется вниз до шага 250 мс.
 *
 * @param repeat_rate_ms Период автоповтора, от 33 до 500 мс.
 * @param repeat_delay_ms Задержка перед автоповтором, от 250 до 1000 мс.
 * @return @c true, если клавиатура подтвердила обе части команды.
 */
bool esp::ps2::ps2_device::send_cmd_typematic_rate_and_delay(int repeat_rate_ms,
                                                             int repeat_delay_ms)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_SET_TYPEMATIC_RATE_AND_DELAY, esp::ps2::p::REPLY_ACK))
        return false;
    std::uint8_t byte_to_send = 0b01011; // По умолчанию 10,9 повтора в секунду (91 мс).
    for (int i = 0; i < 32; ++i)
    {
        if (esp::ps2::p::REPEAT_RATES[i] >= repeat_rate_ms)
        {
            byte_to_send = i;
            break;
        }
    }
    byte_to_send |= (repeat_delay_ms / 250 - 1) << 5;
    return send_command(byte_to_send, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Задаёт частоту опроса мыши.
 * @param sample_rate Отсчётов в секунду: 10, 20, 40, 60, 80, 100 или 200.
 * @return @c true, если мышь подтвердила обе части команды.
 */
bool esp::ps2::ps2_device::send_cmd_set_sample_rate(int sample_rate)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_SET_SAMPLE_RATE, esp::ps2::p::REPLY_ACK))
        return false;
    return send_command(sample_rate, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Возвращает устройству параметры по умолчанию.
 * @return @c true, если устройство подтвердило команду.
 */
bool esp::ps2::ps2_device::send_cmd_set_default_params()
{
    return send_command(esp::ps2::p::CMD_SET_DEFAULT_PARAMS, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Сбрасывает устройство и ждёт итога самопроверки.
 * @return @c true, если самопроверка пройдена за 500 мс.
 */
bool esp::ps2::ps2_device::send_cmd_reset()
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_RESET, esp::ps2::p::REPLY_ACK))
        return false;
    // 500 мс устройству хватает на сброс и самопроверку.
    return get_data(500) == esp::ps2::p::REPLY_SELFTEST_OK;
}

/**
 * @brief Задаёт разрешение мыши.
 * @param resolution 0 — 1 отсчёт на мм, 1 — 2, 2 — 4, 3 — 8.
 * @return @c true, если мышь подтвердила обе части команды.
 */
bool esp::ps2::ps2_device::send_cmd_set_resolution(int resolution)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_SET_RESOLUTION, esp::ps2::p::REPLY_ACK))
        return false;
    return send_command(resolution, esp::ps2::p::REPLY_ACK);
}

/**
 * @brief Задаёт масштабирование перемещений мыши.
 * @param scaling 1 — 1:1, 2 — 1:2.
 * @return @c true, если мышь подтвердила обе части команды.
 */
bool esp::ps2::ps2_device::send_cmd_set_scaling(int scaling)
{
    const esp::ps2::ps2_device_lock device_lock{this};
    if (!send_command(esp::ps2::p::CMD_SET_SCALING, esp::ps2::p::REPLY_ACK))
        return false;
    return send_command(scaling, esp::ps2::p::REPLY_ACK);
}
