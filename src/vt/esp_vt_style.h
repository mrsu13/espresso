#pragma once


/**
 * @file
 * @brief Вид текста знакоместа esp::vt::style: цвета и признаки.
 */


#include "esp_vt_config.h"

#include <cstdint>


namespace esp::vt
{

/**
 * @brief Вид текста: номера цветов ANSI и признаки, всего 2 байта.
 *
 * Цвета хранятся номерами ANSI 0–15, а не цветами палитры: с палитрой
 * видеорежима их сопоставляет рисование, поэтому смена палитры или режима
 * перекрашивает экран. Признаки рисует вывод знаков esp::gfx; в 2-цветных
 * режимах только они и выделяют текст, поэтому выделение строится на них, а
 * цвет его лишь дополняет.
 *
 * Тот же тип описывает и текущий вид текста экрана — с ним выводятся новые
 * знаки, его меняют последовательности @c SGR.
 */
struct style
{
    bool operator==(const esp::vt::style &other) const = default;

    /** @brief Цвет текста — номер цвета ANSI, 0–15; по умолчанию @c esp::vt::DEFAULT_FOREGROUND. */
    std::uint8_t foreground : 4 = esp::vt::DEFAULT_FOREGROUND;

    /** @brief Цвет фона — номер цвета ANSI, 0–15; по умолчанию @c esp::vt::DEFAULT_BACKGROUND. */
    std::uint8_t background : 4 = esp::vt::DEFAULT_BACKGROUND;

    /** @brief Жирный (@c SGR 1, отмена — @c SGR 22). */
    bool bold : 1 = false;

    /** @brief Тусклый, с пониженной яркостью (@c SGR 2, отмена — @c SGR 22). */
    bool dim : 1 = false;

    /** @brief Курсив (@c SGR 3, отмена — @c SGR 23). */
    bool italic : 1 = false;

    /** @brief Подчёркнутый (@c SGR 4, отмена — @c SGR 24). */
    bool underlined : 1 = false;

    /** @brief Инверсия: цвета текста и фона меняются местами (@c SGR 7, отмена — @c SGR 27). */
    bool inverse : 1 = false;

    /** @brief Скрытый: знак не рисуется, видно только фон (@c SGR 8, отмена — @c SGR 28). */
    bool hidden : 1 = false;
};

static_assert(sizeof(esp::vt::style) == 2, "esp::vt::style must take 2 bytes.");

}


// Реализация

// Открытые

/**
 * @fn bool esp::vt::style::operator==(const esp::vt::style &other) const
 * @brief Сравнивает вид текста: равны ли цвета и все признаки.
 * @param other Другой вид текста.
 * @return @c true, если цвета и признаки совпадают.
 */
