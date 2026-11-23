// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


namespace esp
{

class core_usage
{

public:

    core_usage() = delete;

    inline static int quiet_core();
    inline static void set_busiest_core(int core);

private:

    /**
     * @brief Самое загруженное ядро: 0 или 1; -1 — не задано.
     *
     * По умолчанию это esp::VIDEO_TASKS_CORE; контроллеры VGA задают его же,
     * прежде чем поставить на него обработчик прерывания.
     */
    static int _busiest_core;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает менее загруженное ядро — другое, чем задано set_busiest_core().
 *
 * На него ставят то, что не должно мешать видеовыводу: задачу примитивов
 * esp::vga::vga_paletted_controller и обработчик прерывания
 * esp::sound::sound_generator.
 *
 * @return Ядро: 0 или 1; -1, если самое загруженное ядро не задано.
 */
int esp::core_usage::quiet_core()
{
    return _busiest_core != -1 ? _busiest_core ^ 1 : -1;
}

/**
 * @brief Задаёт самое загруженное ядро.
 * @param core Ядро: 0 или 1; -1 — не задано.
 */
void esp::core_usage::set_busiest_core(int core)
{
    _busiest_core = core;
}
