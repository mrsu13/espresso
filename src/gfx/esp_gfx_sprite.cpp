// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_sprite.h
 * @brief Объявление спрайта esp::gfx::sprite.
 */

/**
 * @file esp_gfx_sprite.cpp
 * @brief Спрайт: кадры и перемещение.
 */

#include "esp_gfx_sprite.h"

#include <cstdlib>


#pragma GCC optimize("O2")


/**
 * @struct esp::gfx::sprite
 * @brief Спрайт: изображение поверх экрана, которое не портит фон.
 *
 * Спрайт показывает один из своих кадров (растров @c esp::gfx::bitmap) в
 * заданном месте экрана. Прежде чем нарисовать спрайт, контроллер дисплея
 * сохраняет фон под ним, а перед любым рисованием в этой области
 * восстанавливает его, поэтому спрайт можно двигать, не перерисовывая
 * экран. Курсор мыши — тоже спрайт.
 *
 * Приложение может наследовать от спрайта свою структуру с дополнительными
 * полями и передать их массив в
 * esp::gfx::bitmapped_display_controller::set_sprites(). Методы изменения
 * возвращают указатель на спрайт, поэтому их можно вызывать цепочкой. После
 * изменения спрайта нужно вызвать
 * esp::gfx::bitmapped_display_controller::refresh_sprites().
 *
 * Спрайт владеет массивом кадров и буфером фона, поэтому копировать его
 * нельзя.
 */


// Открытые

/**
 * @brief Создаёт видимый спрайт без кадров в точке (0, 0).
 */
esp::gfx::sprite::sprite()
    : x{0}
    , y{0}
    , frames{nullptr}
    , frames_count{0}
    , current_frame{0}
    , saved_x{0}
    , saved_y{0}
    , saved_background_width{0}
    , saved_background_height{0}
    // Буфер фона выделяет контроллер дисплея, когда спрайты устанавливаются.
    , saved_background{nullptr}
{
    visible = true;
    is_static = false;
    allow_draw = true;
}

/**
 * @brief Освобождает массив кадров и буфер фона; сами растры не трогает.
 */
esp::gfx::sprite::~sprite()
{
    std::free(frames);
    std::free(saved_background);
}

/**
 * @brief Добавляет кадр в конец списка кадров.
 * @param bitmap Растр кадра; должен существовать, пока принадлежит спрайту.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::add_bitmap(esp::gfx::bitmap *bitmap)
{
    ++frames_count;
    frames = static_cast<esp::gfx::bitmap **>(
        std::realloc(frames, sizeof(esp::gfx::bitmap *) * frames_count));
    frames[frames_count - 1] = bitmap;
    return this;
}

/**
 * @brief Добавляет несколько кадров в конец списка кадров.
 * @param bitmap Массив растров; растры должны существовать, пока принадлежат спрайту.
 * @param count Число растров в массиве.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::add_bitmap(esp::gfx::bitmap *bitmap[], int count)
{
    frames = static_cast<esp::gfx::bitmap **>(
        std::realloc(frames, sizeof(esp::gfx::bitmap *) * (frames_count + count)));
    for (int i = 0; i < count; ++i)
        frames[frames_count + i] = bitmap[i];
    frames_count += count;
    return this;
}

/**
 * @brief Убирает все кадры; растры не освобождаются.
 */
void esp::gfx::sprite::clear_bitmaps()
{
    std::free(frames);
    frames = nullptr;
    frames_count = 0;
}

/**
 * @brief Сдвигает спрайт.
 * @param offset_x Сдвиг по горизонтали.
 * @param offset_y Сдвиг по вертикали.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::move_by(int offset_x, int offset_y)
{
    x += offset_x;
    y += offset_y;
    return this;
}

/**
 * @brief Сдвигает спрайт с переходом через края области.
 *
 * Спрайт, целиком ушедший за правый или нижний край, появляется из-за
 * левого или верхнего, и наоборот.
 *
 * @param offset_x Сдвиг по горизонтали.
 * @param offset_y Сдвиг по вертикали.
 * @param wrap_around_width Ширина области.
 * @param wrap_around_height Высота области.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::move_by(int offset_x, int offset_y, int wrap_around_width,
                                            int wrap_around_height)
{
    x += offset_x;
    y += offset_y;
    if (x > wrap_around_width)
        x = -width();
    if (x < -width())
        x = wrap_around_width;
    if (y > wrap_around_height)
        y = -height();
    if (y < -height())
        y = wrap_around_height;
    return this;
}

/**
 * @brief Переносит спрайт в заданное место.
 * @param x Левая граница.
 * @param y Верхняя граница.
 * @return Этот спрайт.
 */
esp::gfx::sprite *esp::gfx::sprite::move_to(int x, int y)
{
    this->x = x;
    this->y = y;
    return this;
}
