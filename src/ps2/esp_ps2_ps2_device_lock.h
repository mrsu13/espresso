// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Захват устройства PS/2 на время блока esp::ps2::ps2_device_lock.
 */


#include "esp_non_copyable.h"
#include "esp_ps2_ps2_device.h"


namespace esp::ps2
{

/**
 * @brief Захватывает устройство PS/2 на время жизни объекта.
 *
 * Конструктор ждёт захвата без ограничения времени, деструктор освобождает
 * устройство — так последовательность команд не перемешается с командами
 * других задач.
 */
class ps2_device_lock : esp::non_copyable
{

public:

    inline explicit ps2_device_lock(esp::ps2::ps2_device *device);
    inline ~ps2_device_lock();

private:

    /** @brief Захваченное устройство. */
    esp::ps2::ps2_device *_device;
};

}


// Реализация

// Открытые

/**
 * @brief Захватывает устройство, ожидая сколько потребуется.
 * @param device Устройство; должно существовать дольше объекта захвата.
 */
esp::ps2::ps2_device_lock::ps2_device_lock(esp::ps2::ps2_device *device)
    : _device{device}
{
    _device->lock(-1);
}

/**
 * @brief Освобождает устройство.
 */
esp::ps2::ps2_device_lock::~ps2_device_lock()
{
    _device->unlock();
}
