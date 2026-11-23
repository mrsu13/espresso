// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_vga8_controller.h
 * @brief Объявление контроллера VGA на 8 цветов esp::vga::vga8_controller.
 */

/**
 * @file esp_vga_vga8_controller.cpp
 * @brief Контроллер VGA на 8 цветов: рисование и обработчик прерывания.
 */

#include "esp_vga_vga8_controller.h"

#include "esp_tools.h"
#include "esp_vga_utils_p.h"

#include <cstdint>
#include <cstring>

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

/** @brief Число строк DMA контроллера на 8 цветов. */
constexpr int VGA8_LINES_COUNT = 4;

/** @brief Ширина области вывода контроллера на 8 цветов кратна этому числу пикселей. */
constexpr int VGA8_COLUMNS_QUANTUM = 16;

FORCE_INLINE_ATTR void vga8_set_pixel_in_row(std::uint8_t *row, int x, int value);
FORCE_INLINE_ATTR int vga8_get_pixel_in_row(std::uint8_t *row, int x);
FORCE_INLINE_ATTR void vga8_set_pixel(int x, int y, int value);
template<typename T>
FORCE_INLINE_ATTR void vga8_invert_pixel_in_row(T *row, int x);
FORCE_INLINE_ATTR void vga8_invert_pixel(int x, int y);

}


/**
 * @brief Записывает номер цвета пикселя в строку буфера кадра.
 * @param row Строка.
 * @param x Пиксель.
 * @param value Номер цвета палитры.
 */
void esp::vga::p::vga8_set_pixel_in_row(std::uint8_t *row, int x, int value)
{
    std::uint32_t *bits24 = reinterpret_cast<std::uint32_t *>(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    *bits24 ^= ((value << shift) ^ *bits24) & (7 << shift);
}

/**
 * @brief Читает номер цвета пикселя из строки буфера кадра.
 * @param row Строка.
 * @param x Пиксель.
 * @return Номер цвета палитры.
 */
int esp::vga::p::vga8_get_pixel_in_row(std::uint8_t *row, int x)
{
    std::uint32_t *bits24 = reinterpret_cast<std::uint32_t *>(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    return (*bits24 >> shift) & 7;
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
void esp::vga::p::vga8_set_pixel(int x, int y, int value)
{
    std::uint8_t *const row = esp::vga::vga8_controller::sget_scanline(y);
    std::uint32_t *bits24 = reinterpret_cast<std::uint32_t *>(row + (x >> 3) * 3); // x / 8 * 3
    int shift = 21 - (x & 7) * 3;
    *bits24 ^= ((value << shift) ^ *bits24) & (7 << shift);
}

/**
 * @brief Инвертирует номер цвета пикселя в строке буфера кадра.
 * @tparam T Тип байта строки: @c std::uint8_t или @c volatile @c std::uint8_t.
 * @param row Строка.
 * @param x Пиксель.
 */
template<typename T>
void esp::vga::p::vga8_invert_pixel_in_row(T *row, int x)
{
    *reinterpret_cast<std::uint32_t *>(const_cast<std::uint8_t *>(row + (x >> 3) * 3)) ^= 7
        << (21 - (x & 7) * 3);
}

/**
 * @brief Инвертирует номер цвета пикселя (@p x, @p y) буфера рисования.
 * @param x Столбец.
 * @param y Строка.
 */
void esp::vga::p::vga8_invert_pixel(int x, int y)
{
    esp::vga::p::vga8_invert_pixel_in_row(esp::vga::vga8_controller::sget_scanline(y), x);
}


/**
 * @class esp::vga::vga8_controller
 * @brief Контроллер VGA на 8 цветов из 64 с палитрой.
 *
 * В буфере кадра на пиксель приходится 3 разряда — номер цвета палитры:
 * 8 пикселей — в 3 байтах. Поэтому буфер в 8/3 раза меньше, чем у esp::vga::vga_controller:
 * при 640 × 480 — около 113 КБ. Вывод занимает до 23 % одного
 * ядра (640 × 480, 60 Гц).
 * @code
 * esp::vga::vga8_controller display;
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
esp::vga::vga8_controller::vga8_controller()
    : vga_paletted_controller(esp::vga::p::VGA8_LINES_COUNT, esp::vga::p::VGA8_COLUMNS_QUANTUM,
                              esp::gfx::native_pixel_format::palette8, 8, 3, isr_handler)
{
    _packed_palette_index_pair_to_signals = static_cast<std::uint16_t *>(
        heap_caps_malloc(256 * sizeof(std::uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
}

/**
 * @brief Освобождает таблицу сигналов палитры.
 */
esp::vga::vga8_controller::~vga8_controller()
{
    heap_caps_free(const_cast<std::uint16_t *>(_packed_palette_index_pair_to_signals));
}

/**
 * @brief Читает пиксели области вывода в цветах палитры.
 *
 * Границы не проверяются.
 *
 * @param rect Прямоугольник.
 * @param dest_buf Буфер на ширину × высоту прямоугольника цветов, построчно.
 */
void esp::vga::vga8_controller::read_screen(const esp::gfx::rect &rect, esp::gfx::rgb888 *dest_buf)
{
    for (int y = rect.y1; y <= rect.y2; ++y)
    {
        auto row = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = rect.x1; x <= rect.x2; ++x, ++dest_buf)
        {
            const esp::gfx::rgb222 v = _palette[esp::vga::p::vga8_get_pixel_in_row(row, x)];
            *dest_buf = esp::gfx::rgb888(v.r * 85, v.g * 85, v.b * 85); // 85 × 3 = 255.
        }
    }
}

/**
 * @brief Задаёт цвет палитры.
 * @param index Номер цвета: от 0 до 7; берётся по модулю 8.
 * @param color Цвет; на выводы идут по 2 старших разряда каналов.
 */
void esp::vga::vga8_controller::set_palette_item(int index, const esp::gfx::rgb888 &color)
{
    index %= 8;
    _palette[index] = color;
    auto packed222 = esp::gfx::rgb888_to_packed_rgb222(color);
    for (int i = 0; i < 8; ++i)
    {
        _packed_palette_index_pair_to_signals[(index << 3) | i] &= 0xFF00;
        _packed_palette_index_pair_to_signals[(index << 3) | i] |= (_hv_sync | packed222);
        _packed_palette_index_pair_to_signals[(i << 3) | index] &= 0x00FF;
        _packed_palette_index_pair_to_signals[(i << 3) | index] |= (_hv_sync | packed222) << 8;
    }
}

/**
 * @brief Записывает номер цвета пикселя (@p x, @p y) буфера рисования без очереди примитивов.
 * @param x Столбец.
 * @param y Строка.
 * @param value Номер цвета палитры.
 */
void esp::vga::vga8_controller::direct_set_pixel(int x, int y, int value)
{
    esp::vga::p::vga8_set_pixel(x, y, value);
}

// Защищённые

/**
 * @brief Задаёт палитру по умолчанию: чёрный, красный, зелёный, синий, их яркие варианты и белый.
 */
void esp::vga::vga8_controller::setup_default_palette()
{
    set_palette_item(0, esp::gfx::rgb888(0, 0, 0)); // 0: чёрный.
    set_palette_item(1, esp::gfx::rgb888(128, 0, 0)); // 1: красный.
    set_palette_item(2, esp::gfx::rgb888(0, 128, 0)); // 2: зелёный.
    set_palette_item(3, esp::gfx::rgb888(0, 0, 128)); // 3: синий.
    set_palette_item(4, esp::gfx::rgb888(255, 0, 0)); // 4: ярко-красный.
    set_palette_item(5, esp::gfx::rgb888(0, 255, 0)); // 5: ярко-зелёный.
    set_palette_item(6, esp::gfx::rgb888(0, 0, 255)); // 6: ярко-синий.
    set_palette_item(7, esp::gfx::rgb888(255, 255, 255)); // 7: белый.
}

// Закрытые

/**
 * @brief Рисует точку заданного цвета.
 * @param pixel_desc Положение относительно начала координат и цвет.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga8_controller::set_pixel_at(const esp::gfx::pixel_desc &pixel_desc,
                                             esp::gfx::rect &update_rect)
{
    generic_set_pixel_at(
        pixel_desc, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, esp::vga::p::vga8_set_pixel);
}

/**
 * @brief Рисует эллипс пером с центром в текущей позиции.
 * @param size Размеры эллипса.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga8_controller::draw_ellipse(const esp::gfx::size &size,
                                             esp::gfx::rect &update_rect)
{
    generic_draw_ellipse(
        size, update_rect, [&](esp::gfx::rgb888 const &color)
        { return rgb888_to_palette_index(color); }, esp::vga::p::vga8_set_pixel);
}

/**
 * @brief Заливает область вывода цветом кисти.
 * @param update_rect Изменённая область.
 */
void esp::vga::vga8_controller::clear(esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t palette_index = rgb888_to_palette_index(actual_brush_color());
    std::uint32_t pattern8 = (palette_index) | (palette_index << 3) | (palette_index << 6)
        | (palette_index << 9) | (palette_index << 12) | (palette_index << 15)
        | (palette_index << 18) | (palette_index << 21);
    for (int y = 0; y < _view_port_height; ++y)
    {
        auto dest = const_cast<std::uint8_t *>(_view_port[y]);
        for (int x = 0; x < _view_port_width; x += 8, dest += 3)
            *reinterpret_cast<std::uint32_t *>(dest) =
                (*reinterpret_cast<std::uint32_t *>(dest) & 0xFF000000) | pattern8;
    }
}

/**
 * @brief Прокручивает область прокрутки по вертикали.
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 */
void esp::vga::vga8_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
{
    generic_v_scroll(
        scroll, update_rect,
        [&](int y_a, int y_b, int x1, int x2) { swap_rows(y_a, y_b, x1, x2); }, // swap_rows_copying
        [&](int y_a, int y_b)
        { esp::tswap(_view_port[y_a], _view_port[y_b]); }, // swap_rows_pointers
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
void esp::vga::vga8_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
{
    hide_sprites(update_rect);
    std::uint8_t back = rgb888_to_palette_index(actual_brush_color());

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
            for (int s = -scroll; s > 0;)
            {
                if (h_scrollling_region_aligned && s >= 8)
                {
                    // Влево на кратное 8.
                    std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]) + x1 / 8 * 3;
                    auto sc = s & ~7;
                    auto sz = width & ~7;
                    std::memmove(row, row + sc / 8 * 3, (sz - sc) / 8 * 3);
                    raw_fill_row(y, x2 - sc + 1, x2, back);
                    s -= sc;
                }
                else
                {
                    // Область прокрутки не выровнена: медленный вариант.
                    auto row = const_cast<std::uint8_t *>(_view_port[y]);
                    for (int x = x1; x <= x2 - s; ++x)
                        esp::vga::p::vga8_set_pixel_in_row(
                            row, x, esp::vga::p::vga8_get_pixel_in_row(row, x + s));
                    // Освободившееся место справа — цветом кисти.
                    raw_fill_row(y, x2 - s + 1, x2, back);
                    s = 0;
                }
            }
        }
    }
    else if (scroll > 0)
    {
        // Вправо.
        for (int y = y1; y <= y2; ++y)
        {
            for (int s = scroll; s > 0;)
            {
                if (h_scrollling_region_aligned && s >= 8)
                {
                    // Вправо на кратное 8.
                    std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]) + x1 / 8 * 3;
                    auto sc = s & ~7;
                    auto sz = width & ~7;
                    std::memmove(row + sc / 8 * 3, row, (sz - sc) / 8 * 3);
                    raw_fill_row(y, x1, x1 + sc - 1, back);
                    s -= sc;
                }
                else
                {
                    // Область прокрутки не выровнена: медленный вариант.
                    auto row = const_cast<std::uint8_t *>(_view_port[y]);
                    for (int x = x2 - s; x >= x1; --x)
                        esp::vga::p::vga8_set_pixel_in_row(
                            row, x + s, esp::vga::p::vga8_get_pixel_in_row(row, x));
                    // Освободившееся место слева — цветом кисти.
                    raw_fill_row(y, x1, x1 + s - 1, back);
                    s = 0;
                }
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
void esp::vga::vga8_controller::draw_glyph(const esp::gfx::glyph &glyph,
                                           esp::gfx::glyph_options glyph_options,
                                           esp::gfx::rgb888 pen_color, esp::gfx::rgb888 brush_color,
                                           esp::gfx::rect &update_rect)
{
    generic_draw_glyph(
        glyph, glyph_options, pen_color, brush_color, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); }, [&](int y)
        { return const_cast<std::uint8_t *>(_view_port[y]); }, esp::vga::p::vga8_set_pixel_in_row);
}

/**
 * @brief Инвертирует номера цветов в прямоугольнике.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga8_controller::invert_rect(const esp::gfx::rect &rect, esp::gfx::rect &update_rect)
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
void esp::vga::vga8_controller::copy_rect(const esp::gfx::rect &source, esp::gfx::rect &update_rect)
{
    generic_copy_rect(
        source, update_rect, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        esp::vga::p::vga8_get_pixel_in_row, esp::vga::p::vga8_set_pixel_in_row);
}

/**
 * @brief Меняет в прямоугольнике цвета пера и кисти; другие цвета не трогает.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */
void esp::vga::vga8_controller::swap_fgbg(const esp::gfx::rect &rect, esp::gfx::rect &update_rect)
{
    generic_swap_fgbg(
        rect, update_rect,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); },
        esp::vga::p::vga8_get_pixel_in_row, esp::vga::p::vga8_set_pixel_in_row);
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
void esp::vga::vga8_controller::raw_draw_bitmap_native(int dest_x, int dest_y,
                                                       const esp::gfx::bitmap *bitmap, int x1,
                                                       int y1, int x_count, int y_count)
{
    generic_raw_draw_bitmap_native(
        dest_x, dest_y, bitmap->data, bitmap->width, x1, y1, x_count, y_count,
        [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga8_set_pixel_in_row);
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
void esp::vga::vga8_controller::raw_draw_bitmap_mask(int dest_x, int dest_y,
                                                     const esp::gfx::bitmap *bitmap,
                                                     void *save_background, int x1, int y1,
                                                     int x_count, int y_count)
{
    auto foreground_color_index = rgb888_to_palette_index(bitmap->foreground_color);
    generic_raw_draw_bitmap_mask(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga8_get_pixel_in_row,
        [&](std::uint8_t *row, int x)
        {
            esp::vga::p::vga8_set_pixel_in_row(row, x, foreground_color_index);
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
void esp::vga::vga8_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y,
                                                         const esp::gfx::bitmap *bitmap,
                                                         void *save_background, int x1, int y1,
                                                         int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba2222(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        esp::vga::p::vga8_get_pixel_in_row,
        [&](std::uint8_t *row, int x, std::uint8_t src)
        {
            esp::vga::p::vga8_set_pixel_in_row(row, x, rgb2222_to_palette_index(src));
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
void esp::vga::vga8_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y,
                                                         const esp::gfx::bitmap *bitmap,
                                                         void *save_background, int x1, int y1,
                                                         int x_count, int y_count)
{
    generic_raw_draw_bitmap_rgba8888(
        dest_x, dest_y, bitmap, static_cast<std::uint8_t *>(save_background), x1, y1, x_count,
        y_count, [&](int y) { return const_cast<std::uint8_t *>(_view_port[y]); }, // raw_get_row
        [&](std::uint8_t *row, int x)
        { return esp::vga::p::vga8_get_pixel_in_row(row, x); }, // raw_get_pixel_in_row
        [&](std::uint8_t *row, int x, esp::gfx::rgba8888 const &src)
        {
            esp::vga::p::vga8_set_pixel_in_row(row, x, rgb8888_to_palette_index(src));
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
void esp::vga::vga8_controller::raw_fill_row(int y, int x1, int x2, esp::gfx::rgb888 color)
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
void esp::vga::vga8_controller::raw_fill_row(int y, int x1, int x2, std::uint8_t color_index)
{
    std::uint8_t *row = const_cast<std::uint8_t *>(_view_port[y]);
    for (; x1 <= x2; ++x1)
        esp::vga::p::vga8_set_pixel_in_row(row, x1, color_index);
}

/**
 * @brief Инвертирует номера цветов в отрезке строки; параметры не проверяются.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void esp::vga::vga8_controller::raw_invert_row(int y, int x1, int x2)
{
    auto row = _view_port[y];
    for (int x = x1; x <= x2; ++x)
        esp::vga::p::vga8_invert_pixel_in_row(row, x);
}

/**
 * @brief Копирует отрезок строки в другую строку.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param src_y Исходная строка.
 * @param dst_y Строка назначения.
 */
void esp::vga::vga8_controller::raw_copy_row(int x1, int x2, int src_y, int dst_y)
{
    auto src_row = const_cast<std::uint8_t *>(_view_port[src_y]);
    auto dst_row = const_cast<std::uint8_t *>(_view_port[dst_y]);
    for (; x1 <= x2; ++x1)
        esp::vga::p::vga8_set_pixel_in_row(dst_row, x1,
                                           esp::vga::p::vga8_get_pixel_in_row(src_row, x1));
}

/**
 * @brief Меняет местами отрезки двух строк.
 * @param y_a Первая строка.
 * @param y_b Вторая строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 */
void esp::vga::vga8_controller::swap_rows(int y_a, int y_b, int x1, int x2)
{
    auto row_a = const_cast<std::uint8_t *>(_view_port[y_a]);
    auto row_b = const_cast<std::uint8_t *>(_view_port[y_b]);
    for (; x1 <= x2; ++x1)
    {
        std::uint8_t a = esp::vga::p::vga8_get_pixel_in_row(row_a, x1);
        std::uint8_t b = esp::vga::p::vga8_get_pixel_in_row(row_b, x1);
        esp::vga::p::vga8_set_pixel_in_row(row_a, x1, b);
        esp::vga::p::vga8_set_pixel_in_row(row_b, x1, a);
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
void esp::vga::vga8_controller::abs_draw_line(int x1, int y1, int x2, int y2,
                                              esp::gfx::rgb888 color)
{
    generic_abs_draw_line(
        x1, y1, x2, y2, color,
        [&](esp::gfx::rgb888 const &color) { return rgb888_to_palette_index(color); },
        [&](int y, int x1, int x2, std::uint8_t color_index)
        { raw_fill_row(y, x1, x2, color_index); },
        [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); }, esp::vga::p::vga8_set_pixel,
        [&](int x, int y) { esp::vga::p::vga8_invert_pixel(x, y); });
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
void IRAM_ATTR esp::vga::vga8_controller::isr_handler(void *arg)
{
    auto *const self = static_cast<esp::vga::vga8_controller *>(arg);

    if (I2S1.int_st.out_eof)
    {
        const auto desc = reinterpret_cast<lldesc_t *>(I2S1.out_eof_des_addr);

        if (desc == _frame_reset_desc)
            _scan_line = 0;

        auto const width = self->_view_port_width;
        auto const height = self->_view_port_height;
        auto const packed_palette_index_pair_to_signals =
            const_cast<const std::uint16_t *>(self->_packed_palette_index_pair_to_signals);
        auto const lines = self->_lines;

        int scan_line = (_scan_line + esp::vga::p::VGA8_LINES_COUNT / 2) % height;

        auto line_index = scan_line & (esp::vga::p::VGA8_LINES_COUNT - 1);

        for (int i = 0; i < esp::vga::p::VGA8_LINES_COUNT / 2; ++i)
        {
            auto src = const_cast<const std::uint8_t *>(_isr_view_port_visible[scan_line]);
            auto dest =
                reinterpret_cast<std::uint16_t *>(const_cast<std::uint8_t *>(lines[line_index]));

            // Ширина области вывода кратна 16.
            for (int col = 0; col < width; col += 16)
            {
                auto w1 =
                    *(reinterpret_cast<const std::uint16_t *>(src)); // hi A:23334445, lo A:55666777
                auto w2 = *(reinterpret_cast<const std::uint16_t *>(
                    src + 2)); // hi B:55666777, lo A:00011122
                auto w3 = *(reinterpret_cast<const std::uint16_t *>(
                    src + 4)); // hi B:00011122, lo B:23334445

                ESP_VGA_PSRAM_HACK();

                auto src1 = w1 | (w2 << 16);
                auto src2 = (w2 >> 8) | (w3 << 8);

                auto v1 = packed_palette_index_pair_to_signals[(src1) & 0x3f]; // Пиксели 0 и 1.
                auto v2 =
                    packed_palette_index_pair_to_signals[(src1 >> 6) & 0x3f]; // Пиксели 2 и 3.
                auto v3 =
                    packed_palette_index_pair_to_signals[(src1 >> 12) & 0x3f]; // Пиксели 4 и 5.
                auto v4 =
                    packed_palette_index_pair_to_signals[(src1 >> 18) & 0x3f]; // Пиксели 6 и 7.
                auto v5 = packed_palette_index_pair_to_signals[(src2) & 0x3f]; // Пиксели 8 и 9.
                auto v6 =
                    packed_palette_index_pair_to_signals[(src2 >> 6) & 0x3f]; // Пиксели 10 и 11.
                auto v7 =
                    packed_palette_index_pair_to_signals[(src2 >> 12) & 0x3f]; // Пиксели 12 и 13.
                auto v8 =
                    packed_palette_index_pair_to_signals[(src2 >> 18) & 0x3f]; // Пиксели 14 и 15.

                *(dest + 2) = v1;
                *(dest + 3) = v2;
                *(dest + 0) = v3;
                *(dest + 1) = v4;
                *(dest + 6) = v5;
                *(dest + 7) = v6;
                *(dest + 4) = v7;
                *(dest + 5) = v8;

                dest += 8;
                src += 6;
            }

            ++line_index;
            ++scan_line;
        }

        _scan_line += esp::vga::p::VGA8_LINES_COUNT / 2;

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
