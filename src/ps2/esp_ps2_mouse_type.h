// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Тип мыши esp::ps2::mouse_type.
 */


namespace esp::ps2
{

/**
 * @brief Тип мыши, определённый при сбросе.
 */
enum class mouse_type
{
    /** @brief Обычная мышь PS/2: три кнопки, перемещения по двум осям, пакет из 3 байтов. */
    legacy_mouse,

    /** @brief Мышь, совместимая с Microsoft IntelliMouse: ещё колесо, пакет из 4 байтов. */
    intellimouse,
};

}
