#pragma once

#include "esp_gfx_canvas.h"

#include <cstddef>
#include <cstdint>


namespace esp::gfx
{

class font
{

public:

    font(int width, int height, const std::uint16_t *codepoints, const std::uint8_t *glyphs,
         std::size_t count, char32_t fallback);

    inline int width() const;
    inline int height() const;

    int index(char32_t codepoint) const;
    void draw_char(canvas &canvas, int x, int y, char32_t codepoint) const;
    int draw_text(canvas &canvas, int x, int y, const char *text) const;

private:

    int find(char32_t codepoint) const;

    /** @brief Ширина знакоместа, точки. */
    int _width;

    /** @brief Высота знакоместа, точки. */
    int _height;

    /** @brief Коды знаков по возрастанию. */
    const std::uint16_t *_codepoints;

    /** @brief Изображения знаков в порядке _codepoints. */
    const std::uint8_t *_glyphs;

    /** @brief Число знаков. */
    std::size_t _count;

    /** @brief Номер знака, который выводится вместо отсутствующих. */
    int _fallback;
};

}


// Реализация

/**
 * @brief Возвращает ширину знакоместа.
 * @return Ширина в точках.
 */
int esp::gfx::font::width() const
{
    return _width;
}

/**
 * @brief Возвращает высоту знакоместа.
 * @return Высота в точках.
 */
int esp::gfx::font::height() const
{
    return _height;
}
