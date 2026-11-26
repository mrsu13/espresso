/**
 * @file esp_vt_renderer.h
 * @brief Объявление рисования экрана терминала esp::vt::renderer.
 */

/**
 * @file esp_vt_renderer.cpp
 * @brief Рисование экрана терминала на холсте: изменённые строки, прокрутка, курсор.
 */

#include "esp_vt_renderer.h"

#include "esp_vt_config.h"

#include <cstddef>
#include <cstdint>
#include <utility>


/**
 * @class esp::vt::renderer
 * @brief Рисует экран терминала esp::vt::screen на холсте знаками шрифта.
 *
 * Знакоместо рисуется одним знаком шрифта с заливкой фона, поэтому новый
 * знак закрывает прежний. Рисуются только строки, которые экран отметил
 * изменёнными. Если весь экран прокрутился вверх, изображение сдвигается
 * командой прокрутки холста — драйверы с палитрой меняют местами указатели
 * строк, это быстрее, чем рисовать строки заново, — и дорисовываются лишь
 * новые строки внизу.
 *
 * Цвета ANSI заменяются ближайшими цветами палитры видеорежима. Если у
 * знака цвет текста и цвет фона разные, а в палитре они совпали — в
 * 2-цветном режиме так бывает с большинством цветов, — текст рисуется
 * цветом по умолчанию или, на таком фоне, фоном по умолчанию, чтобы не
 * пропасть. Признаки (жирный, тусклый, курсив, подчёркнутый, скрытый)
 * рисует вывод знаков холста; инверсию — сам рисовальщик, меняя цвета
 * местами.
 *
 * Курсор — знакоместо, нарисованное в инверсии. Его рисуют, только когда
 * экран показывает текущие строки, а не историю.
 */


// Открытые

/**
 * @brief Готовит рисование и заменяет цвета ANSI цветами палитры.
 * @param canvas Холст видеорежима; должен жить дольше рисовальщика.
 * @param font Шрифт знакомест; должен жить дольше рисовальщика.
 * @param palette Цвета палитры видеорежима, хотя бы один.
 */
esp::vt::renderer::renderer(esp::gfx::canvas &canvas, const esp::gfx::font &font,
                            std::span<const esp::gfx::rgb888> palette)
    : _canvas{canvas}
    , _font{font}
    , _colors{}
    , _cursor_row{-1}
    , _cursor_column{0}
    , _is_invalid{true}
    , _is_state_known{false}
    , _pen{}
    , _brush{}
    , _options{}
{
    for (std::size_t ansi = 0; ansi < _colors.size(); ++ansi)
    {
        const std::uint32_t rgb = esp::vt::ANSI_COLORS[ansi];
        const int red = static_cast<int>((rgb >> 16) & 0xFF);
        const int green = static_cast<int>((rgb >> 8) & 0xFF);
        const int blue = static_cast<int>(rgb & 0xFF);

        // Ближайший цвет — с наименьшей суммой квадратов разностей каналов;
        // из равных — первый в палитре.
        int best_distance = -1;
        for (const esp::gfx::rgb888 &color : palette)
        {
            const int delta_red = red - color.r;
            const int delta_green = green - color.g;
            const int delta_blue = blue - color.b;
            const int distance =
                delta_red * delta_red + delta_green * delta_green + delta_blue * delta_blue;
            if (best_distance < 0 || distance < best_distance)
            {
                best_distance = distance;
                _colors[ansi] = color;
            }
        }
    }
}

/**
 * @brief Рисует изменения экрана и курсор.
 *
 * Сначала изображение сдвигается на столько строк, на сколько прокрутился
 * весь экран, затем стирается прежний курсор, рисуются изменённые строки и
 * новый курсор. Отметки изменений экрана снимаются.
 *
 * @param screen Экран.
 * @param is_cursor_on Фаза мигания: @c true — курсор показан, если он виден
 *                     и экран показывает текущие строки.
 */
void esp::vt::renderer::draw(esp::vt::screen &screen, bool is_cursor_on)
{
    const int rows = screen.rows();
    const int columns = screen.columns();
    const int width = _font.width();
    const int height = _font.height();

    std::uint64_t changed = screen.changed_rows();
    const int scroll = screen.pending_scroll();
    if (_is_invalid)
    {
        changed = rows >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << rows) - 1;
        _cursor_row = -1;
        _is_invalid = false;
    }
    else if (scroll > 0)
    {
        if (scroll < rows)
        {
            // Освободившиеся внизу строки холст заливает цветом кисти; их
            // всё равно рисовать заново.
            apply(_colors[esp::vt::DEFAULT_FOREGROUND], _colors[esp::vt::DEFAULT_BACKGROUND],
                  _options);
            _canvas.set_scrolling_region(0, 0, columns * width - 1, rows * height - 1);
            _canvas.scroll(0, -scroll * height);
        }
        // Нарисованный курсор уехал вверх вместе с изображением.
        _cursor_row = _cursor_row >= scroll ? _cursor_row - scroll : -1;
    }

    if (_cursor_row >= 0 && (changed & (std::uint64_t{1} << _cursor_row)) == 0)
        draw_cell(screen.view_cell(_cursor_row, _cursor_column), _cursor_row, _cursor_column,
                  false);
    _cursor_row = -1;

    for (int row = 0; row < rows; ++row)
    {
        if ((changed & (std::uint64_t{1} << row)) == 0)
            continue;
        for (int column = 0; column < columns; ++column)
            draw_cell(screen.view_cell(row, column), row, column, false);
    }

    if (is_cursor_on && screen.is_cursor_visible() && screen.view_offset() == 0)
    {
        _cursor_row = screen.cursor_row();
        _cursor_column = screen.cursor_column();
        draw_cell(screen.cell(_cursor_row, _cursor_column), _cursor_row, _cursor_column, true);
    }

    screen.clear_changes();
}

/**
 * @brief Велит перерисовать при следующем draw() весь экран.
 *
 * Нужно, когда изображение на холсте испорчено чем-то посторонним.
 */
void esp::vt::renderer::invalidate()
{
    _is_invalid = true;
}


// Закрытые

/**
 * @brief Рисует знакоместо.
 *
 * Цвета пера и кисти и параметры знаков меняются, только если они
 * отличаются от заданных последними: соседние знакоместа одного вида
 * ставят в очередь видеоконтроллера одни знаки.
 *
 * @param cell Знакоместо.
 * @param row Строка экрана.
 * @param column Столбец экрана.
 * @param is_cursor @c true — нарисовать как курсор, с обратной инверсией.
 */
void esp::vt::renderer::draw_cell(const esp::vt::cell &cell, int row, int column, bool is_cursor)
{
    const esp::vt::style &style = cell.style;
    esp::gfx::rgb888 foreground = _colors[style.foreground];
    esp::gfx::rgb888 background = _colors[style.background];
    if (foreground == background && style.foreground != style.background)
    {
        foreground = background == _colors[esp::vt::DEFAULT_BACKGROUND]
            ? _colors[esp::vt::DEFAULT_FOREGROUND]
            : _colors[esp::vt::DEFAULT_BACKGROUND];
    }
    if (style.inverse != is_cursor)
        std::swap(foreground, background);

    esp::gfx::glyph_options options{};
    options.fill_background = 1;
    options.bold = style.bold;
    options.reduce_luminosity = style.dim;
    options.italic = style.italic;
    options.underline = style.underlined;
    options.blank = style.hidden;
    apply(foreground, background, options);

    _font.draw_char(_canvas, column * _font.width(), row * _font.height(), cell.code_point);
}

/**
 * @brief Задаёт холсту цвета пера и кисти и параметры знаков — только то, что изменилось.
 * @param pen Цвет пера — цвет знака.
 * @param brush Цвет кисти — цвет фона знакоместа.
 * @param options Параметры знаков.
 */
void esp::vt::renderer::apply(const esp::gfx::rgb888 &pen, const esp::gfx::rgb888 &brush,
                              esp::gfx::glyph_options options)
{
    if (!_is_state_known || pen != _pen)
    {
        _canvas.set_pen_color(pen);
        _pen = pen;
    }
    if (!_is_state_known || brush != _brush)
    {
        _canvas.set_brush_color(brush);
        _brush = brush;
    }
    if (!_is_state_known || options.value != _options.value)
    {
        _canvas.set_glyph_options(options);
        _options = options;
    }
    _is_state_known = true;
}
