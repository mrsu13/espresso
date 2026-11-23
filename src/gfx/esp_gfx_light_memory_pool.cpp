// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_gfx_light_memory_pool.h
 * @brief Объявление пула памяти esp::gfx::light_memory_pool.
 */

/**
 * @file esp_gfx_light_memory_pool.cpp
 * @brief Простой пул памяти для данных примитивов рисования.
 */

#include "esp_gfx_light_memory_pool.h"

#include <cstdint>

#include <esp_heap_caps.h>


#pragma GCC optimize("O2")


/**
 * @class esp::gfx::light_memory_pool
 * @brief Небольшой пул памяти с блоками переменного размера.
 *
 * Контроллер дисплея копирует сюда вершины многоугольников, чтобы
 * вызывающему не нужно было хранить их до выполнения примитива. Блок
 * занимает столько байтов, сколько запрошено, плюс 2 байта заголовка;
 * наибольший блок — 32767 байтов. Освобождение только помечает блок
 * свободным и потому годится для обработчика прерывания; соседние
 * свободные блоки сливаются при поиске места в alloc().
 *
 * Пул владеет своей памятью, поэтому копировать его нельзя.
 */


// Открытые

/**
 * @brief Выделяет память пула одним свободным блоком.
 * @param pool_size Полезный размер пула, байты.
 */
esp::gfx::light_memory_pool::light_memory_pool(int pool_size)
{
    _pool_size = pool_size + 2;
    _mem = static_cast<std::uint8_t *>(
        heap_caps_malloc(_pool_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
    mark(0, _pool_size - 2, false);
}

/**
 * @brief Освобождает память пула.
 */
esp::gfx::light_memory_pool::~light_memory_pool()
{
    heap_caps_free(_mem);
}

/**
 * @brief Выделяет блок.
 *
 * Ищет первый свободный блок не меньше @p size, по дороге сливая соседние
 * свободные блоки. Блок большего размера делится: остаток становится
 * новым свободным блоком, если в нём есть место хотя бы под заголовок и
 * байт данных, иначе отдаётся целиком.
 *
 * @param size Нужный размер, байты.
 * @return Блок или @c nullptr, если места нет.
 */
void *esp::gfx::light_memory_pool::alloc(int size)
{
    for (int pos = 0; pos < _pool_size;)
    {
        const std::int16_t block_size = get_size(pos);
        if (is_free(pos))
        {
            if (block_size == size)
            {
                // Блок ровно нужного размера.
                mark(pos, size, true);
                return _mem + pos + 2;
            }
            else if (block_size > size)
            {
                // Блок больше нужного: остаток становится новым свободным блоком,
                // если в нём есть место хотя бы под заголовок и байт данных.
                const int remaining_size = block_size - size - 2;
                if (remaining_size > 0)
                    mark(pos + 2 + size, remaining_size, false);
                else
                    size = block_size;
                mark(pos, size, true);
                return _mem + pos + 2;
            }
            else
            {
                // Блок мал. Если следующий свободен, блоки сливаются, и поиск
                // остаётся на месте; иначе — к следующему блоку.
                const int next_block_pos = pos + 2 + block_size;
                if (next_block_pos < _pool_size && is_free(next_block_pos))
                    mark(pos, block_size + get_size(next_block_pos) + 2, false);
                else
                    pos += block_size + 2;
            }
        }
        else
            pos += block_size + 2;
    }
    return nullptr;
}

// Закрытые

/**
 * @brief Записывает заголовок блока.
 * @param pos Смещение заголовка в @c _mem.
 * @param size Размер блока без заголовка, до 32767 байтов.
 * @param allocated @c true — блок занят.
 */
void esp::gfx::light_memory_pool::mark(int pos, std::int16_t size, bool allocated)
{
    _mem[pos] = size & 0xff;
    _mem[pos + 1] = ((size >> 8) & 0x7f) | (allocated ? 0x80 : 0);
}

/**
 * @brief Читает размер блока из заголовка.
 * @param pos Смещение заголовка в @c _mem.
 * @return Размер блока без заголовка, байты.
 */
std::int16_t esp::gfx::light_memory_pool::get_size(int pos) const
{
    return _mem[pos] | ((_mem[pos + 1] & 0x7f) << 8);
}

/**
 * @brief Сообщает, свободен ли блок.
 * @param pos Смещение заголовка в @c _mem.
 * @return @c true, если блок свободен.
 */
bool esp::gfx::light_memory_pool::is_free(int pos) const
{
    return !(_mem[pos + 1] & 0x80);
}
