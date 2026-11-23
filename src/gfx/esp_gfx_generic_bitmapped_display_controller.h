// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Шаблоны рисования для контроллеров дисплея esp::gfx::generic_bitmapped_display_controller.
 */


#include "esp_gfx_bitmap.h"
#include "esp_gfx_bitmapped_display_controller.h"
#include "esp_gfx_config.h"
#include "esp_gfx_glyph.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_pixel_desc.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_rgba8888.h"
#include "esp_gfx_size.h"
#include "esp_gfx_utils.h"
#include "esp_tools.h"

#include <alloca.h>
#include <cstdint>
#include <cstdlib>


namespace esp::gfx
{

/**
 * @brief Готовые алгоритмы рисования для растровых контроллеров дисплея.
 *
 * Шаблоны рисуют точки, линии, эллипсы, знаки, прокрутку, копирование и
 * растры с отсечением и учётом параметров рисования, а с пикселями
 * работают через функции, которые передаёт производный контроллер:
 * подготовить цвет в формате пикселя, получить строку буфера кадра,
 * прочитать и записать пиксель в строке. Так один алгоритм служит
 * контроллерам с разными форматами пикселя, а встраиваемые функции не
 * стоят вызова на каждый пиксель.
 *
 * Шаблоны выполняются и в обработчике прерывания кадровой развёртки.
 *
 * @bug generic_draw_glyph_full() для знаков двойной высоты выделяет память
 *      @c alloca() на стеке обработчика прерывания, а руководство по
 *      стилю запрещает @c alloca().
 */
class generic_bitmapped_display_controller : public bitmapped_display_controller
{

protected:

    template<typename TPreparePixel, typename TRawSetPixel>
    inline void generic_set_pixel_at(const pixel_desc &pixel_desc, esp::gfx::rect &update_rect,
                                     TPreparePixel prepare_pixel, TRawSetPixel raw_set_pixel);

    template<typename TPreparePixel, typename TRawFillRow, typename TRawInvertRow,
             typename TRawSetPixel, typename TRawInvertPixel>
    inline void generic_abs_draw_line(int x1, int y1, int x2, int y2, const rgb888 &color,
                                      TPreparePixel prepare_pixel, TRawFillRow raw_fill_row,
                                      TRawInvertRow raw_invert_row, TRawSetPixel raw_set_pixel,
                                      TRawInvertPixel raw_invert_pixel);

    template<typename TPreparePixel, typename TRawSetPixel>
    inline void generic_draw_ellipse(const size &size, esp::gfx::rect &update_rect,
                                     TPreparePixel prepare_pixel, TRawSetPixel raw_set_pixel);

    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    inline void generic_draw_glyph(const glyph &glyph, glyph_options glyph_options,
                                   rgb888 pen_color, rgb888 brush_color,
                                   esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
                                   TRawGetRow raw_get_row, TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    inline void generic_draw_glyph_full(const glyph &glyph, glyph_options glyph_options,
                                        rgb888 pen_color, rgb888 brush_color,
                                        esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
                                        TRawGetRow raw_get_row,
                                        TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    inline void generic_draw_glyph_light(const glyph &glyph, glyph_options glyph_options,
                                         rgb888 pen_color, rgb888 brush_color,
                                         esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
                                         TRawGetRow raw_get_row,
                                         TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawInvertRow>
    inline void generic_invert_rect(const rect &rect, esp::gfx::rect &update_rect,
                                    TRawInvertRow raw_invert_row);

    template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
             typename TRawSetPixelInRow>
    inline void generic_swap_fgbg(const rect &rect, esp::gfx::rect &update_rect,
                                  TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                                  TRawGetPixelInRow raw_get_pixel_in_row,
                                  TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow>
    inline void generic_copy_rect(const rect &source, esp::gfx::rect &update_rect,
                                  TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
                                  TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawGetRow, typename TRawSetPixelInRow, typename TDataType>
    inline void generic_raw_draw_bitmap_native(int dest_x, int dest_y, TDataType *data, int width,
                                               int x1, int y1, int x_count, int y_count,
                                               TRawGetRow raw_get_row,
                                               TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    inline void generic_raw_draw_bitmap_mask(int dest_x, int dest_y, const bitmap *bitmap,
                                             TBackground *save_background, int x1, int y1,
                                             int x_count, int y_count, TRawGetRow raw_get_row,
                                             TRawGetPixelInRow raw_get_pixel_in_row,
                                             TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    inline void generic_raw_draw_bitmap_rgba2222(int dest_x, int dest_y, const bitmap *bitmap,
                                                 TBackground *save_background, int x1, int y1,
                                                 int x_count, int y_count, TRawGetRow raw_get_row,
                                                 TRawGetPixelInRow raw_get_pixel_in_row,
                                                 TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    inline void generic_raw_draw_bitmap_rgba8888(int dest_x, int dest_y, const bitmap *bitmap,
                                                 TBackground *save_background, int x1, int y1,
                                                 int x_count, int y_count, TRawGetRow raw_get_row,
                                                 TRawGetPixelInRow raw_get_pixel_in_row,
                                                 TRawSetPixelInRow raw_set_pixel_in_row);

    template<typename TRawCopyRow, typename TRawFillRow>
    inline void generic_v_scroll(int scroll, esp::gfx::rect &update_rect, TRawCopyRow raw_copy_row,
                                 TRawFillRow raw_fill_row);

    template<typename TSwapRowsCopying, typename TSwapRowsPointers, typename TRawFillRow>
    inline void generic_v_scroll(int scroll, esp::gfx::rect &update_rect,
                                 TSwapRowsCopying swap_rows_copying,
                                 TSwapRowsPointers swap_rows_pointers, TRawFillRow raw_fill_row);

    template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
             typename TRawSetPixelInRow>
    inline void generic_h_scroll(int scroll, esp::gfx::rect &update_rect,
                                 TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                                 TRawGetPixelInRow raw_get_pixel_in_row,
                                 TRawSetPixelInRow raw_set_pixel_in_row);
};

}


// Реализация

// Защищённые

/**
 * @brief Рисует точку заданного цвета, если она внутри области отсечения.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawSetPixel Функция «записать пиксель (x, y, значение)».
 * @param pixel_desc Положение относительно начала координат и цвет.
 * @param update_rect Изменённая область; расширяется на точку.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_set_pixel Запись пикселя.
 */
template<typename TPreparePixel, typename TRawSetPixel>
void esp::gfx::generic_bitmapped_display_controller::generic_set_pixel_at(
    const pixel_desc &pixel_desc, esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
    TRawSetPixel raw_set_pixel)
{
    const int x = pixel_desc.pos.x + paint_state().origin.x;
    const int y = pixel_desc.pos.y + paint_state().origin.y;

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    if (x >= clip_x1 && x <= clip_x2 && y >= clip_y1 && y <= clip_y2)
    {
        update_rect = update_rect.merge(rect(x, y, x, y));
        hide_sprites(update_rect);
        raw_set_pixel(x, y, prepare_pixel(pixel_desc.color));
    }
}

/**
 * @brief Рисует линию в абсолютных координатах с отсечением.
 *
 * Толстую линию рисует abs_draw_thick_line(). Горизонтальные и
 * вертикальные линии обрезаются и рисуются строкой или столбцом, прочие —
 * алгоритмом Брезенхэма. При включённой инверсии пиксели инвертируются.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawFillRow Функция «залить отрезок строки (y, x1, x2, значение)».
 * @tparam TRawInvertRow Функция «инвертировать отрезок строки (y, x1, x2)».
 * @tparam TRawSetPixel Функция «записать пиксель (x, y, значение)».
 * @tparam TRawInvertPixel Функция «инвертировать пиксель (x, y)».
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 * @param color Цвет.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_fill_row Заливка отрезка строки.
 * @param raw_invert_row Инверсия отрезка строки.
 * @param raw_set_pixel Запись пикселя.
 * @param raw_invert_pixel Инверсия пикселя.
 *
 * @bug Наклонная линия не отсекается точно: алгоритм Коэна — Сазерленда
 *      только проверяет, видна ли она, а каждый пиксель линии Брезенхэма
 *      проверяется отдельно. Точное отсечение без сдвига пикселей даёт
 *      алгоритм Кузьмина
 *      (https://stackoverflow.com/questions/40884680/how-to-use-bresenhams-line-drawing-algorithm-with-clipping).
 */
template<typename TPreparePixel, typename TRawFillRow, typename TRawInvertRow,
         typename TRawSetPixel, typename TRawInvertPixel>
void esp::gfx::generic_bitmapped_display_controller::generic_abs_draw_line(
    int x1, int y1, int x2, int y2, const rgb888 &color, TPreparePixel prepare_pixel,
    TRawFillRow raw_fill_row, TRawInvertRow raw_invert_row, TRawSetPixel raw_set_pixel,
    TRawInvertPixel raw_invert_pixel)
{
    if (paint_state().pen_width > 1)
    {
        abs_draw_thick_line(x1, y1, x2, y2, paint_state().pen_width, color);
        return;
    }
    auto pattern = prepare_pixel(color);
    if (y1 == y2)
    {
        // Горизонтальная линия.
        if (y1 < paint_state().abs_clipping_rect.y1 || y1 > paint_state().abs_clipping_rect.y2)
            return;
        if (x1 > x2)
            esp::tswap(x1, x2);
        if (x1 > paint_state().abs_clipping_rect.x2 || x2 < paint_state().abs_clipping_rect.x1)
            return;
        x1 =
            esp::iclamp(x1, paint_state().abs_clipping_rect.x1, paint_state().abs_clipping_rect.x2);
        x2 =
            esp::iclamp(x2, paint_state().abs_clipping_rect.x1, paint_state().abs_clipping_rect.x2);
        if (paint_state().paint_options.negate)
            raw_invert_row(y1, x1, x2);
        else
            raw_fill_row(y1, x1, x2, pattern);
    }
    else if (x1 == x2)
    {
        // Вертикальная линия.
        if (x1 < paint_state().abs_clipping_rect.x1 || x1 > paint_state().abs_clipping_rect.x2)
            return;
        if (y1 > y2)
            esp::tswap(y1, y2);
        if (y1 > paint_state().abs_clipping_rect.y2 || y2 < paint_state().abs_clipping_rect.y1)
            return;
        y1 =
            esp::iclamp(y1, paint_state().abs_clipping_rect.y1, paint_state().abs_clipping_rect.y2);
        y2 =
            esp::iclamp(y2, paint_state().abs_clipping_rect.y1, paint_state().abs_clipping_rect.y2);
        if (paint_state().paint_options.negate)
        {
            for (int y = y1; y <= y2; ++y)
                raw_invert_pixel(x1, y);
        }
        else
        {
            for (int y = y1; y <= y2; ++y)
                raw_set_pixel(x1, y, pattern);
        }
    }
    else
    {
        // Прочие линии — алгоритмом Брезенхэма. Отсечённый отрезок мог бы
        // разойтись с линией Брезенхэма на пиксель (например, у продолжения
        // уже нарисованной линии), поэтому отсечение только проверяет, видна
        // ли линия, а каждый её пиксель проверяется отдельно.
        if (!clip_line(x1, y1, x2, y2, paint_state().abs_clipping_rect, true))
            return;
        const int dx = std::abs(x2 - x1);
        const int dy = std::abs(y2 - y1);
        const int sx = x1 < x2 ? 1 : -1;
        const int sy = y1 < y2 ? 1 : -1;
        int err = (dx > dy ? dx : -dy) / 2;
        while (true)
        {
            if (paint_state().abs_clipping_rect.contains(x1, y1))
            {
                if (paint_state().paint_options.negate)
                    raw_invert_pixel(x1, y1);
                else
                    raw_set_pixel(x1, y1, pattern);
            }
            if (x1 == x2 && y1 == y2)
                break;
            const int e2 = err;
            if (e2 > -dx)
            {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dy)
            {
                err += dx;
                y1 += sy;
            }
        }
    }
}

/**
 * @brief Рисует эллипс пером с центром в текущей позиции по алгоритму Макилроя.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawSetPixel Функция «записать пиксель (x, y, значение)».
 * @param size Размеры эллипса.
 * @param update_rect Изменённая область; расширяется на эллипс.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_set_pixel Запись пикселя.
 */
template<typename TPreparePixel, typename TRawSetPixel>
void esp::gfx::generic_bitmapped_display_controller::generic_draw_ellipse(
    const size &size, esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
    TRawSetPixel raw_set_pixel)
{
    auto pattern = prepare_pixel(actual_pen_color());

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int center_x = paint_state().position.x;
    const int center_y = paint_state().position.y;

    const int half_width = size.width / 2;
    const int half_height = size.height / 2;

    update_rect = update_rect.merge(rect(center_x - half_width, center_y - half_height,
                                         center_x + half_width, center_y + half_height));
    hide_sprites(update_rect);

    const int a2 = half_width * half_width;
    const int b2 = half_height * half_height;
    const int crit1 = -(a2 / 4 + half_width % 2 + b2);
    const int crit2 = -(b2 / 4 + half_height % 2 + a2);
    const int crit3 = -(b2 / 4 + half_height % 2);
    const int d2xt = 2 * b2;
    const int d2yt = 2 * a2;
    int x = 0; // Растёт от 0 до half_width.
    int y = half_height; // Убывает от half_height до 0.
    int t = -a2 * y;
    int dxt = 2 * b2 * x;
    int dyt = -2 * a2 * y;

    while (y >= 0 && x <= half_width)
    {
        const int col1 = center_x - x;
        const int col2 = center_x + x;
        const int row1 = center_y - y;
        const int row2 = center_y + y;

        if (col1 >= clip_x1 && col1 <= clip_x2)
        {
            if (row1 >= clip_y1 && row1 <= clip_y2)
                raw_set_pixel(col1, row1, pattern);
            if (row2 >= clip_y1 && row2 <= clip_y2)
                raw_set_pixel(col1, row2, pattern);
        }
        if (col2 >= clip_x1 && col2 <= clip_x2)
        {
            if (row1 >= clip_y1 && row1 <= clip_y2)
                raw_set_pixel(col2, row1, pattern);
            if (row2 >= clip_y1 && row2 <= clip_y2)
                raw_set_pixel(col2, row2, pattern);
        }

        if (t + b2 * x <= crit1 || t + a2 * y <= crit3)
        {
            ++x;
            dxt += d2xt;
            t += dxt;
        }
        else if (t - a2 * y > crit2)
        {
            --y;
            dyt += d2yt;
            t += dyt;
        }
        else
        {
            ++x;
            dxt += d2xt;
            t += dxt;
            --y;
            dyt += d2yt;
            t += dyt;
        }
    }
}

/**
 * @brief Рисует знак быстрым или полным способом.
 *
 * Знак без начертаний (полужирного, курсива, подчёркивания, двойной
 * ширины, заливки знакоместа) шириной до 32 точек рисует быстрый
 * generic_draw_glyph_light(), остальные — generic_draw_glyph_full().
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на знак.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_get_row Получение строки.
 * @param raw_set_pixel_in_row Запись пикселя в строку.
 */
template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_draw_glyph(
    const glyph &glyph, glyph_options glyph_options, rgb888 pen_color, rgb888 brush_color,
    esp::gfx::rect &update_rect, TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    if (!glyph_options.bold && !glyph_options.italic && !glyph_options.blank
        && !glyph_options.underline && !glyph_options.double_width && glyph.width <= 32)
        generic_draw_glyph_light(glyph, glyph_options, pen_color, brush_color, update_rect,
                                 prepare_pixel, raw_get_row, raw_set_pixel_in_row);
    else
        generic_draw_glyph_full(glyph, glyph_options, pen_color, brush_color, update_rect,
                                prepare_pixel, raw_get_row, raw_set_pixel_in_row);
}

/**
 * @brief Рисует знак со всеми начертаниями.
 *
 * Поддерживает заливку знакоместа, полужирное начертание, курсив,
 * подчёркивание, пониженную яркость, инверсию, невидимость, двойную ширину
 * и двойную высоту. Знак отсекается областью отсечения.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на знак.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_get_row Получение строки.
 * @param raw_set_pixel_in_row Запись пикселя в строку.
 *
 * @bug Курсив рисуется неточно, если задана область отсечения.
 * @bug Знак двойной высоты строится в памяти, выделенной @c alloca() на
 *      стеке обработчика прерывания.
 */
template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_draw_glyph_full(
    const glyph &glyph, glyph_options glyph_options, rgb888 pen_color, rgb888 brush_color,
    esp::gfx::rect &update_rect, TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    const int glyph_x = glyph.x + orig_x;
    const int glyph_y = glyph.y + orig_y;

    if (glyph_x > clip_x2 || glyph_y > clip_y2)
        return;

    const std::int16_t glyph_width = glyph.width;
    const std::int16_t glyph_height = glyph.height;
    const std::uint8_t *glyph_data = glyph.data;
    const std::int16_t glyph_width_byte = (glyph_width + 7) / 8;
    const std::int16_t glyph_size = glyph_height * glyph_width_byte;

    const bool fill_background = glyph_options.fill_background;
    const bool bold = glyph_options.bold;
    const bool italic = glyph_options.italic;
    const bool blank = glyph_options.blank;
    const bool underline = glyph_options.underline;
    const int double_width = glyph_options.double_width;

    // Для двойной высоты строится знак из удвоенных строк верхней или нижней
    // половины; простая двойная ширина рисуется прямо при выводе.
    if (double_width > 1)
    {
        std::uint8_t *const new_glyph_data = static_cast<std::uint8_t *>(alloca(glyph_size));
        // Удваивается верхняя половина (2) или нижняя (3).
        const int offset = (double_width == 2 ? 0 : (glyph_height >> 1));
        for (int y = 0; y < glyph_height; ++y)
        {
            for (int x = 0; x < glyph_width_byte; ++x)
            {
                new_glyph_data[x + y * glyph_width_byte] =
                    glyph_data[x + (offset + (y >> 1)) * glyph_width_byte];
            }
        }
        glyph_data = new_glyph_data;
    }

    // Курсив: верхняя треть сдвигается на 2 точки, средняя — на 1.
    int skew_adder = 0;
    int skew_h1 = 0;
    int skew_h2 = 0;
    if (italic)
    {
        skew_adder = 2;
        skew_h1 = glyph_height / 3;
        skew_h2 = skew_h1 * 2;
    }

    std::int16_t x1 = 0;
    std::int16_t x_count = glyph_width;
    std::int16_t dest_x = glyph_x;

    if (dest_x < clip_x1)
    {
        x1 = (clip_x1 - dest_x) / (double_width ? 2 : 1);
        dest_x = clip_x1;
    }
    if (x1 >= glyph_width)
        return;

    if (dest_x + x_count + skew_adder > clip_x2 + 1)
        x_count = clip_x2 + 1 - dest_x - skew_adder;
    if (x1 + x_count > glyph_width)
        x_count = glyph_width - x1;

    std::int16_t y1 = 0;
    std::int16_t y_count = glyph_height;
    int dest_y = glyph_y;

    if (dest_y < clip_y1)
    {
        y1 = clip_y1 - dest_y;
        dest_y = clip_y1;
    }
    if (y1 >= glyph_height)
        return;

    if (dest_y + y_count > clip_y2 + 1)
        y_count = clip_y2 + 1 - dest_y;
    if (y1 + y_count > glyph_height)
        y_count = glyph_height - y1;

    update_rect = update_rect.merge(
        rect(dest_x, dest_y, dest_x + x_count + skew_adder - 1, dest_y + y_count - 1));
    hide_sprites(update_rect);

    if (glyph_options.invert ^ paint_state().paint_options.swap_fgbg)
        esp::tswap(pen_color, brush_color);

    // Пониженная яркость: каналы пера ограничиваются значением 128.
    if (glyph_options.reduce_luminosity)
    {
        if (pen_color.r > 128)
            pen_color.r = 128;
        if (pen_color.g > 128)
            pen_color.g = 128;
        if (pen_color.b > 128)
            pen_color.b = 128;
    }

    auto pen_pattern = prepare_pixel(pen_color);
    auto brush_pattern = prepare_pixel(brush_color);
    auto bold_pattern = bold
        ? prepare_pixel(rgb888(pen_color.r / 2 + 1, pen_color.g / 2 + 1, pen_color.b / 2 + 1))
        : prepare_pixel(rgb888(0, 0, 0));

    for (int y = y1; y < y1 + y_count; ++y, ++dest_y)
    {

        // Нарисована ли предыдущая точка: за ней полужирное начертание рисует ещё одну.
        bool prev_set = false;

        auto dstrow = raw_get_row(dest_y);
        auto srcrow = glyph_data + y * glyph_width_byte;

        if (underline && y == glyph_height - UNDERLINE_POSITION - 1)
        {

            for (int x = x1, adest_x = dest_x + skew_adder; x < x1 + x_count && adest_x <= clip_x2;
                 ++x, ++adest_x)
            {
                raw_set_pixel_in_row(dstrow, adest_x, blank ? brush_pattern : pen_pattern);
                if (double_width)
                {
                    ++adest_x;
                    if (adest_x > clip_x2)
                        break;
                    raw_set_pixel_in_row(dstrow, adest_x, blank ? brush_pattern : pen_pattern);
                }
            }
        }
        else
        {

            for (int x = x1, adest_x = dest_x + skew_adder; x < x1 + x_count && adest_x <= clip_x2;
                 ++x, ++adest_x)
            {
                if ((srcrow[x >> 3] << (x & 7)) & 0x80 && !blank)
                {
                    raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
                    prev_set = true;
                }
                else if (bold && prev_set)
                {
                    raw_set_pixel_in_row(dstrow, adest_x, bold_pattern);
                    prev_set = false;
                }
                else if (fill_background)
                {
                    raw_set_pixel_in_row(dstrow, adest_x, brush_pattern);
                    prev_set = false;
                }
                else
                {
                    prev_set = false;
                }
                if (double_width)
                {
                    ++adest_x;
                    if (adest_x > clip_x2)
                        break;
                    if (fill_background)
                        raw_set_pixel_in_row(dstrow, adest_x,
                                             prev_set ? pen_pattern : brush_pattern);
                    else if (prev_set)
                        raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
                }
            }
        }

        if (italic && (y == skew_h1 || y == skew_h2))
            --skew_adder;
    }
}

/**
 * @brief Быстро рисует знак без начертаний.
 *
 * Строка знака берётся одним 32-разрядным словом и сдвигается по точке.
 * Годится, только если знак шириной до 32 точек, а из параметров знаков
 * заданы лишь заливка знакоместа, инверсия и пониженная яркость (и обмен
 * цветов в общих параметрах рисования).
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на знак.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_get_row Получение строки.
 * @param raw_set_pixel_in_row Запись пикселя в строку.
 */
template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_draw_glyph_light(
    const glyph &glyph, glyph_options glyph_options, rgb888 pen_color, rgb888 brush_color,
    esp::gfx::rect &update_rect, TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    const int glyph_x = glyph.x + orig_x;
    const int glyph_y = glyph.y + orig_y;

    if (glyph_x > clip_x2 || glyph_y > clip_y2)
        return;

    const std::int16_t glyph_width = glyph.width;
    const std::int16_t glyph_height = glyph.height;
    const std::uint8_t *const glyph_data = glyph.data;
    const std::int16_t glyph_width_byte = (glyph_width + 7) / 8;

    std::int16_t x1 = 0;
    std::int16_t x_count = glyph_width;
    std::int16_t dest_x = glyph_x;

    std::int16_t y1 = 0;
    std::int16_t y_count = glyph_height;
    int dest_y = glyph_y;

    if (dest_x < clip_x1)
    {
        x1 = clip_x1 - dest_x;
        dest_x = clip_x1;
    }
    if (x1 >= glyph_width)
        return;

    if (dest_x + x_count > clip_x2 + 1)
        x_count = clip_x2 + 1 - dest_x;
    if (x1 + x_count > glyph_width)
        x_count = glyph_width - x1;

    if (dest_y < clip_y1)
    {
        y1 = clip_y1 - dest_y;
        dest_y = clip_y1;
    }
    if (y1 >= glyph_height)
        return;

    if (dest_y + y_count > clip_y2 + 1)
        y_count = clip_y2 + 1 - dest_y;
    if (y1 + y_count > glyph_height)
        y_count = glyph_height - y1;

    update_rect =
        update_rect.merge(rect(dest_x, dest_y, dest_x + x_count - 1, dest_y + y_count - 1));
    hide_sprites(update_rect);

    if (glyph_options.invert ^ paint_state().paint_options.swap_fgbg)
        esp::tswap(pen_color, brush_color);

    // Пониженная яркость: каналы пера ограничиваются значением 128.
    if (glyph_options.reduce_luminosity)
    {
        if (pen_color.r > 128)
            pen_color.r = 128;
        if (pen_color.g > 128)
            pen_color.g = 128;
        if (pen_color.b > 128)
            pen_color.b = 128;
    }

    const bool fill_background = glyph_options.fill_background;

    const auto pen_pattern = prepare_pixel(pen_color);
    const auto brush_pattern = prepare_pixel(brush_color);

    for (int y = y1; y < y1 + y_count; ++y, ++dest_y)
    {
        const auto dstrow = raw_get_row(dest_y);
        const std::uint8_t *const srcrow = glyph_data + y * glyph_width_byte;

        // Строка знака одним словом: старший разряд — первая точка.
        std::uint32_t src = (srcrow[0] << 24) | (srcrow[1] << 16) | (srcrow[2] << 8) | (srcrow[3]);
        src <<= x1;
        if (fill_background)
        {
            // Фон знакоместа заливается.
            for (int x = x1, adest_x = dest_x; x < x1 + x_count; ++x, ++adest_x, src <<= 1)
            {
                raw_set_pixel_in_row(dstrow, adest_x,
                                     src & 0x80000000 ? pen_pattern : brush_pattern);
            }
        }
        else
        {
            // Фон прозрачен.
            for (int x = x1, adest_x = dest_x; x < x1 + x_count; ++x, ++adest_x, src <<= 1)
            {
                if (src & 0x80000000)
                    raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
            }
        }
    }
}

/**
 * @brief Инвертирует пиксели прямоугольника, обрезанного областью отсечения.
 *
 * @tparam TRawInvertRow Функция «инвертировать отрезок строки (y, x1, x2)».
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется.
 * @param raw_invert_row Инверсия отрезка строки.
 */
template<typename TRawInvertRow>
void esp::gfx::generic_bitmapped_display_controller::generic_invert_rect(
    const rect &rect, esp::gfx::rect &update_rect, TRawInvertRow raw_invert_row)
{
    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int x1 = esp::iclamp(rect.x1 + orig_x, clip_x1, clip_x2);
    const int y1 = esp::iclamp(rect.y1 + orig_y, clip_y1, clip_y2);
    const int x2 = esp::iclamp(rect.x2 + orig_x, clip_x1, clip_x2);
    const int y2 = esp::iclamp(rect.y2 + orig_y, clip_y1, clip_y2);

    update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
    hide_sprites(update_rect);

    for (int y = y1; y <= y2; ++y)
        raw_invert_row(y, x1, x2);
}

/**
 * @brief Меняет в прямоугольнике цвета пера и кисти.
 *
 * Пиксели цвета пера становятся цвета кисти и наоборот; другие цвета не
 * меняются.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
         typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_swap_fgbg(
    const rect &rect, esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
    TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    auto pen_pattern = prepare_pixel(paint_state().pen_color);
    auto brush_pattern = prepare_pixel(paint_state().brush_color);

    int orig_x = paint_state().origin.x;
    int orig_y = paint_state().origin.y;

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int x1 = esp::iclamp(rect.x1 + orig_x, clip_x1, clip_x2);
    const int y1 = esp::iclamp(rect.y1 + orig_y, clip_y1, clip_y2);
    const int x2 = esp::iclamp(rect.x2 + orig_x, clip_x1, clip_x2);
    const int y2 = esp::iclamp(rect.y2 + orig_y, clip_y1, clip_y2);

    update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
    hide_sprites(update_rect);

    for (int y = y1; y <= y2; ++y)
    {
        auto row = raw_get_row(y);
        for (int x = x1; x <= x2; ++x)
        {
            auto px = raw_get_pixel_in_row(row, x);
            if (px == pen_pattern)
                raw_set_pixel_in_row(row, x, brush_pattern);
            else if (px == brush_pattern)
                raw_set_pixel_in_row(row, x, pen_pattern);
        }
    }
}

/**
 * @brief Копирует прямоугольник в текущую позицию.
 *
 * Порядок обхода строк и столбцов выбирается по направлению сдвига, поэтому
 * области могут перекрываться. Пиксели вне области отсечения не пишутся.
 *
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param source Исходный прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на обе области.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_copy_rect(
    const rect &source, esp::gfx::rect &update_rect, TRawGetRow raw_get_row,
    TRawGetPixelInRow raw_get_pixel_in_row, TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    int orig_x = paint_state().origin.x;
    int orig_y = paint_state().origin.y;

    int src_x = source.x1 + orig_x;
    int src_y = source.y1 + orig_y;
    int width = source.x2 - source.x1 + 1;
    int height = source.y2 - source.y1 + 1;
    int dest_x = paint_state().position.x;
    int dest_y = paint_state().position.y;
    int delta_x = dest_x - src_x;
    int delta_y = dest_y - src_y;

    int inc_x = delta_x < 0 ? 1 : -1;
    int inc_y = delta_y < 0 ? 1 : -1;

    int start_x = delta_x < 0 ? dest_x : dest_x + width - 1;
    int start_y = delta_y < 0 ? dest_y : dest_y + height - 1;

    update_rect = update_rect.merge(rect(src_x, src_y, src_x + width - 1, src_y + height - 1));
    update_rect = update_rect.merge(rect(dest_x, dest_y, dest_x + width - 1, dest_y + height - 1));
    hide_sprites(update_rect);

    for (int y = start_y, i = 0; i < height; y += inc_y, ++i)
    {
        if (y >= clip_y1 && y <= clip_y2)
        {
            auto src_row = raw_get_row(y - delta_y);
            auto dst_row = raw_get_row(y);
            for (int x = start_x, j = 0; j < width; x += inc_x, ++j)
            {
                if (x >= clip_x1 && x <= clip_x2)
                    raw_set_pixel_in_row(dst_row, x, raw_get_pixel_in_row(src_row, x - delta_x));
            }
        }
    }
}

/**
 * @brief Переносит часть растра собственного формата на экран без отсечения.
 *
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @tparam TDataType Тип пикселя растра.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param data Пиксели растра.
 * @param width Ширина растра, пиксели.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 * @param raw_get_row Получение строки.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TRawGetRow, typename TRawSetPixelInRow, typename TDataType>
void esp::gfx::generic_bitmapped_display_controller::generic_raw_draw_bitmap_native(
    int dest_x, int dest_y, TDataType *data, int width, int x1, int y1, int x_count, int y_count,
    TRawGetRow raw_get_row, TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int y_end = y1 + y_count;
    const int x_end = x1 + x_count;
    for (int y = y1; y < y_end; ++y, ++dest_y)
    {
        auto dstrow = raw_get_row(dest_y);
        auto src = data + y * width + x1;
        for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++src)
            raw_set_pixel_in_row(dstrow, adest_x, *src);
    }
}

/**
 * @brief Переносит часть маски на экран без отсечения, сохраняя фон.
 *
 * Непрозрачные пиксели маски пишутся заранее подготовленным цветом —
 * функция записи сама знает его.
 *
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x)».
 * @tparam TBackground Тип пикселя сохранённого фона.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата маски.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
         typename TBackground>
void esp::gfx::generic_bitmapped_display_controller::generic_raw_draw_bitmap_mask(
    int dest_x, int dest_y, const bitmap *bitmap, TBackground *save_background, int x1, int y1,
    int x_count, int y_count, TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int width = bitmap->width;
    const int y_end = y1 + y_count;
    const int x_end = x1 + x_count;
    auto data = bitmap->data;
    const int rowlen = (bitmap->width + 7) / 8;

    if (save_background)
    {

        // Фон сохраняется, растр рисуется.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto save_px = save_background + y * width + x1;
            auto src = data + y * rowlen;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++save_px)
            {
                *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                if ((src[x >> 3] << (x & 7)) & 0x80)
                    raw_set_pixel_in_row(dstrow, adest_x);
            }
        }
    }
    else
    {

        // Только рисование растра.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto src = data + y * rowlen;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x)
            {
                if ((src[x >> 3] << (x & 7)) & 0x80)
                    raw_set_pixel_in_row(dstrow, adest_x);
            }
        }
    }
}

/**
 * @brief Переносит часть растра RGBA2222 на экран без отсечения, сохраняя фон.
 *
 * Пиксели с нулевым альфа-каналом пропускаются.
 *
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель RGBA2222 в строку (строка, x, пиксель)».
 * @tparam TBackground Тип пикселя сохранённого фона.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата RGBA2222.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
         typename TBackground>
void esp::gfx::generic_bitmapped_display_controller::generic_raw_draw_bitmap_rgba2222(
    int dest_x, int dest_y, const bitmap *bitmap, TBackground *save_background, int x1, int y1,
    int x_count, int y_count, TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int width = bitmap->width;
    const int y_end = y1 + y_count;
    const int x_end = x1 + x_count;
    auto data = bitmap->data;

    if (save_background)
    {

        // Фон сохраняется, растр рисуется.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto save_px = save_background + y * width + x1;
            auto src = data + y * width + x1;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++save_px, ++src)
            {
                *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                if (*src & 0xc0) // Альфа-канал не нулевой.
                    raw_set_pixel_in_row(dstrow, adest_x, *src);
            }
        }
    }
    else
    {

        // Только рисование растра.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto src = data + y * width + x1;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++src)
            {
                if (*src & 0xc0) // Альфа-канал не нулевой.
                    raw_set_pixel_in_row(dstrow, adest_x, *src);
            }
        }
    }
}

/**
 * @brief Переносит часть растра RGBA8888 на экран без отсечения, сохраняя фон.
 *
 * Пиксели с нулевым альфа-каналом пропускаются.
 *
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель RGBA8888 в строку (строка, x, пиксель)».
 * @tparam TBackground Тип пикселя сохранённого фона.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата RGBA8888.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
         typename TBackground>
void esp::gfx::generic_bitmapped_display_controller::generic_raw_draw_bitmap_rgba8888(
    int dest_x, int dest_y, const bitmap *bitmap, TBackground *save_background, int x1, int y1,
    int x_count, int y_count, TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
    TRawSetPixelInRow raw_set_pixel_in_row)
{
    const int width = bitmap->width;
    const int y_end = y1 + y_count;
    const int x_end = x1 + x_count;
    const rgba8888 *const data = reinterpret_cast<const rgba8888 *>(bitmap->data);

    if (save_background)
    {

        // Фон сохраняется, растр рисуется.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto save_px = save_background + y * width + x1;
            auto src = data + y * width + x1;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++save_px, ++src)
            {
                *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                if (src->a)
                    raw_set_pixel_in_row(dstrow, adest_x, *src);
            }
        }
    }
    else
    {

        // Только рисование растра.
        for (int y = y1; y < y_end; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto src = data + y * width + x1;
            for (int x = x1, adest_x = dest_x; x < x_end; ++x, ++adest_x, ++src)
            {
                if (src->a)
                    raw_set_pixel_in_row(dstrow, adest_x, *src);
            }
        }
    }
}

/**
 * @brief Прокручивает область прокрутки по вертикали копированием строк.
 *
 * Строки копируются в направлении, обратном прокрутке, а освободившиеся
 * заливаются цветом кисти.
 *
 * @tparam TRawCopyRow Функция «скопировать отрезок строки (x1, x2, откуда, куда)».
 * @tparam TRawFillRow Функция «залить отрезок строки (y, x1, x2, цвет)».
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 * @param raw_copy_row Копирование отрезка строки.
 * @param raw_fill_row Заливка отрезка строки.
 */
template<typename TRawCopyRow, typename TRawFillRow>
void esp::gfx::generic_bitmapped_display_controller::generic_v_scroll(int scroll,
                                                                      esp::gfx::rect &update_rect,
                                                                      TRawCopyRow raw_copy_row,
                                                                      TRawFillRow raw_fill_row)
{
    hide_sprites(update_rect);
    rgb888 color = actual_brush_color();
    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;
    int height = y2 - y1 + 1;

    if (scroll < 0)
    {

        // Прокрутка вверх.

        for (int i = 0; i < height + scroll; ++i)
        {
            // Отрезок x1…x2 строки y1 + i − scroll копируется в строку y1 + i.
            raw_copy_row(x1, x2, (y1 + i - scroll), (y1 + i));
        }
        // Освободившиеся нижние строки заливаются цветом кисти.
        for (int i = height + scroll; i < height; ++i)
            raw_fill_row(y1 + i, x1, x2, color);
    }
    else if (scroll > 0)
    {

        // Прокрутка вниз.
        for (int i = height - scroll - 1; i >= 0; --i)
        {
            // Отрезок x1…x2 строки y1 + i копируется в строку y1 + i + scroll.
            raw_copy_row(x1, x2, (y1 + i), (y1 + i + scroll));
        }

        // Освободившиеся верхние строки заливаются цветом кисти.
        for (int i = 0; i < scroll; ++i)
            raw_fill_row(y1 + i, x1, x2, color);
    }
}

/**
 * @brief Прокручивает область прокрутки по вертикали перестановкой строк.
 *
 * Строки буфера кадра меняются местами указателями, без копирования
 * пикселей; части строк левее и правее области прокрутки возвращаются на
 * место копированием. Освободившиеся строки заливаются цветом кисти.
 *
 * @tparam TSwapRowsCopying Функция «обменять отрезки двух строк копированием (a, b, x1, x2)».
 * @tparam TSwapRowsPointers Функция «обменять указатели двух строк (a, b)».
 * @tparam TRawFillRow Функция «залить отрезок строки (y, x1, x2, цвет)».
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 * @param swap_rows_copying Обмен отрезков строк.
 * @param swap_rows_pointers Обмен указателей строк.
 * @param raw_fill_row Заливка отрезка строки.
 */
template<typename TSwapRowsCopying, typename TSwapRowsPointers, typename TRawFillRow>
void esp::gfx::generic_bitmapped_display_controller::generic_v_scroll(
    int scroll, esp::gfx::rect &update_rect, TSwapRowsCopying swap_rows_copying,
    TSwapRowsPointers swap_rows_pointers, TRawFillRow raw_fill_row)
{
    hide_sprites(update_rect);
    rgb888 color = actual_brush_color();
    const int y1 = paint_state().scrolling_region.y1;
    const int y2 = paint_state().scrolling_region.y2;
    const int x1 = paint_state().scrolling_region.x1;
    const int x2 = paint_state().scrolling_region.x2;
    const int height = y2 - y1 + 1;

    const int view_port_width = this->view_port_width();

    if (scroll < 0)
    {

        // Прокрутка вверх.

        for (int i = 0; i < height + scroll; ++i)
        {

            // Части строк вне области прокрутки остаются на месте.
            if (x1 > 0)
                swap_rows_copying(y1 + i, y1 + i - scroll, 0, x1 - 1);
            if (x2 < view_port_width - 1)
                swap_rows_copying(y1 + i, y1 + i - scroll, x2 + 1, view_port_width - 1);

            // Строки меняются местами указателями.
            swap_rows_pointers(y1 + i, y1 + i - scroll);
        }

        // Освободившиеся нижние строки заливаются цветом кисти.
        for (int i = height + scroll; i < height; ++i)
            raw_fill_row(y1 + i, x1, x2, color);
    }
    else if (scroll > 0)
    {

        // Прокрутка вниз.
        for (int i = height - scroll - 1; i >= 0; --i)
        {

            // Части строк вне области прокрутки остаются на месте.
            if (x1 > 0)
                swap_rows_copying(y1 + i, y1 + i + scroll, 0, x1 - 1);
            if (x2 < view_port_width - 1)
                swap_rows_copying(y1 + i, y1 + i + scroll, x2 + 1, view_port_width - 1);

            // Строки меняются местами указателями.
            swap_rows_pointers(y1 + i, y1 + i + scroll);
        }

        // Освободившиеся верхние строки заливаются цветом кисти.
        for (int i = 0; i < scroll; ++i)
            raw_fill_row(y1 + i, x1, x2, color);
    }
}

/**
 * @brief Прокручивает область прокрутки по горизонтали копированием пикселей.
 *
 * Медленно: каждый пиксель читается и пишется отдельно. Освободившиеся
 * столбцы заливаются цветом кисти.
 *
 * @tparam TPreparePixel Функция «цвет → значение пикселя».
 * @tparam TRawGetRow Функция «строка буфера кадра по номеру».
 * @tparam TRawGetPixelInRow Функция «прочитать пиксель строки (строка, x)».
 * @tparam TRawSetPixelInRow Функция «записать пиксель в строку (строка, x, значение)».
 * @param scroll Шаг: отрицательный — влево, положительный — вправо.
 * @param update_rect Изменённая область.
 * @param prepare_pixel Подготовка значения пикселя.
 * @param raw_get_row Получение строки.
 * @param raw_get_pixel_in_row Чтение пикселя.
 * @param raw_set_pixel_in_row Запись пикселя.
 */
template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
         typename TRawSetPixelInRow>
void esp::gfx::generic_bitmapped_display_controller::generic_h_scroll(
    int scroll, esp::gfx::rect &update_rect, TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
    TRawGetPixelInRow raw_get_pixel_in_row, TRawSetPixelInRow raw_set_pixel_in_row)
{
    hide_sprites(update_rect);
    auto pattern = prepare_pixel(actual_brush_color());

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    if (scroll < 0)
    {
        // Прокрутка влево.
        for (int y = y1; y <= y2; ++y)
        {
            auto row = raw_get_row(y);
            for (int x = x1; x <= x2 + scroll; ++x)
            {
                auto c = raw_get_pixel_in_row(row, x - scroll);
                raw_set_pixel_in_row(row, x, c);
            }
            // Освободившиеся правые столбцы заливаются цветом кисти.
            for (int x = x2 + 1 + scroll; x <= x2; ++x)
                raw_set_pixel_in_row(row, x, pattern);
        }
    }
    else if (scroll > 0)
    {
        // Прокрутка вправо.
        for (int y = y1; y <= y2; ++y)
        {
            auto row = raw_get_row(y);
            for (int x = x2 - scroll; x >= x1; --x)
            {
                auto c = raw_get_pixel_in_row(row, x);
                raw_set_pixel_in_row(row, x + scroll, c);
            }
            // Освободившиеся левые столбцы заливаются цветом кисти.
            for (int x = x1; x < x1 + scroll; ++x)
                raw_set_pixel_in_row(row, x, pattern);
        }
    }
}
