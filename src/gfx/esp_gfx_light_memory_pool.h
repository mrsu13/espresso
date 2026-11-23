// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_non_copyable.h"

#include <cstdint>


namespace esp::gfx
{

class light_memory_pool : esp::non_copyable
{

public:

    explicit light_memory_pool(int pool_size);
    ~light_memory_pool();

    void *alloc(int size);
    inline void free(void *mem);

private:

    void mark(int pos, std::int16_t size, bool allocated);
    inline void mark_free(int pos);
    std::int16_t get_size(int pos) const;
    bool is_free(int pos) const;

    /**
     * @brief Память пула: блоки подряд, каждый с 2-байтовым заголовком.
     *
     * Заголовок — 16-разрядное число, младший байт первым: разряды 0–14 —
     * размер блока без заголовка, разряд 15 — блок занят. Выделяется во
     * внутренней ОЗУ конструктором и освобождается деструктором.
     */
    std::uint8_t *_mem;

    /** @brief Размер @c _mem, байты, вместе с заголовком первого блока. */
    int _pool_size;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает блок в пул.
 *
 * Блок только помечается свободным; со свободными соседями он сливается
 * при следующих выделениях. Метод встраивается в код, поэтому его можно
 * вызывать из рисования в обработчике прерывания.
 *
 * @param mem Блок, полученный от alloc(), или @c nullptr — тогда ничего не
 *        происходит.
 */
void esp::gfx::light_memory_pool::free(void *mem)
{
    if (mem)
        mark_free(static_cast<std::uint8_t *>(mem) - _mem - 2);
}

// Закрытые

/**
 * @brief Помечает блок свободным, не меняя его размера.
 * @param pos Смещение заголовка блока в @c _mem.
 */
void esp::gfx::light_memory_pool::mark_free(int pos)
{
    _mem[pos + 1] &= 0x7f;
}
