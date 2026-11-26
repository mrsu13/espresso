#pragma once

#include "esp_gfx_canvas.h"
#include "esp_gfx_font.h"
#include "esp_gfx_glyph_options.h"
#include "esp_gfx_rgb888.h"
#include "esp_non_copyable.h"
#include "esp_vt_cell.h"
#include "esp_vt_screen.h"
#include "esp_vt_style.h"

#include <array>
#include <span>


namespace esp::vt
{

class renderer : esp::non_copyable
{

public:

    renderer(esp::gfx::canvas &canvas, const esp::gfx::font &font,
             std::span<const esp::gfx::rgb888> palette);

    void draw(esp::vt::screen &screen, bool is_cursor_on);
    void invalidate();

private:

    void draw_cell(const esp::vt::cell &cell, int row, int column, bool is_cursor);
    void apply(const esp::gfx::rgb888 &pen, const esp::gfx::rgb888 &brush,
               esp::gfx::glyph_options options);

    /** @brief Холст видеорежима; живёт дольше рисования. */
    esp::gfx::canvas &_canvas;

    /** @brief Шрифт знакомест; его ширина и высота задают размер знакоместа на экране. */
    const esp::gfx::font &_font;

    /**
     * @brief Цвета ANSI 0–15, заменённые ближайшими цветами палитры видеорежима.
     *
     * Холсту передаются только цвета палитры, поэтому видеоконтроллеру не
     * приходится подбирать цвет самому.
     */
    std::array<esp::gfx::rgb888, 16> _colors;

    /** @brief Строка, где нарисован курсор; @c -1 — курсор не нарисован. */
    int _cursor_row;

    /** @brief Столбец, где нарисован курсор. */
    int _cursor_column;

    /** @brief Перерисовать при следующем draw() весь экран, а не только изменённые строки. */
    bool _is_invalid;

    /** @brief Известны ли цвета пера и кисти и параметры знаков, заданные холсту последними. */
    bool _is_state_known;

    /** @brief Цвет пера, заданный холсту последним. */
    esp::gfx::rgb888 _pen;

    /** @brief Цвет кисти, заданный холсту последним. */
    esp::gfx::rgb888 _brush;

    /** @brief Параметры знаков, заданные холсту последними. */
    esp::gfx::glyph_options _options;
};

}
