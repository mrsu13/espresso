// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga2_controller.h
 * @brief Объявление контроллера VGA на 2 цветов esp::vga::vga2_controller.
 */

/**
 * @file esp_vga_vga2_controller.cpp
 * @brief Контроллер VGA на 2 цветов: рисование и обработчик прерывания.
 */

#include "esp_vga_vga2_controller.h"

#include "esp_vga_utils_p.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include <esp_attr.h>
#include <esp_heap_caps.h>
#if __has_include(<esp_private/cache_utils.h>)
#   include <esp_private/cache_utils.h>
#else
#   include <esp_spi_flash.h>
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/i2s_struct.h>


#pragma GCC optimize("O2")


namespace esp::vga::p
{

/** @brief Число строк DMA контроллера на 2 цветов. */
constexpr int VGA2_LINES_COUNT = 4;

/** @brief Ширина области вывода контроллера на 2 цветов кратна этому числу пикселей. */
constexpr int VGA2_COLUMNS_QUANTUM = 16;

FORCE_INLINE_ATTR void vga2_set_pixel_in_row(std::uint8_t *row, int x, int value);
FORCE_INLINE_ATTR int vga2_get_pixel_in_row(std::uint8_t *row, int x);
FORCE_INLINE_ATTR void vga2_set_pixel(int x, int y, int value);
template<typename T>
FORCE_INLINE_ATTR void vga2_invert_pixel_in_row(T *row, int x);
FORCE_INLINE_ATTR void vga2_invert_pixel(int x, int y);

}


/**
 * @brief Записывает номер цвета пикселя в строку буфера кадра.
 * @param row Строка.
 * @param x Пиксель.
 * @param value Номер цвета палитры.
 */
void esp::vga::p::vga2_set_pixel_in_row(std::uint8_t *row, int x, int value)
{
    int brow = x >> 3;
    row[brow] ^= (-value ^ row[brow]) & (0x80 >> (x & 7));
}

/**
 * @brief Читает номер цвета пикселя из строки буфера кадра.
 * @param row Строка.
 * @param x Пиксель.
 * @return Номер цвета палитры.
 */
int esp::vga::p::vga2_get_pixel_in_row(std::uint8_t *row, int x)
{
    int brow = x >> 3;
    return (row[brow] & (0x80 >> (x & 7))) != 0;
}

/**
 * @brief Записывает номер цвета пикселя (@p x, @p y) буфера рисования.
 *
 * Строка берётся из статической копии указателей строк: шаблонам
 * рисования нужна функция без объекта.
 *
 * @param x Столбец.
 * @param y Строка.
 * @param value Номер цвета палитры.
 */
void esp::vga::p::vga2_set_pixel(int x, int y, int value)
{
    std::uint8_t *const row = esp::vga::vga2_controller::sget_scanline(y);
    int brow = x >> 3;
    row[brow] ^= (-value ^ row[brow]) & (0x80 >> (x & 7));
}

/**
 * @brief Инвертирует номер цвета пикселя в строке буфера кадра.
 * @tparam T Тип байта строки: @c std::uint8_t или @c volatile @c std::uint8_t.
 * @param row Строка.
 * @param x Пиксель.
 */
template<typename T>
void esp::vga::p::vga2_invert_pixel_in_row(T *row, int x)
{
    row[x >> 3] ^= (0x80 >> (x & 7));
}

/**
 * @brief Инвертирует номер цвета пикселя (@p x, @p y) буфера рисования.
 * @param x Столбец.
 * @param y Строка.
 */
void esp::vga::p::vga2_invert_pixel(int x, int y)
{
    esp::vga::p::vga2_invert_pixel_in_row(esp::vga::vga2_controller::sget_scanline(y), x);
}


/**
 * @class esp::vga::vga2_controller
 * @brief Контроллер VGA на 2 цветов из 64 с палитрой.
 *
 * В буфере кадра на пиксель приходится 1 разряд — номер цвета палитры:
 * в байте 8 пикселей, старший разряд — левый пиксель. Поэтому буфер в 8 раз меньше, чем у esp::vga::vga_controller:
 * при 640 × 480 — около 40 КБ. Вывод занимает до 10 % одного
 * ядра (640 × 480, 60 Гц).
 * @code
 * esp::vga::vga2_controller display;
 * display.begin(); // Выводы TTGO VGA32.
 * display.set_resolution(esp::vga::VGA_640X480_60HZ);
 * @endcode
 */


// Открытые

/**
 * @brief Создаёт контроллер; выводы и развёртку задают begin() и set_resolution().
 *
 * Выделяет таблицу сигналов палитры.
 */
esp::vga::vga2_controller::vga2_controller()
    : vga_paletted_controller(esp::vga::p::VGA2_LINES_COUNT, esp::vga::p::VGA2_COLUMNS_QUANTUM,
                              esp::gfx::native_pixel_format::palette2, 8, 1, isr_handler)
{
    _packed_palette_index_octet_to_signals = static_cast<std::uint64_t *>(
        heap_caps_malloc(256 * sizeof(std::uint64_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
}

/**
 * @brief Освобождает таблицу сигналов палитры.
 */
esp::vga::vga2_controller::~vga2_controller()
{
    heap_caps_free(const_cast<std::uint64_t *>(_packed_palette_index_octet_to_signals));
}

/**
 * @brief Читает пиксели области вывода в цветах палитры.
 *
 * Границы не проверяются.
 *
 * @param rect Прямоугольник.
 * @param dest_buf Буфер на ширину × высоту прямоугольника цветов, построчно.
 */
void esp::vga::vga2_controller::read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[esp::vga::p::vga2_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 × 3 = 255.
        }
    }
}

/**
 * @brief Задаёт цвет палитры.
 * @param index Номер цвета: от 0 до 1; берётся по модулю 2.
 * @param color Цвет; на выводы идут по 2 старших разряда каналов.
 */
void esp::vga::vga2_controller::set_palette_item(int index, const esp::gfx::rgb888 &color)
{
    index %= 2;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 256; ++i)
    {
        auto b = reinterpret_cast<std::uint8_t *>(
            const_cast<std::uint64_t *>(_packed_palette_index_octet_to_signals + i));
        for (int j = 0; j < 8; ++j)
        {
            auto aj = 7 - j;
            if ((index == 0 && ((1 << aj) & i) == 0) || (index == 1 && ((1 << aj) & i) != 0))
            {
                b[j ^ 2] = _hv_sync | packed222;
            }
        }
    }
}

// Защищённые

/**
 * @brief Задаёт палитру по умолчанию: чёрный и белый.
 */
void esp::vga::vga2_controller::setup_default_palette()
{
    set_palette_item(0, esp::gfx::rgb888(0, 0, 0)); // 0: чёрный.
    set_palette_item(1, esp::gfx::rgb888(255, 255, 255)); // 1: белый.
}

// Закрытые

/**
 * @brief Рисует точку заданного цвета.
 * @param pixel_desc Положение относительно начала координат и цвет.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::set_pixel_at(const esp::gfx::pixel_desc &pixel_desc,
                                             esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, esp::vga::p::vga2_set_pixel);
}

/**
 * @brief Рисует эллипс пером с центром в текущей позиции.
 * @param size Размеры эллипса.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::draw_ellipse(const esp::gfx::size &size,
                                             esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, esp::vga::p::vga2_set_pixel);
}

/**
 * @brief Заливает область вывода цветом кисти.
 * @param update_rect Изменённая область.
 */
void esp::vga::vga2_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    std::uint8_t pattern8 = palette_index ? 0xFF : 0x00;
    for (int y = 0; y < _view_port_height; ++y)
        std::memset(const_cast<std::uint8_t *>(_view_port[y]), pattern8, _view_port_width / 8);
}

/**
 * @brief Прокручивает область прокрутки по вертикали.
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 */
void esp::vga::vga2_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swap_rows_copying
        [&](int y_a, int y_b)
        { std::swap(_view_port[y_a], _view_port[y_b]); }, // swap_rows_pointers
        [&](int y, int x1, int x2, esp::gfx::rgb888 color)
        { raw_fill_row(y, x1, x2, color); } // raw_fill_row
    );
}

/**
 * @brief Прокручивает область прокрутки по горизонтали.
 *
 * Если область выровнена, пиксели сдвигаются целыми байтами и словами,
 * иначе — по одному.
 *
 * @param scroll Шаг: отрицательный — влево, положительный — вправо.
 * @param update_rect Изменённая область.
 */
void esp::vga::vga2_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t back = rgb888_to_palette_index(actual_brush_color());
    std::uint8_t back8 = back ? 0xFF : 0x00;

    int y1 = paint_state().scrolling_region.y1;
    int y2 = paint_state().scrolling_region.y2;
    int x1 = paint_state().scrolling_region.x1;
    int x2 = paint_state().scrolling_region.x2;

    int width = x2 - x1 + 1;
    bool h_scrollling_region_aligned =
        ((x1 & 7) == 0 && (width & 7) == 0); // Выровнено по 8 пикселей.

    if (scroll < 0)
    {
        // Влево.
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // Область прокрутки выровнена: быстрый вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]) + x1 / 8;
                for (int s = -scroll; s > 0;)
                {
                    if (s < 8)
                    {
                        // Влево на 1…7.
                        int sz = width / 8;
                        std::uint8_t prev = back8;
                        for (int i = sz - 1; i >= 0; --i)
                        {
                            std::uint8_t lowbits = prev >> (8 - s);
                            prev = row[i];
                            row[i] = (row[i] << s) | lowbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // Влево на кратное 8.
                        auto sc = s & ~7;
                        auto sz = width & ~7;
                        std::memmove(row, row + sc / 8, (sz - sc) / 8);
                        raw_fill_row(y, x2 - sc + 1, x2, back);
                        s -= sc;
                    }
                }
            }
            else
            {
                // Область прокрутки не выровнена: медленный вариант.
                auto row = const_cast<std::uint8_t *>(_view_port[y]);
                for (int x = x1; x <= x2 + scroll; ++x)
                    esp::vga::p::vga2_set_pixel_in_row(
                        row, x, esp::vga::p::vga2_get_pixel_in_row(row, x - scroll));
                // Освободившееся место справа — цветом кисти.
                raw_fill_row(y, x2 + 1 + scroll, x2, back);
            }
        }
    }
    else if (scroll > 0)
    {
        // Вправо.
        for (int y = y1; y <= y2; ++y)
        {
            if (h_scrollling_region_aligned)
            {
                // Область прокрутки выровнена: быстрый вариант.
                std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]) + x1 / 8;
                for (int s = scroll; s > 0;)
                {
                    if (s < 8)
                    {
                        // Вправо на 1…7.
                        int sz = width / 8;
                        std::uint8_t prev = back8;
                        for (int i = 0; i < sz; ++i)
                        {
                            std::uint8_t highbits = prev << (8 - s);
                            prev = row[i];
                            row[i] = (row[i] >> s) | highbits;
                        }
                        s = 0;
                    }
                    else
                    {
                        // Вправо на кратное 8.
                        auto sc = s & ~7;
                        auto sz = width & ~7;
                        std::memmove(row + sc / 8, row, (sz - sc) / 8);
                        raw_fill_row(y, x1, x1 + sc - 1, back);
                        s -= sc;
                    }
                }
            }
            else
            {
                // Область прокрутки не выровнена: медленный вариант.
                auto row = const_cast<std::uint8_t *>(_view_port[y]);
                for (int x = x2 - scroll; x >= x1; --x)
                    esp::vga::p::vga2_set_pixel_in_row(row, x + scroll,
                                                       esp::vga::p::vga2_get_pixel_in_row(row, x));
                // Освободившееся место слева — цветом кисти.
                raw_fill_row(y, x1, x1 + scroll - 1, back);
            }
        }
    }
}

/**
 * @brief Рисует знак.
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::draw_glyph(const esp::gfx::glyph &glyph,
                                           esp::gfx::glyph_options glyph_options,
                                           esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                                           esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); }, [&](int y)
        { return const_cast<std::uint8_t *>(_view_port[y]); }, esp::vga::p::vga2_set_pixel_in_row);
}

/**
 * @brief Инвертирует номера цветов в прямоугольнике.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::invert_rect(const esp::gfx::rect &rect, esp::gfx::rect &update_rect)
{
    generic_invert_rect(rect, update_rect,
                        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
}

/**
 * @brief Копирует прямоугольник в текущую позицию; области могут перекрываться.
 *
 * Медленно: пиксели копируются по одному.
 *
 * @param source Исходный прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::copy_rect(const esp::gfx::rect &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        esp::vga::p::vga2_get_pixel_in_row, esp::vga::p::vga2_set_pixel_in_row);
}

/**
 * @brief Меняет в прямоугольнике цвета пера и кисти; другие цвета не трогает.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga2_controller::swap_fgbg(const esp::gfx::rect &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        esp::vga::p::vga2_get_pixel_in_row, esp::vga::p::vga2_set_pixel_in_row);
}

/**
 * @brief Переносит часть растра собственного формата на экран без отсечения.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void esp::vga::vga2_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                       const esp::gfx::bitmap *bitmap, int x1,
                                                       int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga2_set_pixel_in_row);
}

/**
 * @brief Рисует маску растра цветом его переднего плана без отсечения.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void esp::vga::vga2_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                     const esp::gfx::bitmap *bitmap,
                                                     void *save_background, int x1, int y1,
                                                     int x_count, int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga2_get_pixel_in_row,
        [&](std::uint8_t *row, int x)
        {
            esp::vga::p::vga2_set_pixel_in_row(row, x, foreground_color_index);
        } // raw_set_pixel_in_row
    );
}

/**
 * @brief Переносит часть растра RGBA2222 на экран без отсечения; прозрачные пиксели пропускаются.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void esp::vga::vga2_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                         const esp::gfx::bitmap *bitmap,
                                                         void *save_background, int x1, int y1,
                                                         int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga2_get_pixel_in_row,
        [&](std::uint8_t *row, int x, std::uint8_t src)
        {
            esp::vga::p::vga2_set_pixel_in_row(row, x, rgb2222_to_palette_index(src));
        } // raw_set_pixel_in_row
    );
}

/**
 * @brief Переносит часть растра RGBA8888 на экран без отсечения; прозрачные пиксели пропускаются.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Куда сохранить фон под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */
void esp::vga::vga2_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                         const esp::gfx::bitmap *bitmap,
                                                         void *save_background, int x1, int y1,
                                                         int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x)
        { return esp::vga::p::vga2_get_pixel_in_row(row, x); }, // raw_get_pixel_in_row
        [&](std::uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        {
            esp::vga::p::vga2_set_pixel_in_row(row, x, rgb8888_to_palette_index(src));
        } // raw_set_pixel_in_row
    );
}

/**
 * @brief Заливает отрезок строки цветом палитры, ближайшим к заданному; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param color Цвет.
 */
void esp::vga::vga2_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
{
    raw_fill_row(y, x1, x2, rgb888_to_palette_index(color));
}

/**
 * @brief Заливает отрезок строки цветом палитры; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param color_index Номер цвета палитры.
 */
void esp::vga::vga2_controller::raw_fill_row(int y, int x1, int x2, std::uint8_t color_index)
{
    std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
    // Первые пиксели до целого байта.
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        esp::vga::p::vga2_set_pixel_in_row(row, x, color_index);
    }
    // Целые байты (по 8 пикселей).
    if (x <= x2)
    {
        int sz = (x2 & ~7) - x;
        std::memset(row + x / 8, color_index ? 0xFF : 0x00, sz / 8);
        x += sz;
    }
    // Последние невыровненные пиксели.
    for (; x <= x2; ++x)
    {
        esp::vga::p::vga2_set_pixel_in_row(row, x, color_index);
    }
}

/**
 * @brief Инвертирует номера цветов в отрезке строки; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void esp::vga::vga2_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        esp::vga::p::vga2_invert_pixel_in_row(row, x);
}

/**
 * @brief Копирует отрезок строки в другую строку.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param src_y Исходная строка.
 * @param dst_y Строка назначения.
 */
void esp::vga::vga2_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = const_cast<std::uint8_t *>(_view_port[src_y]);
    auto dst_row = const_cast<std::uint8_t *>(_view_port[dst_y]);
    // Первые пиксели до целого байта.
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        esp::vga::p::vga2_set_pixel_in_row(dst_row, x,
                                           esp::vga::p::vga2_get_pixel_in_row(src_row, x));
    }
    // Целые байты (по 8 пикселей).
    auto src = reinterpret_cast<std::uint8_t *>(src_row + x / 8);
    auto dst = reinterpret_cast<std::uint8_t *>(dst_row + x / 8);
    for (int right = (x2 & ~7); x < right; x += 8)
        *dst++ = *src++;
    // Последние невыровненные пиксели.
    for (x = (x2 & ~7); x <= x2; ++x)
    {
        esp::vga::p::vga2_set_pixel_in_row(dst_row, x,
                                           esp::vga::p::vga2_get_pixel_in_row(src_row, x));
    }
}

/**
 * @brief Меняет местами отрезки двух строк.
 * @param y_a Первая строка.
 * @param y_b Вторая строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void esp::vga::vga2_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = const_cast<std::uint8_t *>(_view_port[y_a]);
    auto row_b = const_cast<std::uint8_t *>(_view_port[y_b]);
    // Первые пиксели до целого байта.
    int x = x1;
    for (; x <= x2 && (x & 7) != 0; ++x)
    {
        std::uint8_t a = esp::vga::p::vga2_get_pixel_in_row(row_a, x);
        std::uint8_t b = esp::vga::p::vga2_get_pixel_in_row(row_b, x);
        esp::vga::p::vga2_set_pixel_in_row(row_a, x, b);
        esp::vga::p::vga2_set_pixel_in_row(row_b, x, a);
    }
    // Целые байты (по 8 пикселей).
    auto a = reinterpret_cast<std::uint8_t *>(row_a + x / 8);
    auto b = reinterpret_cast<std::uint8_t *>(row_b + x / 8);
    for (int right = (x2 & ~7); x < right; x += 8)
        std::swap(*a++, *b++);
    // Последние невыровненные пиксели.
    for (x = (x2 & ~7); x <= x2; ++x)
    {
        std::uint8_t a = esp::vga::p::vga2_get_pixel_in_row(row_a, x);
        std::uint8_t b = esp::vga::p::vga2_get_pixel_in_row(row_b, x);
        esp::vga::p::vga2_set_pixel_in_row(row_a, x, b);
        esp::vga::p::vga2_set_pixel_in_row(row_b, x, a);
    }
}

/**
 * @brief Рисует линию в абсолютных координатах с отсечением по текущей области отсечения.
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 * @param color Цвет.
 */
void esp::vga::vga2_controller::abs_draw_line(int x1, int y1, int x2, int y2,
                                              esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, std::uint8_t color_index)
        { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, esp::vga::p::vga2_set_pixel,
        [&](int x, int y) { esp::vga::p::vga2_invert_pixel(x, y); });
}

/**
 * @brief Обработчик прерывания I2S1: переводит номера цветов следующих строк в сигналы VGA.
 *
 * Вызывается каждые полчисла строк DMA и заполняет освободившиеся строки
 * DMA по таблице сигналов палитры. После последней строки области вывода
 * будит задачу примитивов. Работает в IRAM.
 *
 * @param arg Контроллер.
 */
void IRAM_ATTR esp::vga::vga2_controller::isr_handler(void *arg)
{
    auto *const self = static_cast<esp::vga::vga2_controller *>(arg);

    if (I2S1.int_st.out_eof)
    {
        const auto desc = reinterpret_cast<lldesc_t *>(I2S1.out_eof_des_addr);

        if (desc == _frame_reset_desc)
            _scan_line = 0;

        auto const width = self->_view_port_width;
        auto const height = self->_view_port_height;
        auto const packed_palette_index_octet_to_signals =
            const_cast<const std::uint64_t *>(self->_packed_palette_index_octet_to_signals);
        auto const lines = self->_lines;

        int scan_line = (_scan_line + esp::vga::p::VGA2_LINES_COUNT / 2) % height;

        auto line_index = scan_line & (esp::vga::p::VGA2_LINES_COUNT - 1);

        for (int i = 0; i < esp::vga::p::VGA2_LINES_COUNT / 2; ++i)
        {
            auto src = const_cast<const std::uint8_t *>(_isr_view_port_visible[scan_line]);
            auto dest =
                reinterpret_cast<std::uint64_t *>(const_cast<std::uint8_t *>(lines[line_index]));

            // Ширина области вывода кратна 16.
            for (int col = 0; col < width; col += 16)
            {
                auto src1 = *(src + 0);
                auto src2 = *(src + 1);

                ESP_VGA_PSRAM_HACK();

                auto v1 = packed_palette_index_octet_to_signals[src1];
                auto v2 = packed_palette_index_octet_to_signals[src2];

                *(dest + 0) = v1;
                *(dest + 1) = v2;

                dest += 2;
                src += 2;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += esp::vga::p::VGA2_LINES_COUNT / 2;

        if (scan_line >= height && !self->_primitive_processing_suspended
            && spi_flash_cache_enabled() && self->_primitive_exec_task)
        {
            // Кадр выведен: будим задачу примитивов.
            // Рисуя примитивы, нельзя вызывать vTaskSuspendAll(): иначе vTaskNotifyGiveFromISR() задержится и изображение замерцает.
            vTaskNotifyGiveFromISR(self->_primitive_exec_task, nullptr);
        }
    }


    I2S1.int_clr.val = I2S1.int_st.val;
}
