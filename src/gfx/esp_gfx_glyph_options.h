// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Параметры рисования знаков esp::gfx::glyph_options.
 */


#include <cstdint>

#include <esp_attr.h>


namespace esp::gfx
{

/**
 * @brief Параметры рисования знаков: фон, начертание, инверсия.
 *
 * Отдельные флаги задают поля структуры, все сразу — поле @c value (0 —
 * все флаги сброшены). Методы set_…() меняют один флаг и возвращают
 * ссылку на объект, поэтому их можно вызывать цепочкой:
 * @code
 * canvas.set_glyph_options(esp::gfx::glyph_options().set_fill_background(true).set_bold(true));
 * @endcode
 * Параметры действуют на примитивы, которые холст ставит в очередь
 * после esp::gfx::canvas::set_glyph_options().
 */
union glyph_options
{
    inline glyph_options &set_fill_background(bool value);
    inline glyph_options &set_bold(bool value);
    inline glyph_options &set_italic(bool value);
    inline glyph_options &set_underline(bool value);
    inline glyph_options &set_double_width(std::uint8_t value);
    inline glyph_options &set_invert(std::uint8_t value);
    inline glyph_options &set_blank(std::uint8_t value);

    struct
    {
        /** @brief Заливать фон знакоместа цветом кисти; иначе фон прозрачен. */
        std::uint16_t fill_background : 1;

        /**
         * @brief Полужирное начертание.
         *
         * За каждой точкой знака справа рисуется ещё одна, цветом вполовину
         * яркости пера.
         */
        std::uint16_t bold : 1;

        /** @brief Пониженная яркость: каналы цвета пера ограничиваются значением 128. */
        std::uint16_t reduce_luminosity : 1;

        /**
         * @brief Курсив.
         *
         * Верхняя треть знака сдвигается вправо на две точки, средняя — на
         * одну. При заданной области отсечения курсив рисуется неточно.
         */
        std::uint16_t italic : 1;

        /**
         * @brief Поменять местами цвета пера и кисти — инверсия символа.
         *
         * Складывается по исключающему ИЛИ с флагом
         * esp::gfx::paint_options::swap_fgbg.
         */
        std::uint16_t invert : 1;

        /** @brief Залить знакоместо цветом кисти — невидимый или мигающий символ. */
        std::uint16_t blank : 1;

        /** @brief Подчеркнуть знак линией на строке @c esp::gfx::UNDERLINE_POSITION снизу. */
        std::uint16_t underline : 1;

        /**
         * @brief Двойная ширина и высота.
         *
         * 0 — обычный знак, 1 — двойная ширина, 2 — двойная ширина и верхняя
         * половина двойной высоты, 3 — двойная ширина и нижняя половина
         * двойной высоты.
         */
        std::uint16_t double_width : 2;

        /** @brief Флаг для нужд приложения; при рисовании не используется. */
        std::uint16_t user_opt_1 : 1;

        /** @brief Флаг для нужд приложения; при рисовании не используется. */
        std::uint16_t user_opt_2 : 1;
    };

    /** @brief Все флаги сразу: 0 — обычные непрозрачные знаки без украшений. */
    std::uint16_t value;
} PACKED_ATTR;

}


// Реализация

/**
 * @brief Задаёт заливку фона знакоместа цветом кисти.
 * @param value @c true — заливать фон.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_fill_background(bool value)
{
    fill_background = value;
    return *this;
}

/**
 * @brief Задаёт полужирное начертание.
 * @param value @c true — полужирный.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_bold(bool value)
{
    bold = value;
    return *this;
}

/**
 * @brief Задаёт курсив.
 * @param value @c true — курсив.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_italic(bool value)
{
    italic = value;
    return *this;
}

/**
 * @brief Задаёт подчёркивание.
 * @param value @c true — подчёркивать.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_underline(bool value)
{
    underline = value;
    return *this;
}

/**
 * @brief Задаёт двойную ширину и высоту.
 * @param value От 0 до 3, см. @c double_width.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_double_width(std::uint8_t value)
{
    double_width = value;
    return *this;
}

/**
 * @brief Задаёт инверсию цветов пера и кисти.
 * @param value Ненулевое значение — инверсия.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_invert(std::uint8_t value)
{
    invert = value;
    return *this;
}

/**
 * @brief Задаёт заливку знакоместа цветом кисти вместо знака.
 * @param value Ненулевое значение — знак невидим.
 * @return Этот объект.
 */
esp::gfx::glyph_options &esp::gfx::glyph_options::set_blank(std::uint8_t value)
{
    blank = value;
    return *this;
}
