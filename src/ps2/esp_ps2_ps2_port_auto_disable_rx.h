// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Запрет приёма с порта PS/2 на время блока esp::ps2::ps2_port_auto_disable_rx.
 */


#include "esp_non_copyable.h"
#include "esp_ps2_ps2_controller.h"


namespace esp::ps2
{

/**
 * @brief Запрещает приём с порта PS/2 на время жизни объекта.
 *
 * Пока одно устройство получает команду, другое не должно ничего
 * присылать: конструктор держит линию CLK его порта низкой, деструктор
 * отпускает её.
 */
class ps2_port_auto_disable_rx : esp::non_copyable
{

public:

    inline explicit ps2_port_auto_disable_rx(int ps2_port);
    inline ~ps2_port_auto_disable_rx();

private:

    /** @brief Порт, приём с которого запрещён: 0 или 1. */
    int _port;
};

}


// Реализация

// Открытые

/**
 * @brief Запрещает приём с порта.
 * @param ps2_port Порт: 0 или 1.
 */
esp::ps2::ps2_port_auto_disable_rx::ps2_port_auto_disable_rx(int ps2_port)
    : _port{ps2_port}
{
    esp::ps2::ps2_controller::disable_rx(_port);
}

/**
 * @brief Снова разрешает приём с порта.
 */
esp::ps2::ps2_port_auto_disable_rx::~ps2_port_auto_disable_rx()
{
    esp::ps2::ps2_controller::enable_rx(_port);
}
