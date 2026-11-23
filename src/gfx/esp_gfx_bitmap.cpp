// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_bitmap.h
 * @brief Объявление растра esp::gfx::bitmap.
 */

/**
 * @file esp_gfx_bitmap.cpp
 * @brief Растр: данные, выделение памяти, запись пикселей.
 */

#include "esp_gfx_bitmap.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <esp_heap_caps.h>


#pragma GCC optimize("O2")


/**
 * @struct esp::gfx::bitmap
 * @brief Растр: прямоугольное изображение в одном из форматов @c esp::gfx::pixel_format.
 *
 * Растр рисуется холстом (esp::gfx::canvas::draw_bitmap()), служит кадром
 * спрайта и изображением курсора мыши. Данные либо чужие (растр только
 * ссылается на них), либо скопированы в собственную память растра, которую
 * освобождает деструктор. Поэтому растр нельзя копировать: копия
 * освободила бы те же данные второй раз.
 */


// Открытые

/**
 * @brief Создаёт растр; маска получает белый цвет.
 *
 * @param width Ширина, пиксели.
 * @param height Высота, пиксели.
 * @param data Пиксели в формате @p format.
 * @param format Формат пикселей.
 * @param copy @c true — скопировать данные в собственную память растра;
 *        @c false — ссылаться на них, тогда они должны существовать, пока
 *        существует растр.
 */
esp::gfx::bitmap::bitmap(int width, int height, const void *data, pixel_format format, bool copy)
    : bitmap(width, height, data, format, rgb888(255, 255, 255), copy)
{
}

/**
 * @brief Создаёт растр.
 *
 * @param width Ширина, пиксели.
 * @param height Высота, пиксели.
 * @param data Пиксели в формате @p format.
 * @param format Формат пикселей.
 * @param foreground_color Цвет непрозрачных пикселей маски.
 * @param copy @c true — скопировать данные в собственную память растра;
 *        @c false — ссылаться на них, тогда они должны существовать, пока
 *        существует растр.
 */
esp::gfx::bitmap::bitmap(int width, int height, const void *data, pixel_format format,
                         rgb888 foreground_color, bool copy)
    : width{static_cast<std::int16_t>(width)}
    , height{static_cast<std::int16_t>(height)}
    , format{format}
    , foreground_color{foreground_color}
    , data{static_cast<std::uint8_t *>(const_cast<void *>(data))}
    , data_allocated{false}
{
    if (copy)
    {
        allocate();
        copy_from(data);
    }
}

/**
 * @brief Освобождает данные, если растр выделил их сам.
 */
esp::gfx::bitmap::~bitmap()
{
    if (data_allocated)
        heap_caps_free(data);
}

/**
 * @brief Задаёт пиксель маски.
 *
 * Только для формата @c esp::gfx::pixel_format::mask и собственных данных
 * растра.
 *
 * @param x Столбец.
 * @param y Строка.
 * @param value 0 — прозрачный пиксель, иначе непрозрачный.
 */
void esp::gfx::bitmap::set_pixel(int x, int y, int value)
{
    const int rowlen = (width + 7) / 8;
    std::uint8_t *const rowptr = data + y * rowlen;
    if (value)
        rowptr[x >> 3] |= 0x80 >> (x & 7);
    else
        rowptr[x >> 3] &= ~(0x80 >> (x & 7));
}

/**
 * @brief Задаёт пиксель растра формата @c esp::gfx::pixel_format::rgba2222.
 * @param x Столбец.
 * @param y Строка.
 * @param value Цвет с прозрачностью.
 */
void esp::gfx::bitmap::set_pixel(int x, int y, rgba2222 value)
{
    reinterpret_cast<rgba2222 *>(data)[y * width + x] = value;
}

/**
 * @brief Задаёт пиксель растра формата @c esp::gfx::pixel_format::rgba8888.
 * @param x Столбец.
 * @param y Строка.
 * @param value Цвет с прозрачностью.
 */
void esp::gfx::bitmap::set_pixel(int x, int y, rgba8888 value)
{
    reinterpret_cast<rgba8888 *>(data)[y * width + x] = value;
}

// Закрытые

/**
 * @brief Выделяет собственную память под пиксели по формату и размеру растра.
 *
 * Прежние собственные данные освобождаются. Память берётся из внутренней
 * ОЗУ; для форматов @c esp::gfx::pixel_format::undefined и
 * @c esp::gfx::pixel_format::native память не выделяется.
 */
void esp::gfx::bitmap::allocate()
{
    if (data_allocated)
    {
        std::free(data);
        data = nullptr;
    }
    data_allocated = true;
    switch (format)
    {
        case pixel_format::undefined:
        case pixel_format::native:
            break;

        case pixel_format::mask:
            data = static_cast<std::uint8_t *>(
                heap_caps_malloc((width + 7) / 8 * height, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
            break;

        case pixel_format::rgba2222:
            data = static_cast<std::uint8_t *>(
                heap_caps_malloc(width * height, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
            break;

        case pixel_format::rgba8888:
            data = static_cast<std::uint8_t *>(
                heap_caps_malloc(width * height * 4, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
            break;
    }
}

/**
 * @brief Копирует пиксели в собственную память растра.
 * @param src_data Пиксели в формате растра; копируется столько байтов, сколько занимает растр.
 */
void esp::gfx::bitmap::copy_from(const void *src_data)
{
    switch (format)
    {
        case pixel_format::undefined:
        case pixel_format::native:
            break;

        case pixel_format::mask:
            std::memcpy(data, src_data, (width + 7) / 8 * height);
            break;

        case pixel_format::rgba2222:
            std::memcpy(data, src_data, width * height);
            break;

        case pixel_format::rgba8888:
            std::memcpy(data, src_data, width * height * 4);
            break;
    }
}
