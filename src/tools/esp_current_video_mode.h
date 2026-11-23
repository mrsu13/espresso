// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_video_mode.h"


namespace esp
{

class current_video_mode
{

public:

    current_video_mode() = delete;

    inline static esp::video_mode get();
    inline static void set(esp::video_mode value);

private:

    /** @brief Текущий видеовыход; esp::video_mode::none, пока его не настроили. */
    static esp::video_mode _video_mode;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает текущий видеовыход.
 * @return Видеовыход; esp::video_mode::none, если он ещё не настроен.
 */
esp::video_mode esp::current_video_mode::get()
{
    return _video_mode;
}

/**
 * @brief Задаёт текущий видеовыход; его вызывает драйвер видеовыхода при настройке.
 * @param value Видеовыход.
 */
void esp::current_video_mode::set(esp::video_mode value)
{
    _video_mode = value;
}
