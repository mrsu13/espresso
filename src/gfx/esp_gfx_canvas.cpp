// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_canvas.h
 * @brief Объявление холста esp::gfx::canvas.
 */

/**
 * @file esp_gfx_canvas.cpp
 * @brief Холст: рисование через очередь примитивов контроллера дисплея.
 */

#include "esp_gfx_canvas.h"

#include "esp_gfx_glyph.h"
#include "esp_gfx_primitive.h"
#include "esp_gfx_size.h"

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#pragma GCC optimize("O2")


namespace esp::gfx::p
{

/**
 * @brief «Недействительный» прямоугольник — признак незаданной области отсечения.
 *
 * Все границы −32768; настоящая область отсечения такой не бывает.
 */
constexpr esp::gfx::rect INVALID_RECT{-32768, -32768, -32768, -32768};

}


/**
 * @class esp::gfx::canvas
 * @brief Холст: рисование линий, фигур, знаков и растров на дисплее.
 *
 * Холст не рисует сам: каждый метод ставит примитив рисования в очередь
 * контроллера дисплея, а тот выполняет его, обычно на обратном ходе луча.
 * Поэтому методы возвращаются сразу, а рисование может отставать;
 * дождаться его можно wait_completion().
 *
 * Координаты отсчитываются от левого верхнего угла области вывода, (0, 0),
 * до (width() − 1, height() − 1), и сдвигаются на начало координат
 * (set_origin()). Фигуры обрезаются областью отсечения. Линии рисуются
 * цветом пера, заливки — цветом кисти:
 * @code
 * esp::gfx::canvas canvas(&display);
 * canvas.set_pen_color(esp::gfx::color::bright_red);
 * canvas.set_brush_color(esp::gfx::color::bright_green);
 * canvas.fill_rectangle(0, 0, canvas.width() - 1, canvas.height() - 1);
 * canvas.draw_rectangle(0, 0, canvas.width() - 1, canvas.height() - 1);
 * @endcode
 */


// Открытые

/**
 * @brief Создаёт холст для контроллера дисплея.
 * @param display_controller Контроллер; должен существовать, пока существует холст.
 */
esp::gfx::canvas::canvas(bitmapped_display_controller *display_controller)
    : _display_controller{display_controller}
    , _origin{0, 0}
    , _clipping_rect{esp::gfx::p::INVALID_RECT}
{
}

/**
 * @brief Ждёт, пока нарисуются примитивы очереди.
 *
 * @param wait_v_sync @c true — ждать, пока их выполнит обработчик на
 *        обратном ходе луча (медленно, без мерцания); @c false — выполнить
 *        их сейчас (быстро, но изображение может мерцать).
 */
void esp::gfx::canvas::wait_completion(bool wait_v_sync)
{
    if (wait_v_sync)
        _display_controller->primitives_execution_wait();
    else
        _display_controller->process_primitives();
}

/**
 * @brief Приостанавливает рисование.
 *
 * Пока рисование приостановлено, очередь не разбирается и может
 * переполниться, тогда следующий вызов рисования зависнет. Поэтому нужно
 * часто вызывать wait_completion(false).
 */
void esp::gfx::canvas::begin_update()
{
    _display_controller->suspend_background_primitive_execution();
}

/**
 * @brief Возобновляет рисование после begin_update().
 */
void esp::gfx::canvas::end_update()
{
    _display_controller->resume_background_primitive_execution();
}

/**
 * @brief Меняет местами буферы кадра при двойной буферизации и ждёт обмена.
 *
 * Двойная буферизация включается при выборе режима
 * (esp::gfx::base_display_controller::set_resolution()). Обмен всегда
 * происходит на кадровом синхроимпульсе.
 */
void esp::gfx::canvas::swap_buffers()
{
    primitive p;
    p.cmd = primitive_cmd::swap_buffers;
    p.notify_task = xTaskGetCurrentTaskHandle();
    _display_controller->add_primitive(p);
    _display_controller->primitives_execution_wait();
}

/**
 * @brief Задаёт начало координат.
 *
 * Все координаты, кроме положения спрайтов, сдвигаются на начало координат.
 *
 * @param x Сдвиг по горизонтали; 0 — без сдвига.
 * @param y Сдвиг по вертикали; 0 — без сдвига.
 */
void esp::gfx::canvas::set_origin(int x, int y)
{
    set_origin(point(x, y));
}

/**
 * @brief Задаёт начало координат.
 * @param origin Начало координат.
 */
void esp::gfx::canvas::set_origin(const point &origin)
{
    primitive p;
    p.cmd = primitive_cmd::set_origin;
    p.position = _origin = origin;
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт область отсечения относительно начала координат.
 *
 * По умолчанию область отсечения — весь холст.
 *
 * @param rect Область отсечения.
 */
void esp::gfx::canvas::set_clipping_rect(const rect &rect)
{
    primitive p;
    p.cmd = primitive_cmd::set_clipping_rect;
    p.rect = _clipping_rect = rect;
    _display_controller->add_primitive(p);
}

/**
 * @brief Возвращает область отсечения, заданную set_clipping_rect().
 * @return Область отсечения; если она не задана — весь холст.
 */
esp::gfx::rect esp::gfx::canvas::clipping_rect()
{
    if (_clipping_rect == esp::gfx::p::INVALID_RECT)
        _clipping_rect = rect(0, 0, width() - 1, height() - 1);
    return _clipping_rect;
}

/**
 * @brief Заливает весь холст цветом кисти.
 */
void esp::gfx::canvas::clear()
{
    primitive p;
    p.cmd = primitive_cmd::clear;
    p.ivalue = 0;
    _display_controller->add_primitive(p);
}

/**
 * @brief Возвращает параметры рисования контроллера к начальным.
 *
 * Сбрасываются цвета, позиция, параметры знаков и рисования, начало
 * координат и области прокрутки и отсечения.
 */
void esp::gfx::canvas::reset()
{
    primitive p;
    p.cmd = primitive_cmd::reset;
    _display_controller->add_primitive(p);
    _origin = point(0, 0);
    _clipping_rect = esp::gfx::p::INVALID_RECT;
}

/**
 * @brief Задаёт область прокрутки — прямоугольник, в котором работает scroll().
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
void esp::gfx::canvas::set_scrolling_region(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::set_scrolling_region;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}

/**
 * @brief Прокручивает область прокрутки.
 *
 * Вертикальная прокрутка переставляет строки буфера кадра и потому быстрая;
 * горизонтальная копирует пиксели по одному и нагружает процессор.
 * Освободившаяся часть заливается цветом кисти.
 *
 * @param offset_x Сдвиг: отрицательный — влево, положительный — вправо.
 * @param offset_y Сдвиг: отрицательный — вверх, положительный — вниз.
 */
void esp::gfx::canvas::scroll(int offset_x, int offset_y)
{
    primitive p;
    if (offset_y)
    {
        p.cmd = primitive_cmd::v_scroll;
        p.ivalue = offset_y;
        _display_controller->add_primitive(p);
    }
    if (offset_x)
    {
        p.cmd = primitive_cmd::h_scroll;
        p.ivalue = offset_x;
        _display_controller->add_primitive(p);
    }
}

/**
 * @brief Переносит текущую позицию пера.
 *
 * С текущей позиции рисует line_to():
 * @code
 * // Треугольник.
 * canvas.move_to(10, 12);
 * canvas.line_to(30, 40);
 * canvas.line_to(40, 12);
 * canvas.line_to(10, 12);
 * @endcode
 *
 * @param x Абсцисса.
 * @param y Ордината.
 */
void esp::gfx::canvas::move_to(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::move_to;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт цвет пера по каналам.
 * @param red Красный канал, от 0 до 255.
 * @param green Зелёный канал, от 0 до 255.
 * @param blue Синий канал, от 0 до 255.
 */
void esp::gfx::canvas::set_pen_color(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    set_pen_color(rgb888(red, green, blue));
}

/**
 * @brief Задаёт цвет пера по имени.
 * @param color Именованный цвет.
 */
void esp::gfx::canvas::set_pen_color(color color)
{
    set_pen_color(rgb888(color));
}

/**
 * @brief Задаёт цвет пера: им рисуются линии, рамки, точки и знаки.
 * @param color Цвет.
 */
void esp::gfx::canvas::set_pen_color(const rgb888 &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_pen_color;
    p.color = color;
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт цвет кисти по каналам.
 * @param red Красный канал, от 0 до 255.
 * @param green Зелёный канал, от 0 до 255.
 * @param blue Синий канал, от 0 до 255.
 */
void esp::gfx::canvas::set_brush_color(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    set_brush_color(rgb888(red, green, blue));
}

/**
 * @brief Задаёт цвет кисти по имени.
 * @param color Именованный цвет.
 */
void esp::gfx::canvas::set_brush_color(color color)
{
    set_brush_color(rgb888(color));
}

/**
 * @brief Задаёт цвет кисти: им заливаются фигуры, фон знаков и холст при очистке.
 * @param color Цвет.
 */
void esp::gfx::canvas::set_brush_color(const rgb888 &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_brush_color;
    p.color = color;
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт толщину пера для линий, рамок и многоугольников.
 * @param value Толщина, пиксели; меньше 1 не бывает.
 */
void esp::gfx::canvas::set_pen_width(int value)
{
    primitive p;
    p.cmd = primitive_cmd::set_pen_width;
    p.ivalue = value;
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт форму концов линий толщиной больше 1.
 * @param value Форма концов.
 */
void esp::gfx::canvas::set_line_ends(line_ends value)
{
    primitive p;
    p.cmd = primitive_cmd::set_line_ends;
    p.line_ends = value;
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует точку цветом пера.
 * @param x Абсцисса.
 * @param y Ордината.
 */
void esp::gfx::canvas::set_pixel(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::set_pixel;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует точку заданного цвета.
 * @param x Абсцисса.
 * @param y Ордината.
 * @param color Цвет.
 */
void esp::gfx::canvas::set_pixel(int x, int y, const rgb888 &color)
{
    set_pixel(point(x, y), color);
}

/**
 * @brief Рисует точку заданного цвета.
 * @param pos Положение.
 * @param color Цвет.
 */
void esp::gfx::canvas::set_pixel(const point &pos, const rgb888 &color)
{
    primitive p;
    p.cmd = primitive_cmd::set_pixel_at;
    p.pixel_desc = {pos, color};
    _display_controller->add_primitive(p);
}

/**
 * @brief Проводит линию пером от текущей позиции и переносит туда позицию.
 *
 * Начало линии задаёт move_to() или конец предыдущей line_to().
 *
 * @param x Абсцисса конца.
 * @param y Ордината конца.
 */
void esp::gfx::canvas::line_to(int x, int y)
{
    primitive p;
    p.cmd = primitive_cmd::line_to;
    p.position = point(x, y);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует отрезок пером; текущая позиция переносится в его конец.
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 */
void esp::gfx::canvas::draw_line(int x1, int y1, int x2, int y2)
{
    move_to(x1, y1);
    line_to(x2, y2);
}

/**
 * @brief Рисует рамку прямоугольника пером.
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
void esp::gfx::canvas::draw_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::draw_rect;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует рамку прямоугольника пером.
 * @param rect Прямоугольник.
 */
void esp::gfx::canvas::draw_rectangle(const rect &rect)
{
    draw_rectangle(rect.x1, rect.y1, rect.x2, rect.y2);
}

/**
 * @brief Заливает прямоугольник кистью.
 *
 * Рамку пером нужно рисовать отдельно, draw_rectangle().
 *
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
void esp::gfx::canvas::fill_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::fill_rect;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}

/**
 * @brief Заливает прямоугольник кистью.
 * @param rect Прямоугольник.
 */
void esp::gfx::canvas::fill_rectangle(const rect &rect)
{
    primitive p;
    p.cmd = primitive_cmd::fill_rect;
    p.rect = rect;
    _display_controller->add_primitive(p);
}

/**
 * @brief Инвертирует пиксели прямоугольника.
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
void esp::gfx::canvas::invert_rectangle(int x1, int y1, int x2, int y2)
{
    invert_rectangle(rect(x1, y1, x2, y2));
}

/**
 * @brief Инвертирует пиксели прямоугольника.
 * @param rect Прямоугольник.
 */
void esp::gfx::canvas::invert_rectangle(const rect &rect)
{
    primitive p;
    p.cmd = primitive_cmd::invert_rect;
    p.rect = rect;
    _display_controller->add_primitive(p);
}

/**
 * @brief Меняет в прямоугольнике цвета пера и кисти.
 *
 * Пиксели цвета пера становятся цвета кисти и наоборот; другие цвета не
 * меняются.
 *
 * @param x1 Левая граница.
 * @param y1 Верхняя граница.
 * @param x2 Правая граница.
 * @param y2 Нижняя граница.
 */
void esp::gfx::canvas::swap_rectangle(int x1, int y1, int x2, int y2)
{
    primitive p;
    p.cmd = primitive_cmd::swap_fgbg;
    p.rect = rect(x1, y1, x2, y2);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует эллипс пером; текущая позиция переносится в его центр.
 * @param x Абсцисса центра.
 * @param y Ордината центра.
 * @param width Ширина эллипса.
 * @param height Высота эллипса.
 */
void esp::gfx::canvas::draw_ellipse(int x, int y, int width, int height)
{
    move_to(x, y);
    primitive p;
    p.cmd = primitive_cmd::draw_ellipse;
    p.size = size(width, height);
    _display_controller->add_primitive(p);
}

/**
 * @brief Заливает эллипс кистью; текущая позиция переносится в его центр.
 * @param x Абсцисса центра.
 * @param y Ордината центра.
 * @param width Ширина эллипса.
 * @param height Высота эллипса.
 */
void esp::gfx::canvas::fill_ellipse(int x, int y, int width, int height)
{
    move_to(x, y);
    primitive p;
    p.cmd = primitive_cmd::fill_ellipse;
    p.size = size(width, height);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует знак — однобитное изображение — цветами пера и кисти.
 *
 * Вид знака задают параметры знаков (set_glyph_options()). Изображение
 * не копируется и должно существовать, пока примитив не выполнен.
 *
 * @param x Левая граница.
 * @param y Верхняя граница.
 * @param width Ширина знака, точки.
 * @param height Высота знака, точки.
 * @param data Изображения знаков: строки по (@p width + 7) / 8 байтов,
 *        старший разряд — левая точка.
 * @param index Номер знака, если в @p data их несколько подряд.
 */
void esp::gfx::canvas::draw_glyph(int x, int y, int width, int height, const std::uint8_t *data,
                                  int index)
{
    primitive p;
    p.cmd = primitive_cmd::draw_glyph;
    p.glyph = glyph(x, y, width, height, data + index * height * ((width + 7) / 8));
    _display_controller->add_primitive(p);
}

/**
 * @brief Задаёт параметры рисования следующих знаков: фон, начертание, инверсию.
 * @param options Параметры знаков.
 */
void esp::gfx::canvas::set_glyph_options(glyph_options options)
{
    primitive p;
    p.cmd = primitive_cmd::set_glyph_options;
    p.glyph_options = options;
    _display_controller->add_primitive(p);
}

/**
 * @brief Сбрасывает параметры знаков: непрозрачные знаки без украшений.
 */
void esp::gfx::canvas::reset_glyph_options()
{
    set_glyph_options(glyph_options());
}

/**
 * @brief Задаёт общие параметры рисования: обмен цветов и инверсию.
 * @param options Параметры рисования.
 */
void esp::gfx::canvas::set_paint_options(paint_options options)
{
    primitive p;
    p.cmd = primitive_cmd::set_paint_options;
    p.paint_options = options;
    _display_controller->add_primitive(p);
}

/**
 * @brief Сбрасывает общие параметры рисования.
 */
void esp::gfx::canvas::reset_paint_options()
{
    set_paint_options(paint_options());
}

/**
 * @brief Копирует прямоугольник экрана в другое место.
 *
 * Области могут перекрываться. Пиксели копируются по одному, поэтому
 * копирование медленное. Текущая позиция переносится в левый верхний угол
 * назначения.
 *
 * @param source_x Левая граница источника.
 * @param source_y Верхняя граница источника.
 * @param dest_x Левая граница назначения.
 * @param dest_y Верхняя граница назначения.
 * @param width Ширина прямоугольника.
 * @param height Высота прямоугольника.
 */
void esp::gfx::canvas::copy_rect(int source_x, int source_y, int dest_x, int dest_y, int width,
                                 int height)
{
    move_to(dest_x, dest_y);
    const int source_x2 = source_x + width - 1;
    const int source_y2 = source_y + height - 1;
    primitive p;
    p.cmd = primitive_cmd::copy_rect;
    p.rect = rect(source_x, source_y, source_x2, source_y2);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует растр.
 *
 * Растр не копируется и должен существовать, пока примитив не выполнен.
 * Пиксели с нулевым альфа-каналом прозрачны, остальные непрозрачны.
 *
 * @param x Левая граница.
 * @param y Верхняя граница.
 * @param bitmap Растр.
 */
void esp::gfx::canvas::draw_bitmap(int x, int y, const bitmap *bitmap)
{
    primitive p;
    p.cmd = primitive_cmd::draw_bitmap;
    p.bitmap_drawing_info = bitmap_drawing_info(x, y, bitmap);
    _display_controller->add_primitive(p);
}

/**
 * @brief Рисует пером замкнутый многоугольник.
 *
 * Последняя вершина соединяется с первой. Небольшой массив вершин
 * копируется в пул контроллера; больший должен существовать, пока примитив
 * не выполнен.
 *
 * @param points Вершины.
 * @param points_count Число вершин.
 */
void esp::gfx::canvas::draw_path(const point *points, int points_count)
{
    primitive p;
    p.cmd = primitive_cmd::draw_path;
    p.path.points = points;
    p.path.points_count = points_count;
    p.path.free_points = false;
    _display_controller->add_primitive(p);
}

/**
 * @brief Заливает кистью замкнутый многоугольник.
 *
 * Небольшой массив вершин копируется в пул контроллера; больший должен
 * существовать, пока примитив не выполнен.
 *
 * @param points Вершины.
 * @param points_count Число вершин.
 */
void esp::gfx::canvas::fill_path(const point *points, int points_count)
{
    primitive p;
    p.cmd = primitive_cmd::fill_path;
    p.path.points = points;
    p.path.points_count = points_count;
    p.path.free_points = false;
    _display_controller->add_primitive(p);
}

/**
 * @brief Читает пиксель экрана.
 *
 * Чтение может обогнать рисование, которое ещё в очереди; чтобы прочитать
 * нарисованное, сначала вызовите wait_completion().
 *
 * @param x Абсцисса.
 * @param y Ордината.
 * @return Цвет пикселя.
 */
esp::gfx::rgb888 esp::gfx::canvas::pixel(int x, int y)
{
    rgb888 rgb;
    _display_controller->read_screen(rect(x, y, x, y), &rgb);
    return rgb;
}
