// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include <cstdint>


namespace esp::ps2
{

class time_out
{

public:

    time_out();

    bool expired(int value_ms) const;

private:

    /** @brief Момент создания, микросекунды от запуска (esp_timer_get_time()). */
    std::int64_t _start;
};

}
