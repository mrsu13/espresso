// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Знак для рисования esp::gfx::glyph.
 */


#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Знак: однобитное изображение и место, где его нарисовать.
 *
 * Изображение — строки по целому числу байтов, старший разряд байта —
 * левая точка; 1 — точка переднего плана (цвет пера), 0 — фон (цвет кисти
 * или прозрачность, см. @c esp::gfx::glyph_options). Так рисуются символы
 * шрифта @c esp::gfx::font. Структура упакована, потому что входит в
 * примитив рисования @c esp::gfx::primitive.
 */
struct glyph
{
    inline glyph();
    inline glyph(int x, int y, int width, int height, const std::uint8_t *data);

    /** @brief Левая граница знака, пиксели, относительно начала координат холста. */
    std::int16_t x;

    /** @brief Верхняя граница знака, пиксели, относительно начала координат холста. */
    std::int16_t y;

    /** @brief Ширина знака, точки. */
    std::uint8_t width;

    /** @brief Высота знака, точки. */
    std::uint8_t height;

    /**
     * @brief Изображение знака.
     *
     * @c height строк по (@c width + 7) / 8 байтов, старший разряд байта —
     * левая точка. Данные не копируются и должны существовать, пока
     * примитив не нарисован.
     */
    const std::uint8_t *data;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Создаёт пустой знак без изображения.
 */
esp::gfx::glyph::glyph()
    : x{0}
    , y{0}
    , width{0}
    , height{0}
    , data{nullptr}
{
}

/**
 * @brief Создаёт знак.
 * @param x Левая граница.
 * @param y Верхняя граница.
 * @param width Ширина, точки; до 255.
 * @param height Высота, точки; до 255.
 * @param data Изображение знака.
 */
esp::gfx::glyph::glyph(int x, int y, int width, int height, const std::uint8_t *data)
    : x{static_cast<std::int16_t>(x)}
    , y{static_cast<std::int16_t>(y)}
    , width{static_cast<std::uint8_t>(width)}
    , height{static_cast<std::uint8_t>(height)}
    , data{data}
{
}
