// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include <cstddef>
#include <cstdint>


namespace esp::ps2
{

int isqrt(int x);
bool calc_parity(std::uint8_t v);

template<typename T>
inline T move_items(T dest, T src, std::size_t n);

}


// Реализация

/**
 * @brief Копирует элементы массива; области могут перекрываться.
 *
 * Как @c std::memmove(), но поэлементно, через указатели заданного типа.
 *
 * @tparam T Тип указателя на элемент.
 * @param dest Куда копировать.
 * @param src Откуда копировать.
 * @param n Число элементов.
 * @return @p dest.
 */
template<typename T>
T esp::ps2::move_items(T dest, T src, std::size_t n)
{
    T pd = dest;
    T ps = src;
    if (pd != ps)
    {
        // Копирование с конца, если источник левее назначения, чтобы не затереть
        // ещё не скопированное.
        if (ps < pd)
        {
            for (pd += n, ps += n; n--;)
                *--pd = *--ps;
        }
        else
        {
            while (n--)
                *pd++ = *ps++;
        }
    }
    return dest;
}
