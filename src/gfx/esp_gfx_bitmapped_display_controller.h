// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_base_display_controller.h"
#include "esp_gfx_bitmap.h"
#include "esp_gfx_bitmap_drawing_info.h"
#include "esp_gfx_cursor.h"
#include "esp_gfx_cursor_name.h"
#include "esp_gfx_glyph.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_light_memory_pool.h"
#include "esp_gfx_native_pixel_format.h"
#include "esp_gfx_paint_state.h"
#include "esp_gfx_path.h"
#include "esp_gfx_pixel_desc.h"
#include "esp_gfx_point.h"
#include "esp_gfx_primitive.h"
#include "esp_gfx_rect.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_size.h"
#include "esp_gfx_sprite.h"

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>


namespace esp::gfx
{

class bitmapped_display_controller : public base_display_controller
{

public:

    bitmapped_display_controller();
    virtual ~bitmapped_display_controller();

    virtual esp::gfx::native_pixel_format native_pixel_format() = 0;

    inline esp::gfx::paint_state &paint_state();

    void add_primitive(primitive &primitive);
    void primitives_execution_wait();

    void enable_background_primitive_execution(bool value);
    inline void enable_background_primitive_timeout(bool value);
    inline bool background_primitive_timeout_enabled() const;
    virtual void suspend_background_primitive_execution() = 0;
    virtual void resume_background_primitive_execution() = 0;
    void process_primitives();

    template<typename T>
    inline void set_sprites(T *sprites, int count);
    inline void remove_sprites();
    void refresh_sprites();

    inline bool is_double_buffered() const;
    inline bool is_double_buffered_enabled() const;
    virtual bool suspend_double_buffering(bool value);

    void set_mouse_cursor(cursor *cursor);
    void set_mouse_cursor(cursor_name cursor_name);
    void set_mouse_cursor_pos(int x, int y);

    virtual void read_screen(const rect &rect, rgb888 *dest_buf) = 0;

    /**
     * @brief Длина очереди примитивов, элементы.
     *
     * Общая для всех контроллеров; менять её нужно до вызова begin(), потому
     * что очередь создаётся при включении режима. По умолчанию
     * @c esp::gfx::DISPLAY_CONTROLLER_QUEUE_SIZE.
     */
    static int queue_size;

protected:

    virtual void set_pixel_at(const pixel_desc &pixel_desc, esp::gfx::rect &update_rect) = 0;
    virtual void abs_draw_line(int x1, int y1, int x2, int y2, rgb888 color) = 0;
    virtual void raw_fill_row(int y, int x1, int x2, rgb888 color) = 0;
    virtual void draw_ellipse(const size &size, esp::gfx::rect &update_rect) = 0;
    virtual void clear(esp::gfx::rect &update_rect) = 0;
    virtual void v_scroll(int scroll, esp::gfx::rect &update_rect) = 0;
    virtual void h_scroll(int scroll, esp::gfx::rect &update_rect) = 0;
    virtual void draw_glyph(const glyph &glyph, glyph_options glyph_options, rgb888 pen_color,
                            rgb888 brush_color, esp::gfx::rect &update_rect) = 0;
    virtual void invert_rect(const rect &rect, esp::gfx::rect &update_rect) = 0;
    virtual void swap_fgbg(const rect &rect, esp::gfx::rect &update_rect) = 0;
    virtual void copy_rect(const rect &source, esp::gfx::rect &update_rect) = 0;
    virtual void swap_buffers() = 0;
    virtual int bitmap_save_pixel_size() = 0;
    virtual void raw_draw_bitmap_native(int dest_x, int dest_y, const bitmap *bitmap, int x1,
                                        int y1, int x_count, int y_count) = 0;
    virtual void raw_draw_bitmap_mask(int dest_x, int dest_y, const bitmap *bitmap,
                                      void *save_background, int x1, int y1, int x_count,
                                      int y_count) = 0;
    virtual void raw_draw_bitmap_rgba2222(int dest_x, int dest_y, const bitmap *bitmap,
                                          void *save_background, int x1, int y1, int x_count,
                                          int y_count) = 0;
    virtual void raw_draw_bitmap_rgba8888(int dest_x, int dest_y, const bitmap *bitmap,
                                          void *save_background, int x1, int y1, int x_count,
                                          int y_count) = 0;

    void exec_primitive(const primitive &prim, esp::gfx::rect &update_rect, bool inside_isr);
    void reset_paint_state();
    void update_absolute_clipping_rect();
    rgb888 actual_pen_color();
    rgb888 actual_brush_color();

    void line_to(const point &position, esp::gfx::rect &update_rect);
    void draw_rect(const rect &rect, esp::gfx::rect &update_rect);
    void draw_path(const path &path, esp::gfx::rect &update_rect);
    void abs_draw_thick_line(int x1, int y1, int x2, int y2, int pen_width, const rgb888 &color);
    void fill_rect(const rect &rect, const rgb888 &color, esp::gfx::rect &update_rect);
    void fill_ellipse(int center_x, int center_y, const size &size, const rgb888 &color,
                      esp::gfx::rect &update_rect);
    void fill_path(const path &path, const rgb888 &color, esp::gfx::rect &update_rect);
    void draw_bitmap(const bitmap_drawing_info &bitmap_drawing_info, esp::gfx::rect &update_rect);
    void abs_draw_bitmap(int dest_x, int dest_y, const bitmap *bitmap, void *save_background,
                         bool ignore_clipping_rect);

    void set_sprites(sprite *sprites, int count, int sprite_size);
    sprite *get_sprite(int index);
    inline int sprites_count() const;
    void hide_sprites(esp::gfx::rect &update_rect);
    void show_sprites(esp::gfx::rect &update_rect);
    inline sprite *mouse_cursor();

    void set_double_buffered(bool value);

    bool get_primitive(primitive *primitive, int time_out_ms = 0);
    bool get_primitive_isr(primitive *primitive);
    void wait_for_primitives();

private:

    void primitive_replace_dynamic_buffers(primitive &primitive);

    /** @brief Текущие параметры рисования; их меняют примитивы по мере выполнения. */
    esp::gfx::paint_state _paint_state;

    /** @brief Включён ли режим с двойной буферизацией. */
    volatile bool _double_buffered_setup;

    /**
     * @brief Действует ли двойная буферизация сейчас.
     *
     * В режиме с двойной буферизацией её можно приостановить
     * (suspend_double_buffering()); тогда рисование идёт прямо в видимый
     * буфер.
     */
    volatile bool _double_buffered_enabled;

    /**
     * @brief Очередь примитивов, которую выполняет обработчик кадровой развёртки.
     *
     * В режиме двойной буферизации в ней один элемент: add_primitive()
     * ждёт, пока буферы поменяются местами (см. set_double_buffered()).
     */
    volatile QueueHandle_t _exec_queue;

    /**
     * @brief Запасная очередь для приостановленной двойной буферизации.
     *
     * Создаётся при первой приостановке; suspend_double_buffering() меняет
     * её местами с @c _exec_queue.
     */
    volatile QueueHandle_t _exec_queue_alt;

    /**
     * @brief Выполнять ли примитивы в фоне.
     *
     * @c true — примитивы встают в очередь и выполняются на обратном ходе
     * луча; @c false — выполняются сразу в add_primitive().
     */
    bool _background_primitive_execution_enabled;

    /**
     * @brief Ограничено ли время выполнения примитивов в обработчике кадровой развёртки.
     *
     * @c volatile: флаг читает обработчик прерывания.
     */
    volatile bool _background_primitive_timeout_enabled;

    /** @brief Массив спрайтов приложения; @c nullptr — спрайтов нет. */
    void *_sprites;

    /** @brief Размер одного элемента @c _sprites, байты: приложение может расширять @c esp::gfx::sprite. */
    int _sprite_size;

    /** @brief Число спрайтов в @c _sprites. */
    int _sprites_count;

    /** @brief Скрыты ли спрайты: @c true между hide_sprites() и show_sprites(). */
    bool _sprites_hidden;

    /** @brief Спрайт курсора мыши; рисуется поверх остальных спрайтов. */
    sprite _mouse_cursor;

    /** @brief Горизонтальное положение горячей точки текущего курсора в его изображении. */
    std::int16_t _mouse_hotspot_x;

    /** @brief Вертикальное положение горячей точки текущего курсора в его изображении. */
    std::int16_t _mouse_hotspot_y;

    /**
     * @brief Пул памяти для данных примитивов.
     *
     * Сюда копируются вершины многоугольников; размер —
     * @c esp::gfx::PRIMITIVE_BUFFER_SIZE.
     */
    light_memory_pool _prim_dyn_mem_pool;
};

}


// Реализация

// Открытые

/**
 * @fn esp::gfx::native_pixel_format esp::gfx::bitmapped_display_controller::native_pixel_format()
 * @brief Возвращает собственный формат пикселя контроллера.
 * @return Формат пикселя буфера кадра.
 */

/**
 * @brief Возвращает текущие параметры рисования.
 *
 * Их меняют примитивы; производные контроллеры читают их при рисовании.
 *
 * @return Ссылка на параметры рисования.
 */
esp::gfx::paint_state &esp::gfx::bitmapped_display_controller::paint_state()
{
    return _paint_state;
}

/**
 * @brief Включает или выключает ограничение времени рисования в обработчике кадровой развёртки.
 *
 * Без ограничения рисование быстрее, но изображение может мерцать: примитивы
 * выполняются и за пределами обратного хода луча.
 *
 * @param value @c true — ограничивать (по умолчанию).
 */
void esp::gfx::bitmapped_display_controller::enable_background_primitive_timeout(bool value)
{
    _background_primitive_timeout_enabled = value;
}

/**
 * @brief Сообщает, ограничено ли время рисования в обработчике кадровой развёртки.
 * @return @c true, если ограничено.
 */
bool esp::gfx::bitmapped_display_controller::background_primitive_timeout_enabled() const
{
    return _background_primitive_timeout_enabled;
}

/**
 * @fn void esp::gfx::bitmapped_display_controller::suspend_background_primitive_execution()
 * @brief Приостанавливает выполнение примитивов в обработчике кадровой развёртки.
 *
 * Вызовы можно вкладывать: контроллер считает их. Пока выполнение
 * приостановлено, очередь может переполниться, и добавление примитива
 * зависнет, поэтому нужно часто вызывать process_primitives().
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::resume_background_primitive_execution()
 * @brief Возобновляет выполнение примитивов после suspend_background_primitive_execution().
 */

/**
 * @brief Устанавливает список спрайтов.
 *
 * Спрайтов может быть сколько угодно, но много больших спрайтов замедляют
 * обновление экрана и вызывают мерцание. Тип элемента может расширять
 * @c esp::gfx::sprite полями приложения:
 * @code
 * struct my_sprite : esp::gfx::sprite
 * {
 *     int velocity_x;
 *     int velocity_y;
 * };
 *
 * static my_sprite sprites[10];
 *
 * display.set_sprites(sprites, 10);
 * @endcode
 *
 * @tparam T Тип элемента массива спрайтов.
 * @param sprites Массив спрайтов; должен существовать, пока установлен.
 * @param count Число спрайтов.
 */
template<typename T>
void esp::gfx::bitmapped_display_controller::set_sprites(T *sprites, int count)
{
    set_sprites(sprites, count, sizeof(T));
}

/**
 * @brief Убирает все спрайты.
 */
void esp::gfx::bitmapped_display_controller::remove_sprites()
{
    set_sprites(nullptr, 0, 0);
}

/**
 * @brief Сообщает, включён ли режим с двойной буферизацией.
 * @return @c true, если режим включён с двойной буферизацией.
 */
bool esp::gfx::bitmapped_display_controller::is_double_buffered() const
{
    return _double_buffered_setup;
}

/**
 * @brief Сообщает, действует ли двойная буферизация сейчас.
 *
 * Двойную буферизацию можно приостановить
 * (suspend_double_buffering()).
 *
 * @return @c true, если режим включён с двойной буферизацией и она не
 *         приостановлена.
 */
bool esp::gfx::bitmapped_display_controller::is_double_buffered_enabled() const
{
    return _double_buffered_enabled;
}

/**
 * @fn void esp::gfx::bitmapped_display_controller::read_screen(const rect &rect, rgb888 *dest_buf)
 * @brief Читает пиксели экрана.
 * @param rect Прямоугольник экрана.
 * @param dest_buf Буфер на ширину × высоту прямоугольника цветов, построчно.
 */

// Защищённые

/**
 * @fn void esp::gfx::bitmapped_display_controller::set_pixel_at(const pixel_desc &pixel_desc, esp::gfx::rect &update_rect)
 * @brief Рисует точку заданного цвета.
 * @param pixel_desc Положение относительно начала координат и цвет.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::abs_draw_line(int x1, int y1, int x2, int y2, rgb888 color)
 * @brief Рисует линию в абсолютных координатах с отсечением и текущей толщиной пера.
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 * @param color Цвет.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::raw_fill_row(int y, int x1, int x2, rgb888 color)
 * @brief Заливает отрезок строки без отсечения.
 * @param y Строка.
 * @param x1 Первый столбец.
 * @param x2 Последний столбец, включительно.
 * @param color Цвет.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::draw_ellipse(const size &size, esp::gfx::rect &update_rect)
 * @brief Рисует эллипс пером с центром в текущей позиции.
 * @param size Размеры эллипса.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::clear(esp::gfx::rect &update_rect)
 * @brief Заливает область вывода цветом кисти.
 * @param update_rect Изменённая область.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::v_scroll(int scroll, esp::gfx::rect &update_rect)
 * @brief Прокручивает область прокрутки по вертикали.
 * @param scroll Шаг: отрицательный — вверх, положительный — вниз.
 * @param update_rect Изменённая область.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::h_scroll(int scroll, esp::gfx::rect &update_rect)
 * @brief Прокручивает область прокрутки по горизонтали.
 * @param scroll Шаг: отрицательный — влево, положительный — вправо.
 * @param update_rect Изменённая область.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::draw_glyph(const glyph &glyph, glyph_options glyph_options, rgb888 pen_color, rgb888 brush_color, esp::gfx::rect &update_rect)
 * @brief Рисует знак.
 * @param glyph Знак и его положение относительно начала координат.
 * @param glyph_options Параметры рисования знаков.
 * @param pen_color Цвет точек знака.
 * @param brush_color Цвет фона знакоместа.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::invert_rect(const rect &rect, esp::gfx::rect &update_rect)
 * @brief Инвертирует пиксели прямоугольника.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::swap_fgbg(const rect &rect, esp::gfx::rect &update_rect)
 * @brief Меняет в прямоугольнике цвета пера и кисти; другие цвета не трогает.
 * @param rect Прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::copy_rect(const rect &source, esp::gfx::rect &update_rect)
 * @brief Копирует прямоугольник в текущую позицию; области могут перекрываться.
 * @param source Исходный прямоугольник относительно начала координат.
 * @param update_rect Изменённая область; расширяется на нарисованное.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::swap_buffers()
 * @brief Меняет местами видимый буфер кадра и буфер рисования.
 */

/**
 * @fn int esp::gfx::bitmapped_display_controller::bitmap_save_pixel_size()
 * @brief Возвращает размер пикселя сохранённого под спрайтом фона.
 * @return Байты на пиксель в собственном формате контроллера.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::raw_draw_bitmap_native(int dest_x, int dest_y, const bitmap *bitmap, int x1, int y1, int x_count, int y_count)
 * @brief Переносит часть растра собственного формата на экран без отсечения.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::raw_draw_bitmap_mask(int dest_x, int dest_y, const bitmap *bitmap, void *save_background, int x1, int y1, int x_count, int y_count)
 * @brief Переносит часть маски на экран без отсечения, сохраняя фон.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата маски.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::raw_draw_bitmap_rgba2222(int dest_x, int dest_y, const bitmap *bitmap, void *save_background, int x1, int y1, int x_count, int y_count)
 * @brief Переносит часть растра RGBA2222 на экран без отсечения, сохраняя фон.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата RGBA2222.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */

/**
 * @fn void esp::gfx::bitmapped_display_controller::raw_draw_bitmap_rgba8888(int dest_x, int dest_y, const bitmap *bitmap, void *save_background, int x1, int y1, int x_count, int y_count)
 * @brief Переносит часть растра RGBA8888 на экран без отсечения, сохраняя фон.
 * @param dest_x Левая граница на экране.
 * @param dest_y Верхняя граница на экране.
 * @param bitmap Растр формата RGBA8888.
 * @param save_background Буфер для фона под растром или @c nullptr.
 * @param x1 Первый столбец растра.
 * @param y1 Первая строка растра.
 * @param x_count Число столбцов.
 * @param y_count Число строк.
 */

/**
 * @brief Возвращает число установленных спрайтов.
 * @return Число спрайтов, без курсора мыши.
 */
int esp::gfx::bitmapped_display_controller::sprites_count() const
{
    return _sprites_count;
}

/**
 * @brief Возвращает спрайт курсора мыши.
 * @return Спрайт курсора.
 */
esp::gfx::sprite *esp::gfx::bitmapped_display_controller::mouse_cursor()
{
    return &_mouse_cursor;
}
