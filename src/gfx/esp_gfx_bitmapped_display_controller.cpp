// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_bitmapped_display_controller.h
 * @brief Объявление растрового контроллера дисплея esp::gfx::bitmapped_display_controller.
 */

/**
 * @file esp_gfx_bitmapped_display_controller.cpp
 * @brief Растровый контроллер дисплея: очередь примитивов, спрайты, курсор мыши,
 *        фигуры.
 */

/**
 * @namespace esp::gfx
 * @brief Графика: геометрия, цвета, растры, спрайты, примитивы рисования,
 *        растровый контроллер дисплея и холст.
 */

#include "esp_gfx_bitmapped_display_controller.h"

#include "esp_gfx_bitmapped_display_controller_p.h"
#include "esp_gfx_config.h"
#include "esp_gfx_utils.h"
#include "esp_tools.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


#pragma GCC optimize("O2")


/**
 * @class esp::gfx::bitmapped_display_controller
 * @brief Основа контроллеров дисплея с буфером кадра: очередь примитивов, спрайты, фигуры.
 *
 * Холст @c esp::gfx::canvas ставит примитивы рисования в очередь
 * (add_primitive()), а контроллер выполняет их в обработчике кадровой
 * развёртки, на обратном ходе луча, чтобы изображение не мерцало. Фоновое
 * выполнение можно выключить — тогда примитивы выполняются сразу.
 *
 * Сам класс выполняет примитивы (exec_primitive()) и рисует то, что
 * сводится к строкам и линиям: прямоугольники, заливку эллипсов и
 * многоугольников, толстые линии, растры с отсечением. Работу с пикселями
 * — линии, точки, знаки, прокрутку, перенос растров — делают чистые
 * виртуальные методы производного контроллера; готовые шаблоны для них
 * даёт @c esp::gfx::generic_bitmapped_display_controller.
 *
 * Контроллер ведёт спрайты и курсор мыши: перед рисованием в области под
 * ними восстанавливает сохранённый фон (hide_sprites()), а после рисования
 * рисует их снова (show_sprites()).
 *
 * Методы, помеченные @c IRAM_ATTR, выполняются в обработчике прерывания
 * кадровой развёртки.
 *
 * @bug fill_path() держит точки пересечения строки с многоугольником в
 *      массиве переменной длины на стеке обработчика прерывания; руководство
 *      по стилю запрещает такие массивы, а большой многоугольник может
 *      переполнить стек.
 */


int esp::gfx::bitmapped_display_controller::queue_size = esp::gfx::DISPLAY_CONTROLLER_QUEUE_SIZE;


// Открытые

/**
 * @brief Создаёт контроллер без очереди и без спрайтов.
 *
 * Очередь примитивов создаёт set_double_buffered() при включении режима.
 * Примитивы выполняются в фоне, двойная буферизация выключена, курсор мыши
 * не показан.
 */
esp::gfx::bitmapped_display_controller::bitmapped_display_controller()
    : _prim_dyn_mem_pool(esp::gfx::PRIMITIVE_BUFFER_SIZE)
{
    _exec_queue = nullptr;
    _exec_queue_alt = nullptr;
    _background_primitive_execution_enabled = true;
    _sprites = nullptr;
    _sprites_count = 0;
    _double_buffered_setup = false;
    _double_buffered_enabled = false;
    _mouse_cursor.visible = false;
    _background_primitive_timeout_enabled = true;
    _sprites_hidden = true;
}

/**
 * @brief Удаляет очереди примитивов.
 */
esp::gfx::bitmapped_display_controller::~bitmapped_display_controller()
{
    // Очереди нет, если режим так и не был задан.
    if (_exec_queue)
        vQueueDelete(_exec_queue);
    if (_exec_queue_alt)
        vQueueDelete(_exec_queue_alt);
}

/**
 * @brief Ставит примитив в очередь или выполняет его сразу.
 *
 * Примитив встаёт в очередь, если выполнение в фоне включено, а двойная
 * буферизация не действует, а также всегда — команда обмена буферов. Если
 * очередь полна, метод ждёт. В режиме двойной буферизации после обмена
 * буферов метод ждёт уведомления от обработчика, выполнившего обмен.
 * Иначе примитив выполняется сразу, и спрайты рисуются заново.
 *
 * Вершины многоугольника копируются в пул контроллера, поэтому вызывающему
 * не нужно хранить их до выполнения.
 *
 * @param primitive Примитив; вершины многоугольника в нём могут быть
 *        заменены копией.
 */
void esp::gfx::bitmapped_display_controller::add_primitive(primitive &primitive)
{
    if ((_background_primitive_execution_enabled && !_double_buffered_enabled)
        || primitive.cmd == primitive_cmd::swap_buffers)
    {
        primitive_replace_dynamic_buffers(primitive);
        xQueueSendToBack(_exec_queue, &primitive, portMAX_DELAY);

        if (_double_buffered_enabled)
        {
            // Ждать уведомления от обработчика, выполнившего обмен буферов.
            ulTaskNotifyTake(true, portMAX_DELAY);
        }
    }
    else
    {
        esp::gfx::rect update_rect{SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN};
        exec_primitive(primitive, update_rect, false);
        show_sprites(update_rect);
    }
}

/**
 * @brief Ждёт, пока очередь примитивов опустеет.
 *
 * Ждёт активно, не отдавая процессор; если выполнение в фоне выключено,
 * возвращается сразу.
 */
void esp::gfx::bitmapped_display_controller::primitives_execution_wait()
{
    if (_background_primitive_execution_enabled)
        while (uxQueueMessagesWaiting(_exec_queue) > 0)
        {
        }
}

/**
 * @brief Включает или выключает выполнение примитивов в фоне.
 *
 * В фоне примитивы выполняются на обратном ходе луча (для VGA) и не
 * вызывают мерцания. Выключение сразу выполняет все примитивы очереди, и
 * дальше они выполняются в момент добавления. Вызовы не вкладываются.
 *
 * @param value @c true — выполнять в фоне, @c false — сразу.
 */
void esp::gfx::bitmapped_display_controller::enable_background_primitive_execution(bool value)
{
    if (value != _background_primitive_execution_enabled)
    {
        if (value)
            resume_background_primitive_execution();
        else
        {
            suspend_background_primitive_execution();
            process_primitives();
        }
        _background_primitive_execution_enabled = value;
    }
}

/**
 * @brief Выполняет все примитивы очереди сейчас.
 *
 * Рисование идёт не на обратном ходе луча и может мерцать. Вызывать из
 * обработчика прерывания нельзя.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::process_primitives()
{
    suspend_background_primitive_execution();
    esp::gfx::rect update_rect{SHRT_MAX, SHRT_MAX, SHRT_MIN, SHRT_MIN};
    primitive prim;
    while (xQueueReceive(_exec_queue, &prim, 0) == pdTRUE)
        exec_primitive(prim, update_rect, false);
    show_sprites(update_rect);
    resume_background_primitive_execution();
    primitive p{primitive_cmd::refresh, update_rect};
    add_primitive(p);
}

/**
 * @brief Перерисовывает спрайты.
 *
 * Экран обновляется сам, когда рисует примитив. Если же изменился спрайт —
 * изображение, положение или видимость, — его нужно перерисовать этим
 * методом; в режиме двойной буферизации без этого спрайты не рисуются
 * вовсе.
 */
void esp::gfx::bitmapped_display_controller::refresh_sprites()
{
    primitive p{primitive_cmd::refresh_sprites};
    add_primitive(p);
}

/**
 * @brief Приостанавливает или возобновляет двойную буферизацию.
 *
 * Приостановить можно, только если режим включён с двойной буферизацией;
 * тогда рисование идёт прямо в видимый буфер через отдельную очередь.
 * Сначала выполняются все ждущие примитивы.
 *
 * @param value @c true — приостановить, @c false — возобновить.
 * @return Прежнее состояние: @c true, если двойная буферизация была
 *         приостановлена.
 */
bool esp::gfx::bitmapped_display_controller::suspend_double_buffering(bool value)
{
    const bool ret = !_double_buffered_enabled;
    if (_double_buffered_setup && _double_buffered_enabled == value)
    {
        process_primitives();
        if (value && !_exec_queue_alt)
        {
            // Запасная очередь создаётся при первой приостановке.
            _exec_queue_alt =
                xQueueCreate(bitmapped_display_controller::queue_size, sizeof(primitive));
        }
        std::swap(_exec_queue_alt, _exec_queue);
        _double_buffered_enabled = !value;
    }
    return ret;
}

/**
 * @brief Устанавливает курсор мыши и показывает его.
 *
 * Если курсор тот же, ничего не происходит. Позиция мыши сохраняется: при
 * смене курсора спрайт сдвигается на разницу горячих точек.
 *
 * @param cursor Курсор; должен существовать, пока установлен. @c nullptr
 *        — скрыть курсор.
 */
void esp::gfx::bitmapped_display_controller::set_mouse_cursor(cursor *cursor)
{
    if (!cursor || &cursor->bitmap != _mouse_cursor.frame())
    {
        _mouse_cursor.visible = false;
        _mouse_cursor.clear_bitmaps();

        refresh_sprites();
        process_primitives();
        primitives_execution_wait();

        if (cursor)
        {
            _mouse_cursor.move_by(+_mouse_hotspot_x, +_mouse_hotspot_y);
            _mouse_hotspot_x = cursor->hotspot_x;
            _mouse_hotspot_y = cursor->hotspot_y;
            _mouse_cursor.add_bitmap(&cursor->bitmap);
            _mouse_cursor.visible = true;
            _mouse_cursor.move_by(-_mouse_hotspot_x, -_mouse_hotspot_y);
            if (!is_double_buffered_enabled())
                _mouse_cursor.saved_background = static_cast<std::uint8_t *>(std::realloc(
                    _mouse_cursor.saved_background,
                    cursor->bitmap.width * bitmap_save_pixel_size() * cursor->bitmap.height));
        }
        refresh_sprites();
    }
}

/**
 * @brief Устанавливает встроенный курсор мыши и показывает его.
 * @param cursor_name Встроенный курсор.
 */
void esp::gfx::bitmapped_display_controller::set_mouse_cursor(cursor_name cursor_name)
{
    set_mouse_cursor(&esp::gfx::p::CURSORS[static_cast<int>(cursor_name)]);
}

/**
 * @brief Переносит курсор мыши: горячая точка встаёт в заданное место.
 * @param x Абсцисса горячей точки.
 * @param y Ордината горячей точки.
 */
void esp::gfx::bitmapped_display_controller::set_mouse_cursor_pos(int x, int y)
{
    _mouse_cursor.move_to(x - _mouse_hotspot_x, y - _mouse_hotspot_y);
    refresh_sprites();
}

// Защищённые

/**
 * @brief Выполняет примитив.
 *
 * Вызывается из обработчика кадровой развёртки или из process_primitives()
 * и add_primitive(). Изменённую примитивом область добавляет к
 * @p update_rect, чтобы потом нарисовать в ней спрайты.
 *
 * @param prim Примитив.
 * @param update_rect Изменённая область; расширяется.
 * @param inside_isr @c true, если вызов из обработчика прерывания: так
 *        выбирается функция уведомления задачи после обмена буферов.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::exec_primitive(const primitive &prim,
                                                                      esp::gfx::rect &update_rect,
                                                                      bool inside_isr)
{
    switch (prim.cmd)
    {
        case primitive_cmd::refresh:
            update_rect = update_rect.merge(prim.rect);
            break;

        case primitive_cmd::reset:
            reset_paint_state();
            break;

        case primitive_cmd::set_pen_color:
            paint_state().pen_color = prim.color;
            break;

        case primitive_cmd::set_brush_color:
            paint_state().brush_color = prim.color;
            break;

        case primitive_cmd::set_pixel:
            set_pixel_at(pixel_desc{prim.position, actual_pen_color()}, update_rect);
            break;

        case primitive_cmd::set_pixel_at:
            set_pixel_at(prim.pixel_desc, update_rect);
            break;

        case primitive_cmd::move_to:
            paint_state().position = point(prim.position.x + paint_state().origin.x,
                                           prim.position.y + paint_state().origin.y);
            break;

        case primitive_cmd::line_to:
            line_to(prim.position, update_rect);
            break;

        case primitive_cmd::fill_rect:
            fill_rect(prim.rect, actual_brush_color(), update_rect);
            break;

        case primitive_cmd::draw_rect:
            draw_rect(prim.rect, update_rect);
            break;

        case primitive_cmd::fill_ellipse:
            fill_ellipse(paint_state().position.x, paint_state().position.y, prim.size,
                         actual_brush_color(), update_rect);
            break;

        case primitive_cmd::draw_ellipse:
            draw_ellipse(prim.size, update_rect);
            break;

        case primitive_cmd::clear:
            update_rect =
                update_rect.merge(rect(0, 0, view_port_width() - 1, view_port_height() - 1));
            clear(update_rect);
            break;

        case primitive_cmd::v_scroll:
            update_rect = update_rect.merge(
                rect(paint_state().scrolling_region.x1, paint_state().scrolling_region.y1,
                     paint_state().scrolling_region.x2, paint_state().scrolling_region.y2));
            v_scroll(prim.ivalue, update_rect);
            break;

        case primitive_cmd::h_scroll:
            update_rect = update_rect.merge(
                rect(paint_state().scrolling_region.x1, paint_state().scrolling_region.y1,
                     paint_state().scrolling_region.x2, paint_state().scrolling_region.y2));
            h_scroll(prim.ivalue, update_rect);
            break;

        case primitive_cmd::draw_glyph:
            draw_glyph(prim.glyph, paint_state().glyph_options, paint_state().pen_color,
                       paint_state().brush_color, update_rect);
            break;

        case primitive_cmd::set_glyph_options:
            paint_state().glyph_options = prim.glyph_options;
            break;

        case primitive_cmd::set_paint_options:
            paint_state().paint_options = prim.paint_options;
            break;

        case primitive_cmd::invert_rect:
            invert_rect(prim.rect, update_rect);
            break;

        case primitive_cmd::copy_rect:
            copy_rect(prim.rect, update_rect);
            break;

        case primitive_cmd::set_scrolling_region:
            paint_state().scrolling_region = prim.rect;
            break;

        case primitive_cmd::swap_fgbg:
            swap_fgbg(prim.rect, update_rect);
            break;

        case primitive_cmd::draw_bitmap:
            draw_bitmap(prim.bitmap_drawing_info, update_rect);
            break;

        case primitive_cmd::refresh_sprites:
            hide_sprites(update_rect);
            show_sprites(update_rect);
            break;

        case primitive_cmd::swap_buffers:
            swap_buffers();
            update_rect =
                update_rect.merge(rect(0, 0, view_port_width() - 1, view_port_height() - 1));
            if (inside_isr)
                vTaskNotifyGiveFromISR(prim.notify_task, nullptr);
            else
                xTaskNotifyGive(prim.notify_task);
            break;

        case primitive_cmd::draw_path:
            draw_path(prim.path, update_rect);
            break;

        case primitive_cmd::fill_path:
            fill_path(prim.path, actual_brush_color(), update_rect);
            break;

        case primitive_cmd::set_origin:
            paint_state().origin = prim.position;
            update_absolute_clipping_rect();
            break;

        case primitive_cmd::set_clipping_rect:
            paint_state().clipping_rect = prim.rect;
            update_absolute_clipping_rect();
            break;

        case primitive_cmd::set_pen_width:
            paint_state().pen_width = std::max<int>(1, prim.ivalue);
            break;

        case primitive_cmd::set_line_ends:
            paint_state().line_ends = prim.line_ends;
            break;
    }
}

/**
 * @brief Возвращает параметры рисования к начальным.
 *
 * Перо белое, кисть чёрная, позиция и начало координат (0, 0), области
 * прокрутки и отсечения — вся область вывода, перо толщиной 1 без
 * дополнения концов, параметры знаков и рисования сброшены.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::reset_paint_state()
{
    _paint_state.pen_color = rgb888(255, 255, 255);
    _paint_state.brush_color = rgb888(0, 0, 0);
    _paint_state.position = point(0, 0);
    _paint_state.glyph_options.value = 0; // Все параметры знаков сброшены.
    _paint_state.paint_options = paint_options();
    _paint_state.scrolling_region = rect(0, 0, view_port_width() - 1, view_port_height() - 1);
    _paint_state.origin = point(0, 0);
    _paint_state.clipping_rect = rect(0, 0, view_port_width() - 1, view_port_height() - 1);
    _paint_state.abs_clipping_rect = _paint_state.clipping_rect;
    _paint_state.pen_width = 1;
    _paint_state.line_ends = line_ends::none;
}

/**
 * @brief Пересчитывает действующую область отсечения.
 *
 * Сдвигает заданную холстом область на начало координат и обрезает её по
 * области вывода; та к этому времени задана (ширина и высота не меньше 1).
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::update_absolute_clipping_rect()
{
    const int x1 = std::clamp(paint_state().origin.x + paint_state().clipping_rect.x1, 0,
                              view_port_width() - 1);
    const int y1 = std::clamp(paint_state().origin.y + paint_state().clipping_rect.y1, 0,
                              view_port_height() - 1);
    const int x2 = std::clamp(paint_state().origin.x + paint_state().clipping_rect.x2, 0,
                              view_port_width() - 1);
    const int y2 = std::clamp(paint_state().origin.y + paint_state().clipping_rect.y2, 0,
                              view_port_height() - 1);
    paint_state().abs_clipping_rect = rect(x1, y1, x2, y2);
}

/**
 * @brief Возвращает действующий цвет пера с учётом обмена цветов.
 * @return Цвет кисти, если обмен цветов включён, иначе цвет пера.
 */
esp::gfx::rgb888 IRAM_ATTR esp::gfx::bitmapped_display_controller::actual_pen_color()
{
    return paint_state().paint_options.swap_fgbg ? paint_state().brush_color
                                                 : paint_state().pen_color;
}

/**
 * @brief Возвращает действующий цвет кисти с учётом обмена цветов.
 * @return Цвет пера, если обмен цветов включён, иначе цвет кисти.
 */
esp::gfx::rgb888 IRAM_ATTR esp::gfx::bitmapped_display_controller::actual_brush_color()
{
    return paint_state().paint_options.swap_fgbg ? paint_state().pen_color
                                                 : paint_state().brush_color;
}

/**
 * @brief Проводит линию пером от текущей позиции и переносит туда позицию.
 * @param position Конец линии относительно начала координат.
 * @param update_rect Изменённая область; расширяется с учётом толщины пера.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::line_to(const point &position,
                                                               esp::gfx::rect &update_rect)
{
    const rgb888 color = actual_pen_color();

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;
    const int x1 = paint_state().position.x;
    const int y1 = paint_state().position.y;
    const int x2 = position.x + orig_x;
    const int y2 = position.y + orig_y;

    // Изменённая область — с запасом на половину толщины пера.
    const int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(rect(std::min(x1, x2) - hw, std::min(y1, y2) - hw,
                                         std::max(x1, x2) + hw, std::max(y1, y2) + hw));
    hide_sprites(update_rect);
    abs_draw_line(x1, y1, x2, y2, color);

    paint_state().position = point(x2, y2);
}

/**
 * @brief Рисует рамку прямоугольника пером.
 *
 * Четыре стороны рисуются отдельными линиями так, что углы не рисуются
 * дважды: это важно при инверсии пикселей.
 *
 * @param rect Прямоугольник относительно начала координат; углы могут
 *        быть заданы в любом порядке.
 * @param update_rect Изменённая область; расширяется с учётом толщины пера.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::draw_rect(const rect &rect,
                                                                 esp::gfx::rect &update_rect)
{
    const int x1 = (rect.x1 < rect.x2 ? rect.x1 : rect.x2) + paint_state().origin.x;
    const int y1 = (rect.y1 < rect.y2 ? rect.y1 : rect.y2) + paint_state().origin.y;
    const int x2 = (rect.x1 < rect.x2 ? rect.x2 : rect.x1) + paint_state().origin.x;
    const int y2 = (rect.y1 < rect.y2 ? rect.y2 : rect.y1) + paint_state().origin.y;

    const int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(esp::gfx::rect(x1 - hw, y1 - hw, x2 + hw, y2 + hw));
    hide_sprites(update_rect);
    const rgb888 color = actual_pen_color();

    // Стороны не перекрываются в углах: при инверсии угол не инвертируется дважды.

    abs_draw_line(x1 + 1, y1, x2, y1, color);
    abs_draw_line(x2, y1 + 1, x2, y2, color);
    abs_draw_line(x2 - 1, y2, x1, y2, color);
    abs_draw_line(x1, y2 - 1, x1, y1, color);
}

/**
 * @brief Рисует замкнутый многоугольник пером.
 *
 * Если вершины лежат в пуле контроллера, после рисования они
 * освобождаются.
 *
 * @param path Многоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::draw_path(const path &path,
                                                                 esp::gfx::rect &update_rect)
{
    const rgb888 color = actual_pen_color();

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    // Изменённая область — вся ширина области отсечения между крайними вершинами.
    const int min_x = clip_x1;
    const int max_x = clip_x2 + 1;
    int min_y = INT_MAX;
    int max_y = 0;
    for (int i = 0; i < path.points_count; ++i)
    {
        const int py = path.points[i].y + orig_y;
        if (py < min_y)
            min_y = py;
        if (py > max_y)
            max_y = py;
    }
    min_y = std::max(clip_y1, min_y);
    max_y = std::min(clip_y2, max_y);

    const int hw = paint_state().pen_width / 2;
    update_rect = update_rect.merge(rect(min_x - hw, min_y - hw, max_x + hw, max_y + hw));
    hide_sprites(update_rect);

    int i = 0;
    for (; i < path.points_count - 1; ++i)
    {
        const int x1 = path.points[i].x + orig_x;
        const int y1 = path.points[i].y + orig_y;
        const int x2 = path.points[i + 1].x + orig_x;
        const int y2 = path.points[i + 1].y + orig_y;
        abs_draw_line(x1, y1, x2, y2, color);
    }
    const int x1 = path.points[i].x + orig_x;
    const int y1 = path.points[i].y + orig_y;
    const int x2 = path.points[0].x + orig_x;
    const int y2 = path.points[0].y + orig_y;
    abs_draw_line(x1, y1, x2, y2, color);

    if (path.free_points)
        _prim_dyn_mem_pool.free(const_cast<point *>(path.points));
}

/**
 * @brief Рисует толстую линию в абсолютных координатах.
 *
 * Линия — залитый четырёхугольник, повёрнутый по направлению линии; при
 * круглых концах на концах рисуются круги диаметром в толщину пера
 * (чётная толщина уменьшается на 1, чтобы у круга был центр).
 *
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 * @param pen_width Толщина, пиксели.
 * @param color Цвет.
 *
 * @bug Круглые концы сдвинуты на начало координат, если оно не (0, 0):
 *      fill_ellipse() ждёт абсолютные координаты центра, а получает
 *      относительные.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::abs_draw_thick_line(int x1, int y1, int x2,
                                                                           int y2, int pen_width,
                                                                           const rgb888 &color)
{
    // Координаты снова относительные: fill_path() прибавит к ним начало координат.
    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;
    x1 -= orig_x;
    y1 -= orig_y;
    x2 -= orig_x;
    y2 -= orig_y;

    point pts[4];

    // Четырёхугольник: концы линии, сдвинутые на половину толщины по нормали в обе стороны.
    const double angle = std::atan2(y2 - y1, x2 - x1);
    const double pw = static_cast<double>(pen_width) / 2.0;
    const int ofs1 = static_cast<int>(std::lround(pw * std::cos(angle + M_PI_2)));
    const int ofs2 = static_cast<int>(std::lround(pw * std::sin(angle + M_PI_2)));
    const int ofs3 = static_cast<int>(std::lround(pw * std::cos(angle - M_PI_2)));
    const int ofs4 = static_cast<int>(std::lround(pw * std::sin(angle - M_PI_2)));
    pts[0].x = x1 + ofs1;
    pts[0].y = y1 + ofs2;
    pts[1].x = x1 + ofs3;
    pts[1].y = y1 + ofs4;
    pts[2].x = x2 + ofs3;
    pts[2].y = y2 + ofs4;
    pts[3].x = x2 + ofs1;
    pts[3].y = y2 + ofs2;

    esp::gfx::rect update_rect;
    const esp::gfx::path path{pts, 4, false};
    fill_path(path, color, update_rect);

    switch (paint_state().line_ends)
    {
        case line_ends::circle:
            // У круга нечётного диаметра есть центральный пиксель.
            if (!(pen_width & 1))
                --pen_width;
            fill_ellipse(x1, y1, size(pen_width, pen_width), color, update_rect);
            fill_ellipse(x2, y2, size(pen_width, pen_width), color, update_rect);
            break;
        default:
            break;
    }
}

/**
 * @brief Заливает прямоугольник.
 * @param rect Прямоугольник относительно начала координат; углы могут
 *        быть заданы в любом порядке.
 * @param color Цвет заливки.
 * @param update_rect Изменённая область; расширяется.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::fill_rect(const rect &rect,
                                                                 const rgb888 &color,
                                                                 esp::gfx::rect &update_rect)
{
    int x1 = (rect.x1 < rect.x2 ? rect.x1 : rect.x2) + paint_state().origin.x;
    int y1 = (rect.y1 < rect.y2 ? rect.y1 : rect.y2) + paint_state().origin.y;
    int x2 = (rect.x1 < rect.x2 ? rect.x2 : rect.x1) + paint_state().origin.x;
    int y2 = (rect.y1 < rect.y2 ? rect.y2 : rect.y1) + paint_state().origin.y;

    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    if (x1 > clip_x2 || x2 < clip_x1 || y1 > clip_y2 || y2 < clip_y1)
        return;

    // Пересечение с областью отсечения.
    x1 = std::max(x1, clip_x1);
    y1 = std::max(y1, clip_y1);
    x2 = std::min(x2, clip_x2);
    y2 = std::min(y2, clip_y2);

    update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
    hide_sprites(update_rect);

    for (int y = y1; y <= y2; ++y)
        raw_fill_row(y, x1, x2, color);
}

/**
 * @brief Заливает эллипс по алгоритму Макилроя.
 *
 * Эллипс строится по четвертям целочисленным алгоритмом и заливается
 * горизонтальными отрезками.
 *
 * @param center_x Абсцисса центра в абсолютных координатах.
 * @param center_y Ордината центра в абсолютных координатах.
 * @param size Размеры эллипса.
 * @param color Цвет заливки.
 * @param update_rect Изменённая область; расширяется.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::fill_ellipse(int center_x, int center_y,
                                                                    const size &size,
                                                                    const rgb888 &color,
                                                                    esp::gfx::rect &update_rect)
{
    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

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
    int width = 1;
    int t = -a2 * y;
    int dxt = 2 * b2 * x;
    int dyt = -2 * a2 * y;

    while (y >= 0 && x <= half_width)
    {
        if (t + b2 * x <= crit1 || t + a2 * y <= crit3)
        {
            ++x;
            dxt += d2xt;
            t += dxt;
            width += 2;
        }
        else
        {
            int col1 = center_x - x;
            int col2 = center_x - x + width - 1;
            if (col1 <= clip_x2 && col2 >= clip_x1)
            {
                col1 = std::max(col1, clip_x1);
                col2 = std::min(col2, clip_x2);
                int row1 = center_y - y;
                int row2 = center_y + y;
                if (row1 >= clip_y1 && row1 <= clip_y2)
                    raw_fill_row(row1, col1, col2, color);
                if (y != 0 && row2 >= clip_y1 && row2 <= clip_y2)
                    raw_fill_row(row2, col1, col2, color);
            }
            if (t - a2 * y <= crit2)
            {
                ++x;
                dxt += d2xt;
                t += dxt;
                width += 2;
            }
            --y;
            dyt += d2yt;
            t += dyt;
        }
    }
    // Эллипс высотой в одну строку: цикл выше его не рисует.
    if (half_height == 0 && center_y >= clip_y1 && center_y <= clip_y2)
    {
        const int col1 = center_x - half_width;
        const int col2 = center_x + half_width;
        if (col1 <= clip_x2 && col2 >= clip_x1)
            raw_fill_row(center_y, std::max(col1, clip_x1), std::min(col2, clip_x2), color);
    }
}

/**
 * @brief Заливает замкнутый многоугольник.
 *
 * Для каждой строки находятся точки пересечения с рёбрами, сортируются, и
 * отрезки между парами точек заливаются. Если вершины лежат в пуле
 * контроллера, после заливки они освобождаются.
 *
 * @param path Многоугольник относительно начала координат.
 * @param color Цвет заливки.
 * @param update_rect Изменённая область; расширяется.
 *
 * @bug Точки пересечения хранятся в массиве переменной длины на стеке
 *      обработчика прерывания.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::fill_path(const path &path,
                                                                 const rgb888 &color,
                                                                 esp::gfx::rect &update_rect)
{
    const int clip_x1 = paint_state().abs_clipping_rect.x1;
    const int clip_y1 = paint_state().abs_clipping_rect.y1;
    const int clip_x2 = paint_state().abs_clipping_rect.x2;
    const int clip_y2 = paint_state().abs_clipping_rect.y2;

    const int orig_x = paint_state().origin.x;
    const int orig_y = paint_state().origin.y;

    const int min_x = clip_x1;
    const int max_x = clip_x2 + 1;
    int min_y = INT_MAX;
    int max_y = 0;
    for (int i = 0; i < path.points_count; ++i)
    {
        const int py = path.points[i].y + orig_y;
        if (py < min_y)
            min_y = py;
        if (py > max_y)
            max_y = py;
    }
    min_y = std::max(clip_y1, min_y);
    max_y = std::min(clip_y2, max_y);

    update_rect = update_rect.merge(rect(min_x, min_y, max_x, max_y));
    hide_sprites(update_rect);

    // Точки пересечения строки с рёбрами.
    std::int16_t node_x[path.points_count];

    for (int pixel_y = min_y; pixel_y <= max_y; ++pixel_y)
    {
        // Пересечения строки с рёбрами, округлённые вверх.
        int nodes = 0;
        int j = path.points_count - 1;
        for (int i = 0; i < path.points_count; ++i)
        {
            const int piy = path.points[i].y + orig_y;
            const int pjy = path.points[j].y + orig_y;
            if ((piy < pixel_y && pjy >= pixel_y) || (pjy < pixel_y && piy >= pixel_y))
            {
                const int pjx = path.points[j].x + orig_x;
                const int pix = path.points[i].x + orig_x;
                const int a = (pixel_y - piy) * (pjx - pix);
                const int b = (pjy - piy);
                node_x[nodes++] = pix + a / b + (((a < 0) ^ (b > 0)) && (a % b));
            }
            j = i;
        }

        // Сортировка пересечений вставками.
        int i = 0;
        while (i < nodes - 1)
        {
            if (node_x[i] > node_x[i + 1])
            {
                std::swap(node_x[i], node_x[i + 1]);
                if (i)
                    --i;
            }
            else
                ++i;
        }

        // Отрезки между парами пересечений заливаются.
        for (int i = 0; i < nodes; i += 2)
        {
            if (node_x[i] >= max_x)
                break;
            if (node_x[i + 1] > min_x)
            {
                if (node_x[i] < min_x)
                    node_x[i] = min_x;
                if (node_x[i + 1] > max_x)
                    node_x[i + 1] = max_x;
                raw_fill_row(pixel_y, node_x[i], node_x[i + 1] - 1, color);
            }
        }
    }

    if (path.free_points)
        _prim_dyn_mem_pool.free(const_cast<point *>(path.points));
}

/**
 * @brief Рисует растр с отсечением.
 * @param bitmap_drawing_info Растр и его положение относительно начала координат.
 * @param update_rect Изменённая область; расширяется.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::draw_bitmap(
    const bitmap_drawing_info &bitmap_drawing_info, esp::gfx::rect &update_rect)
{
    const int x = bitmap_drawing_info.x + paint_state().origin.x;
    const int y = bitmap_drawing_info.y + paint_state().origin.y;
    update_rect = update_rect.merge(rect(x, y, x + bitmap_drawing_info.bitmap->width - 1,
                                         y + bitmap_drawing_info.bitmap->height - 1));
    hide_sprites(update_rect);
    abs_draw_bitmap(x, y, bitmap_drawing_info.bitmap, nullptr, false);
}

/**
 * @brief Рисует растр в абсолютных координатах с отсечением, сохраняя фон.
 *
 * Отсекает растр областью отсечения или, для спрайтов, областью вывода и
 * переносит видимую часть методом производного контроллера по формату
 * растра.
 *
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param ignore_clipping_rect @c true — отсекать только областью вывода
 *        (так рисуются спрайты).
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::abs_draw_bitmap(int dest_x, int dest_y,
                                                                       const bitmap *bitmap,
                                                                       void *save_background,
                                                                       bool ignore_clipping_rect)
{
    const int clip_x1 = ignore_clipping_rect ? 0 : paint_state().abs_clipping_rect.x1;
    const int clip_y1 = ignore_clipping_rect ? 0 : paint_state().abs_clipping_rect.y1;
    const int clip_x2 =
        ignore_clipping_rect ? view_port_width() - 1 : paint_state().abs_clipping_rect.x2;
    const int clip_y2 =
        ignore_clipping_rect ? view_port_height() - 1 : paint_state().abs_clipping_rect.y2;

    if (dest_x > clip_x2 || dest_y > clip_y2)
        return;

    const int width = bitmap->width;
    const int height = bitmap->height;

    // Видимая часть растра: первый столбец и число столбцов, затем строки.
    int x1 = 0;
    int x_count = width;

    if (dest_x < clip_x1)
    {
        x1 = clip_x1 - dest_x;
        dest_x = clip_x1;
    }
    if (x1 >= width)
        return;

    if (dest_x + x_count > clip_x2 + 1)
        x_count = clip_x2 + 1 - dest_x;
    if (x1 + x_count > width)
        x_count = width - x1;

    int y1 = 0;
    int y_count = height;

    if (dest_y < clip_y1)
    {
        y1 = clip_y1 - dest_y;
        dest_y = clip_y1;
    }
    if (y1 >= height)
        return;

    if (dest_y + y_count > clip_y2 + 1)
        y_count = clip_y2 + 1 - dest_y;
    if (y1 + y_count > height)
        y_count = height - y1;

    switch (bitmap->format)
    {

        case pixel_format::undefined:
            break;

        case pixel_format::native:
            raw_draw_bitmap_native(dest_x, dest_y, bitmap, x1, y1, x_count, y_count);
            break;

        case pixel_format::mask:
            raw_draw_bitmap_mask(dest_x, dest_y, bitmap, save_background, x1, y1, x_count, y_count);
            break;

        case pixel_format::rgba2222:
            raw_draw_bitmap_rgba2222(dest_x, dest_y, bitmap, save_background, x1, y1, x_count,
                                     y_count);
            break;

        case pixel_format::rgba8888:
            raw_draw_bitmap_rgba8888(dest_x, dest_y, bitmap, save_background, x1, y1, x_count,
                                     y_count);
            break;
    }
}

/**
 * @brief Устанавливает список спрайтов.
 *
 * Сначала выполняет все ждущие примитивы. Если двойная буферизация не
 * действует, каждому спрайту выделяется буфер фона по самому большому
 * кадру.
 *
 * @param sprites Массив спрайтов или @c nullptr.
 * @param count Число спрайтов.
 * @param sprite_size Размер элемента массива, байты.
 */
void esp::gfx::bitmapped_display_controller::set_sprites(sprite *sprites, int count,
                                                         int sprite_size)
{
    process_primitives();
    primitives_execution_wait();
    _sprites = sprites;
    _sprite_size = sprite_size;
    _sprites_count = count;

    // Буферы фона по самому большому кадру; при двойной буферизации фон не сохраняется.
    if (!is_double_buffered_enabled())
    {
        std::uint8_t *sprite_ptr = static_cast<std::uint8_t *>(_sprites);
        for (int i = 0; i < _sprites_count; ++i, sprite_ptr += _sprite_size)
        {
            esp::gfx::sprite *const sprite = reinterpret_cast<esp::gfx::sprite *>(sprite_ptr);
            int req_back_buffer_size = 0;
            for (int frame = 0; frame < sprite->frames_count; ++frame)
                req_back_buffer_size =
                    std::max(req_back_buffer_size,
                             sprite->frames[frame]->width * bitmap_save_pixel_size()
                                 * sprite->frames[frame]->height);
            if (req_back_buffer_size > 0)
                sprite->saved_background = static_cast<std::uint8_t *>(
                    std::realloc(sprite->saved_background, req_back_buffer_size));
        }
    }
}

/**
 * @brief Возвращает спрайт по номеру.
 * @param index Номер спрайта.
 * @return Спрайт; элементы массива могут быть больше @c esp::gfx::sprite.
 */
esp::gfx::sprite *IRAM_ATTR esp::gfx::bitmapped_display_controller::get_sprite(int index)
{
    return reinterpret_cast<esp::gfx::sprite *>(static_cast<std::uint8_t *>(_sprites)
                                                + index * _sprite_size);
}

/**
 * @brief Скрывает спрайты и курсор мыши, восстанавливая фон под ними.
 *
 * Вызывается перед рисованием, чтобы примитив не затёр спрайт и не испортил
 * сохранённый фон. Спрайты восстанавливаются в обратном порядке.
 * Повторный вызов до show_sprites() ничего не делает.
 *
 * @param update_rect Изменённая область; расширяется на восстановленный фон.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::hide_sprites(esp::gfx::rect &update_rect)
{
    if (!_sprites_hidden)
    {
        _sprites_hidden = true;

        // Спрайты: фон восстанавливается в обратном порядке рисования.
        if (sprites_count() > 0 && !is_double_buffered_enabled())
        {
            for (int i = sprites_count() - 1; i >= 0; --i)
            {
                esp::gfx::sprite *const sprite = get_sprite(i);
                if (sprite->allow_draw && sprite->saved_background_width > 0)
                {
                    const int saved_x = sprite->saved_x;
                    const int saved_y = sprite->saved_y;
                    const int saved_width = sprite->saved_background_width;
                    const int saved_height = sprite->saved_background_height;
                    const esp::gfx::bitmap bitmap{saved_width, saved_height,
                                                  sprite->saved_background, pixel_format::native};
                    abs_draw_bitmap(saved_x, saved_y, &bitmap, nullptr, true);
                    update_rect = update_rect.merge(rect(
                        saved_x, saved_y, saved_x + saved_width - 1, saved_y + saved_height - 1));
                    sprite->saved_background_width = sprite->saved_background_height = 0;
                }
            }
        }

        // Курсор мыши.
        esp::gfx::sprite *const mouse_sprite = mouse_cursor();
        if (mouse_sprite->saved_background_width > 0)
        {
            const int saved_x = mouse_sprite->saved_x;
            const int saved_y = mouse_sprite->saved_y;
            const int saved_width = mouse_sprite->saved_background_width;
            const int saved_height = mouse_sprite->saved_background_height;
            const esp::gfx::bitmap bitmap{saved_width, saved_height, mouse_sprite->saved_background,
                                          pixel_format::native};
            abs_draw_bitmap(saved_x, saved_y, &bitmap, nullptr, true);
            update_rect = update_rect.merge(
                rect(saved_x, saved_y, saved_x + saved_width - 1, saved_y + saved_height - 1));
            mouse_sprite->saved_background_width = mouse_sprite->saved_background_height = 0;
        }
    }
}

/**
 * @brief Рисует спрайты и курсор мыши, сохраняя фон под ними.
 *
 * Положение спрайта читается один раз, потому что другие задачи могут
 * менять его во время рисования. Статический спрайт после рисования
 * перестаёт перерисовываться. Повторный вызов до hide_sprites() ничего не
 * делает.
 *
 * @param update_rect Изменённая область; расширяется на нарисованные спрайты.
 */
void IRAM_ATTR esp::gfx::bitmapped_display_controller::show_sprites(esp::gfx::rect &update_rect)
{
    if (_sprites_hidden)
    {
        _sprites_hidden = false;

        // Спрайты: фон сохраняется, спрайт рисуется.
        for (int i = 0; i < sprites_count(); ++i)
        {
            esp::gfx::sprite *const sprite = get_sprite(i);
            if (sprite->visible && sprite->allow_draw && sprite->frame())
            {
                // Положение читается один раз: другие задачи могут менять его во время рисования.
                const int sprite_x = sprite->x;
                const int sprite_y = sprite->y;
                const esp::gfx::bitmap *const bitmap = sprite->frame();
                const int bitmap_width = bitmap->width;
                const int bitmap_height = bitmap->height;
                abs_draw_bitmap(sprite_x, sprite_y, bitmap, sprite->saved_background, true);
                sprite->saved_x = sprite_x;
                sprite->saved_y = sprite_y;
                sprite->saved_background_width = bitmap_width;
                sprite->saved_background_height = bitmap_height;
                if (sprite->is_static)
                    sprite->allow_draw = false;
                update_rect = update_rect.merge(rect(
                    sprite_x, sprite_y, sprite_x + bitmap_width - 1, sprite_y + bitmap_height - 1));
            }
        }

        // Курсор мыши — поверх спрайтов.
        esp::gfx::sprite *const mouse_sprite = mouse_cursor();
        if (mouse_sprite->visible && mouse_sprite->frame())
        {
            // Положение читается один раз: другие задачи могут менять его во время рисования.
            const int sprite_x = mouse_sprite->x;
            const int sprite_y = mouse_sprite->y;
            const esp::gfx::bitmap *const bitmap = mouse_sprite->frame();
            const int bitmap_width = bitmap->width;
            const int bitmap_height = bitmap->height;
            abs_draw_bitmap(sprite_x, sprite_y, bitmap, mouse_sprite->saved_background, true);
            mouse_sprite->saved_x = sprite_x;
            mouse_sprite->saved_y = sprite_y;
            mouse_sprite->saved_background_width = bitmap_width;
            mouse_sprite->saved_background_height = bitmap_height;
            update_rect = update_rect.merge(rect(sprite_x, sprite_y, sprite_x + bitmap_width - 1,
                                                 sprite_y + bitmap_height - 1));
        }
    }
}

/**
 * @brief Включает или выключает двойную буферизацию и создаёт очередь примитивов.
 *
 * Производный контроллер вызывает его при включении режима. В режиме
 * двойной буферизации в очереди один элемент: так add_primitive() ждёт
 * обмена буферов.
 *
 * @param value @c true — двойная буферизация.
 */
void esp::gfx::bitmapped_display_controller::set_double_buffered(bool value)
{
    _double_buffered_setup = value;
    _double_buffered_enabled = value;
    if (_exec_queue)
        vQueueDelete(_exec_queue);
    // При двойной буферизации очередь из одного элемента и достаточна, и необходима:
    // add_primitive() ждёт, пока буферы поменяются местами.
    _exec_queue =
        xQueueCreate(value ? 1 : bitmapped_display_controller::queue_size, sizeof(primitive));
}

/**
 * @brief Берёт примитив из очереди.
 * @param primitive Куда записать примитив.
 * @param time_out_ms Сколько ждать, мс; −1 — бесконечно.
 * @return @c true, если примитив получен.
 */
bool esp::gfx::bitmapped_display_controller::get_primitive(primitive *primitive, int time_out_ms)
{
    return xQueueReceive(_exec_queue, primitive, esp::ms_to_ticks(time_out_ms));
}

/**
 * @brief Берёт примитив из очереди в обработчике прерывания, не ожидая.
 * @param primitive Куда записать примитив.
 * @return @c true, если примитив получен.
 */
bool IRAM_ATTR esp::gfx::bitmapped_display_controller::get_primitive_isr(primitive *primitive)
{
    return xQueueReceiveFromISR(_exec_queue, primitive, nullptr);
}

/**
 * @brief Ждёт появления примитива в очереди, не забирая его.
 *
 * Вызывать из обработчика прерывания нельзя.
 */
void esp::gfx::bitmapped_display_controller::wait_for_primitives()
{
    primitive p;
    xQueuePeek(_exec_queue, &p, portMAX_DELAY);
}

// Закрытые

/**
 * @brief Копирует данные примитива, на которые он ссылается, в пул контроллера.
 *
 * Так вызывающему не нужно хранить вершины многоугольника до выполнения
 * примитива; копию освобождает код рисования. Если в пуле нет места, метод
 * ждёт, пока обработчик освободит его. Многоугольники больше пула не
 * копируются.
 *
 * @param primitive Примитив; ссылка на данные заменяется ссылкой на копию.
 */
void esp::gfx::bitmapped_display_controller::primitive_replace_dynamic_buffers(primitive &primitive)
{
    switch (primitive.cmd)
    {
        case primitive_cmd::draw_path:
        case primitive_cmd::fill_path:
        {
            const int sz = primitive.path.points_count * sizeof(point);
            if (sz < PRIMITIVE_BUFFER_SIZE)
            {
                void *newbuf = nullptr;
                // Место в пуле освобождают уже выполненные примитивы — ждать его.
                while (!(newbuf = _prim_dyn_mem_pool.alloc(sz)))
                    taskYIELD();
                std::memcpy(newbuf, primitive.path.points, sz);
                primitive.path.points = static_cast<point *>(newbuf);
                primitive.path.free_points = true;
            }
            break;
        }

        default:
            break;
    }
}
