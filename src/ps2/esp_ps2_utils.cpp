// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_utils.h
 * @brief Объявление вспомогательных функций модуля esp::ps2.
 */

/**
 * @file esp_ps2_utils.cpp
 * @brief Вспомогательные функции PS/2: целый квадратный корень, чётность.
 */

#include "esp_ps2_utils.h"

#include <cstdint>


#pragma GCC optimize("O2")


// Открытые

/**
 * @brief Вычисляет целый квадратный корень.
 *
 * Метод Холлека с ускорением Legalize: разряды корня подбираются от
 * старшего к младшему без умножений.
 *
 * @param x Число.
 * @return Целая часть корня; 0 для чисел меньше 1.
 */
int esp::ps2::isqrt(int x)
{
    if (x < 1)
        return 0;
    int squaredbit = 0x40000000;
    int remainder = x;
    int root = 0;
    while (squaredbit > 0)
    {
        if (remainder >= (squaredbit | root))
        {
            remainder -= (squaredbit | root);
            root >>= 1;
            root |= squaredbit;
        }
        else
            root >>= 1;
        squaredbit >>= 2;
    }
    return root;
}

/**
 * @brief Вычисляет чётность числа единиц в байте.
 *
 * Байт сворачивается до четырёх разрядов, а их чётность берётся из
 * 16-разрядной таблицы 0x6996.
 *
 * @param v Байт.
 * @return @c true, если в байте нечётное число единиц.
 */
bool esp::ps2::calc_parity(std::uint8_t v)
{
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}
