// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Основа контроллеров дисплея esp::gfx::base_display_controller.
 */


#include "esp_non_copyable.h"

#include <cstdint>


namespace esp::gfx
{

/**
 * @brief Основа всех контроллеров дисплея.
 *
 * Задаёт то, что есть у любого дисплея: выбор режима, запуск, число цветов,
 * размеры экрана и области вывода. Размеры хранятся в полях, а не
 * возвращаются виртуальными методами: таблицы виртуальных методов лежат во
 * флеш-памяти, а размеры нужны и рисованию в обработчике прерывания.
 *
 * Контроллер владеет оборудованием и буферами, поэтому копировать его
 * нельзя; удалять его через указатель на эту основу тоже нельзя.
 */
class base_display_controller : esp::non_copyable
{

public:

    virtual void set_resolution(const char *modeline, int view_port_width = -1,
                                int view_port_height = -1, bool double_buffered = false) = 0;

    virtual void begin() = 0;

    virtual int colors_count() = 0;

    inline int screen_width() const;
    inline int screen_height() const;
    inline int view_port_width() const;
    inline int view_port_height() const;

protected:

    ~base_display_controller() = default;

    inline void set_screen_size(int width, int height);

    /** @brief Ширина экрана в текущем режиме, пиксели; задаёт set_screen_size(). */
    std::int16_t _screen_width;

    /** @brief Высота экрана в текущем режиме, пиксели; задаёт set_screen_size(). */
    std::int16_t _screen_height;

    /**
     * @brief Ширина области вывода, пиксели.
     *
     * Область вывода — часть экрана, в которой рисуют примитивы; она может
     * быть меньше экрана. @c volatile: значение читает рисование в
     * обработчике прерывания.
     */
    volatile std::int16_t _view_port_width;

    /**
     * @brief Высота области вывода, пиксели.
     *
     * @c volatile: значение читает рисование в обработчике прерывания.
     */
    volatile std::int16_t _view_port_height;
};

}


// Реализация

// Открытые

/**
 * @fn void esp::gfx::base_display_controller::set_resolution(const char *modeline, int view_port_width, int view_port_height, bool double_buffered)
 * @brief Включает видеорежим.
 *
 * @param modeline Видеорежим в формате строки XFree86 Modeline, например
 *        одна из констант esp_vga_modelines.h.
 * @param view_port_width Ширина области вывода; −1 — во весь экран.
 * @param view_port_height Высота области вывода; −1 — во весь экран.
 * @param double_buffered @c true — двойная буферизация: рисование идёт во
 *        второй буфер, а буферы меняются местами по команде.
 */

/**
 * @fn void esp::gfx::base_display_controller::begin()
 * @brief Настраивает выводы и оборудование контроллера с параметрами по умолчанию.
 */

/**
 * @fn int esp::gfx::base_display_controller::colors_count()
 * @brief Возвращает число цветов, которые может показать дисплей в текущем режиме.
 * @return Число цветов.
 */

/**
 * @brief Возвращает ширину экрана.
 * @return Ширина, пиксели.
 */
int esp::gfx::base_display_controller::screen_width() const
{
    return _screen_width;
}

/**
 * @brief Возвращает высоту экрана.
 * @return Высота, пиксели.
 */
int esp::gfx::base_display_controller::screen_height() const
{
    return _screen_height;
}

/**
 * @brief Возвращает ширину области вывода.
 * @return Ширина, пиксели.
 */
int esp::gfx::base_display_controller::view_port_width() const
{
    return _view_port_width;
}

/**
 * @brief Возвращает высоту области вывода.
 * @return Высота, пиксели.
 */
int esp::gfx::base_display_controller::view_port_height() const
{
    return _view_port_height;
}

// Защищённые

/**
 * @brief Запоминает размер экрана; производный класс вызывает его, когда режим известен.
 * @param width Ширина, пиксели.
 * @param height Высота, пиксели.
 */
void esp::gfx::base_display_controller::set_screen_size(int width, int height)
{
    _screen_width = width;
    _screen_height = height;
}
