// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Обработчик события esp::ps2::delegate.
 */


#include "esp_ps2_utils.h"

#include <cstdint>

#include <esp_heap_caps.h>


namespace esp::ps2
{

/**
 * @brief Обработчик события: хранит копию лямбды или другого функционального объекта.
 *
 * Обработчику присваивают функциональный объект, а вызов обработчика
 * вызывает его копию; пустой обработчик ничего не делает:
 * @code
 * keyboard->on_virtual_key = [](esp::ps2::virtual_key *vk, bool down) { ... };
 * @endcode
 * Копия объекта лежит во внутренней ОЗУ, выделенной @c heap_caps_malloc().
 * Копировать сам обработчик нельзя.
 *
 * @tparam Params Типы параметров вызова; передаются по константной ссылке.
 *
 * @bug Функциональный объект копируется побайтово, 32-разрядными словами:
 *      захваты с нетривиальным копированием копируются неправильно, а
 *      хвост размера, не кратного 4 байтам, теряется. Память прежнего
 *      объекта освобождается без вызова его деструктора.
 */
template<typename... Params>
struct delegate
{
    inline delegate();
    template<typename Func>
    inline delegate(Func f);
    delegate(const delegate &c) = delete;
    inline ~delegate();

    template<typename Func>
    inline void operator=(Func f);
    void operator=(const delegate &) = delete;

    inline void operator()(const Params &...params);

private:

    inline void clean_up();

    /** @brief Переходник, который вызывает хранимый объект его настоящего типа. */
    void (*_closure)(void *func, const Params &...params);

    /** @brief Копия функционального объекта; @c nullptr — обработчик пуст. */
    void *_func;
};

}


// Реализация

// Открытые

/**
 * @brief Создаёт пустой обработчик.
 */
template<typename... Params>
esp::ps2::delegate<Params...>::delegate()
    : _closure{nullptr}
    , _func{nullptr}
{
}

/**
 * @brief Создаёт обработчик из функционального объекта.
 *
 * Преобразование неявное: так обработчик можно передать лямбдой.
 *
 * @tparam Func Тип функционального объекта.
 * @param f Функциональный объект; копируется.
 */
template<typename... Params>
template<typename Func>
esp::ps2::delegate<Params...>::delegate(Func f)
    : delegate()
{
    *this = f;
}

/**
 * @brief Освобождает копию функционального объекта.
 */
template<typename... Params>
esp::ps2::delegate<Params...>::~delegate()
{
    clean_up();
}

/**
 * @brief Заменяет функциональный объект.
 * @tparam Func Тип функционального объекта.
 * @param f Функциональный объект; копируется.
 */
template<typename... Params>
template<typename Func>
void esp::ps2::delegate<Params...>::operator=(Func f)
{
    clean_up();
    _closure = [](void *func, const Params &...params) -> void
    { (*static_cast<Func *>(func))(params...); };
    _func = heap_caps_malloc(sizeof(Func), MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    move_items<std::uint32_t *>(static_cast<std::uint32_t *>(_func),
                                reinterpret_cast<std::uint32_t *>(&f),
                                sizeof(Func) / sizeof(std::uint32_t));
}

/**
 * @brief Вызывает функциональный объект; пустой обработчик ничего не делает.
 * @param params Параметры вызова.
 */
template<typename... Params>
void esp::ps2::delegate<Params...>::operator()(const Params &...params)
{
    if (_func)
        _closure(_func, params...);
}

// Закрытые

/**
 * @brief Освобождает копию функционального объекта, если она есть.
 */
template<typename... Params>
void esp::ps2::delegate<Params...>::clean_up()
{
    if (_func)
        heap_caps_free(_func);
}
