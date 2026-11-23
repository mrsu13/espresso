// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_bitmap.h"
#include "esp_non_copyable.h"

#include <cstdint>


namespace esp::gfx
{

struct sprite : private esp::non_copyable
{
    sprite();
    ~sprite();

    inline esp::gfx::bitmap *frame() const;
    inline int frame_index() const;
    inline void next_frame();
    inline sprite *set_frame(int frame);
    sprite *add_bitmap(esp::gfx::bitmap *bitmap);
    sprite *add_bitmap(esp::gfx::bitmap *bitmap[], int count);
    void clear_bitmaps();

    inline int width() const;
    inline int height() const;

    sprite *move_by(int offset_x, int offset_y);
    sprite *move_by(int offset_x, int offset_y, int wrap_around_width, int wrap_around_height);
    sprite *move_to(int x, int y);

    /**
     * @brief Левая граница спрайта на экране, пиксели.
     *
     * @c volatile: положение меняет задача приложения, пока спрайты рисует
     * обработчик кадровой развёртки.
     */
    volatile std::int16_t x;

    /**
     * @brief Верхняя граница спрайта на экране, пиксели.
     *
     * @c volatile: положение меняет задача приложения, пока спрайты рисует
     * обработчик кадровой развёртки.
     */
    volatile std::int16_t y;

    /**
     * @brief Кадры спрайта: массив указателей на растры.
     *
     * Массив выделяется @c realloc() при добавлении кадров и освобождается
     * деструктором; сами растры принадлежат вызывающему и должны
     * существовать, пока спрайт их показывает. Кадры могут быть разного
     * размера.
     */
    esp::gfx::bitmap **frames;

    /** @brief Число кадров в @c frames. */
    std::int16_t frames_count;

    /** @brief Номер показываемого кадра, от 0 до @c frames_count − 1. */
    std::int16_t current_frame;

    /** @brief Левая граница места, где спрайт нарисован в последний раз; там лежит сохранённый фон. */
    std::int16_t saved_x;

    /** @brief Верхняя граница места, где спрайт нарисован в последний раз; там лежит сохранённый фон. */
    std::int16_t saved_y;

    /** @brief Ширина сохранённого фона; 0 — фон не сохранён, спрайт не нарисован. */
    std::int16_t saved_background_width;

    /** @brief Высота сохранённого фона; 0 — фон не сохранён, спрайт не нарисован. */
    std::int16_t saved_background_height;

    /**
     * @brief Пиксели экрана под спрайтом в собственном формате контроллера.
     *
     * Буфер выделяет контроллер дисплея (@c realloc()) по самому большому
     * кадру, когда спрайты устанавливаются; освобождает деструктор
     * спрайта. В режиме двойной буферизации фон не сохраняется.
     */
    std::uint8_t *saved_background;

    struct
    {
        /** @brief Показывать ли спрайт; по умолчанию 1. */
        std::uint8_t visible : 1;

        /**
         * @brief Статический спрайт: рисуется один раз и не перерисовывается.
         *
         * После рисования контроллер сбрасывает у него @c allow_draw;
         * чтобы нарисовать снова, @c allow_draw нужно установить. Статические
         * спрайты ставят в списке раньше динамических.
         */
        std::uint8_t is_static : 1;

        /** @brief Можно ли рисовать спрайт: у динамических всегда 1, у статических сбрасывается после рисования. */
        std::uint8_t allow_draw : 1;
    };
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает показываемый кадр.
 * @return Растр текущего кадра; @c nullptr, если кадров нет.
 */
esp::gfx::bitmap *esp::gfx::sprite::frame() const
{
    return frames ? frames[current_frame] : nullptr;
}

/**
 * @brief Возвращает номер показываемого кадра.
 * @return Номер кадра, с 0.
 */
int esp::gfx::sprite::frame_index() const
{
    return current_frame;
}

/**
 * @brief Переходит к следующему кадру, после последнего — к первому.
 *
 * Номер кадра меняется одной записью, поэтому рисование, идущее в это
 * время в обработчике прерывания, никогда не увидит номер, равный числу
 * кадров (исправление Dean Belfield из vdp-gl).
 */
void esp::gfx::sprite::next_frame()
{
    current_frame = (current_frame < frames_count - 1) ? current_frame + 1 : 0;
}

/**
 * @brief Выбирает показываемый кадр.
 * @param frame Номер кадра, от 0 до @c frames_count − 1.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::set_frame(int frame)
{
    current_frame = frame;
    return this;
}

/**
 * @brief Возвращает ширину показываемого кадра.
 * @return Ширина, пиксели. У спрайта должен быть хотя бы один кадр.
 */
int esp::gfx::sprite::width() const
{
    return frames[current_frame]->width;
}

/**
 * @brief Возвращает высоту показываемого кадра.
 * @return Высота, пиксели. У спрайта должен быть хотя бы один кадр.
 */
int esp::gfx::sprite::height() const
{
    return frames[current_frame]->height;
}
