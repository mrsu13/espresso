/**
 * @file esp_gfx_font.h
 * @brief Объявление класса шрифта esp::gfx::font.
 */

/**
 * @file esp_gfx_font.cpp
 * @brief Растровый шрифт Unicode для вывода текста на холст esp::gfx::Canvas.
 */

#include "esp_gfx_font.h"

#include "utf8.h"

#include <algorithm>


/**
 * @class esp::gfx::font
 * @brief Растровый моноширинный шрифт, индексированный по кодам Unicode.
 *
 * Шрифт — таблица кодов знаков по возрастанию и изображения знаков в том
 * же порядке, по 1 биту на точку, в формате esp::gfx::Canvas::drawGlyph().
 * Такие таблицы строит сценарий components/consolefonts/ttf2font.py.
 *
 * Знак ищется двоичным поиском; вместо отсутствующего выводится знак
 * замены. Цвет знака — цвет пера холста; фон знакоместа заливается
 * цветом кисти, если это включено в параметрах знаков холста
 * (esp::gfx::Canvas::setGlyphOptions()).
 */


// Открытые

/**
 * @brief Создаёт шрифт по готовым таблицам.
 *
 * Таблицы не копируются и должны существовать, пока существует шрифт;
 * обычно это константы во флеш-памяти.
 *
 * @param width Ширина знакоместа, точки.
 * @param height Высота знакоместа, точки.
 * @param codepoints Коды знаков по возрастанию.
 * @param glyphs Изображения знаков в порядке @p codepoints: строка точек
 *        занимает целое число байтов, старший бит — левая точка.
 * @param count Число знаков.
 * @param fallback Код знака, который выводится вместо отсутствующих; если
 *        и его нет в шрифте, выводится первый знак таблицы.
 */
esp::gfx::font::font(int width, int height, const std::uint16_t *codepoints,
                     const std::uint8_t *glyphs, std::size_t count, char32_t fallback)
    : _width(width)
    , _height(height)
    , _codepoints(codepoints)
    , _glyphs(glyphs)
    , _count(count)
    , _fallback(0)
{
    const int i = find(fallback);
    if (i >= 0)
        _fallback = i;
}

/**
 * @brief Возвращает номер знака в таблицах шрифта.
 * @param codepoint Код Unicode.
 * @return Номер знака; для отсутствующего — номер знака замены.
 */
int esp::gfx::font::index(char32_t codepoint) const
{
    const int i = find(codepoint);
    return i >= 0 ? i : _fallback;
}

/**
 * @brief Рисует один знак.
 * @param canvas Холст.
 * @param x Абсцисса левого верхнего угла знакоместа.
 * @param y Ордината левого верхнего угла знакоместа.
 * @param codepoint Код Unicode.
 */
void esp::gfx::font::draw_char(esp::gfx::Canvas &canvas, int x, int y, char32_t codepoint) const
{
    // Холст сам находит изображение по номеру знака: смещение — номер,
    // умноженный на размер изображения одного знака.
    canvas.drawGlyph(x, y, _width, _height, _glyphs, index(codepoint));
}

/**
 * @brief Рисует строку текста, по знаку в знакоместе.
 *
 * Переносов нет: строка рисуется вправо, пока не кончится, в том числе за
 * краем экрана.
 *
 * @param canvas Холст.
 * @param x Абсцисса левого верхнего угла первого знакоместа.
 * @param y Ордината левого верхнего угла первого знакоместа.
 * @param text Строка в кодировке UTF-8, завершённая нулём. Кодировка не
 *        проверяется: строка должна быть правильной UTF-8.
 * @return Число нарисованных знаков.
 */
int esp::gfx::font::draw_text(esp::gfx::Canvas &canvas, int x, int y, const char *text) const
{
    // С C++20 utf8.h работает с типом char8_t, а не char; байты строки
    // при этом те же.
    const utf8_int8_t *p = reinterpret_cast<const utf8_int8_t *>(text);
    int cells = 0;
    while (*p)
    {
        utf8_int32_t codepoint = 0;
        p = utf8codepoint(p, &codepoint);
        draw_char(canvas, x + cells * _width, y, static_cast<char32_t>(codepoint));
        ++cells;
    }
    return cells;
}


// Закрытые

/**
 * @brief Ищет знак двоичным поиском.
 * @param codepoint Код Unicode.
 * @return Номер знака или -1, если его нет в шрифте.
 */
int esp::gfx::font::find(char32_t codepoint) const
{
    // В таблицах только коды базовой многоязычной плоскости, до U+FFFF.
    if (codepoint > 0xFFFF)
        return -1;

    const std::uint16_t *const end = _codepoints + _count;
    const std::uint16_t *const it =
        std::lower_bound(_codepoints, end, static_cast<std::uint16_t>(codepoint));
    return it != end && *it == codepoint ? static_cast<int>(it - _codepoints) : -1;
}
