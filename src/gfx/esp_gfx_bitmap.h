// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_pixel_format.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_rgba2222.h"
#include "esp_gfx_rgba8888.h"
#include "esp_non_copyable.h"

#include <cstdint>


namespace esp::gfx
{

struct bitmap : private esp::non_copyable
{
    inline bitmap();
    bitmap(int width, int height, const void *data, pixel_format format, bool copy = false);
    bitmap(int width, int height, const void *data, pixel_format format, rgb888 foreground_color,
           bool copy = false);
    ~bitmap();

    void set_pixel(int x, int y, int value);
    void set_pixel(int x, int y, rgba2222 value);
    void set_pixel(int x, int y, rgba8888 value);

    /** @brief Ширина растра, пиксели. */
    std::int16_t width;

    /** @brief Высота растра, пиксели. */
    std::int16_t height;

    /** @brief Формат пикселей; от него зависят размер @c data и способ рисования. */
    pixel_format format;

    /**
     * @brief Цвет непрозрачных пикселей маски.
     *
     * Используется только в формате @c esp::gfx::pixel_format::mask; по
     * умолчанию белый.
     */
    rgb888 foreground_color;

    /**
     * @brief Пиксели растра в формате @c format.
     *
     * Строки идут сверху вниз без промежутков. Растр владеет данными, только
     * если @c data_allocated равно @c true; иначе это чужая память,
     * например константа во флеш-памяти, и менять её через set_pixel()
     * нельзя.
     */
    std::uint8_t *data;

    /** @brief Выделил ли растр @c data сам; тогда деструктор их освобождает. */
    bool data_allocated;

private:

    void allocate();
    void copy_from(const void *src_data);
};

}


// Реализация

/**
 * @brief Создаёт пустой растр без данных и без формата.
 */
esp::gfx::bitmap::bitmap()
    : width{0}
    , height{0}
    , format{pixel_format::undefined}
    , foreground_color{rgb888(255, 255, 255)}
    , data{nullptr}
    , data_allocated{false}
{
}
