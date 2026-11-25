/**
 * @file esp_vt_screen.h
 * @brief Объявление модели экрана esp::vt::screen.
 */

/**
 * @file esp_vt_screen.cpp
 * @brief Модель экрана: исполнение подмножества VT220 над сеткой знакомест.
 */

#include "esp_vt_screen.h"

#include "esp_vt_config.h"
#include "esp_vt_controls.h"

#include <algorithm>
#include <cassert>


/**
 * @class esp::vt::screen
 * @brief Модель экрана терминала: исполняет то, что разобрал esp::vt::parser.
 *
 * Экран — две сетки знакомест одного размера, основная и альтернативная,
 * и показана одна из них. Он выполняет подмножество VT220 и альтернативный
 * экран xterm (раздел 6.2 документа об устройстве): движение курсора,
 * стирание, вставку и удаление знаков и строк, область прокрутки, вид
 * текста @c SGR, запоминание курсора. Остальные последовательности, а также
 * последовательности с промежуточными байтами и частные, кроме режимов DEC
 * с @c ?, экран пропускает.
 *
 * Экран ничего не рисует. Рисованию он сообщает, какие строки изменились
 * (changed_rows()) и на сколько строк весь экран прокрутился вверх
 * (pending_scroll()); нарисовав, оно вызывает clear_changes(). Звонок BEL
 * экран только запоминает: о нём узнают take_bell().
 *
 * Обе сетки выделяются в конструкторе, поэтому переход на альтернативный
 * экран не требует памяти и не может сорваться из-за её нехватки.
 */


namespace esp::vt::p
{

/**
 * @brief Возвращает цвет из 256 цветов xterm с номером 16–255 в виде 0xRRGGBB.
 *
 * Номера 16–231 — куб 6 × 6 × 6 с уровнями 0, 95, 135, 175, 215 и 255,
 * номера 232–255 — оттенки серого от 8 до 238 с шагом 10.
 *
 * @param number Номер цвета, 16–255.
 * @return Цвет.
 */
std::uint32_t xterm_color(int number)
{
    if (number >= 232)
    {
        const std::uint32_t level = static_cast<std::uint32_t>(8 + (number - 232) * 10);
        return (level << 16) | (level << 8) | level;
    }
    static constexpr std::array<std::uint32_t, 6> LEVELS{0, 95, 135, 175, 215, 255};
    const int cube = number - 16;
    return (LEVELS[static_cast<std::size_t>(cube / 36)] << 16)
        | (LEVELS[static_cast<std::size_t>(cube / 6 % 6)] << 8)
        | LEVELS[static_cast<std::size_t>(cube % 6)];
}

/**
 * @brief Находит цвет ANSI 0–15, ближайший к заданному.
 *
 * Расстояние — сумма квадратов разностей каналов.
 *
 * @param rgb Цвет 0xRRGGBB.
 * @return Номер ближайшего цвета из @c esp::vt::ANSI_COLORS.
 */
int nearest_color(std::uint32_t rgb)
{
    const auto distance = [rgb](std::uint32_t other)
    {
        int sum = 0;
        for (int shift = 0; shift <= 16; shift += 8)
        {
            const int delta =
                static_cast<int>((rgb >> shift) & 0xFF) - static_cast<int>((other >> shift) & 0xFF);
            sum += delta * delta;
        }
        return sum;
    };

    int best = 0;
    for (int color = 1; color < static_cast<int>(esp::vt::ANSI_COLORS.size()); ++color)
    {
        if (distance(esp::vt::ANSI_COLORS[static_cast<std::size_t>(color)])
            < distance(esp::vt::ANSI_COLORS[static_cast<std::size_t>(best)]))
            best = color;
    }
    return best;
}

}


// Открытые

/**
 * @brief Создаёт экран и выделяет обе его сетки.
 *
 * Экран сначала чистый, показана основная сетка, курсор в левом верхнем
 * углу.
 *
 * @param columns Число столбцов, больше 0.
 * @param rows Число строк, от 1 до @c esp::vt::MAX_ROWS.
 */
esp::vt::screen::screen(int columns, int rows)
    : _columns{columns}
    , _rows{rows}
    , _buffers{}
    , _is_alternate{false}
    , _is_cursor_visible{true}
    , _is_autowrap_on{true}
    , _changed_rows{0}
    , _pending_scroll{0}
    , _is_bell_pending{false}
{
    assert(columns > 0 && rows > 0 && rows <= esp::vt::MAX_ROWS);
    for (esp::vt::screen::buffer &buffer : _buffers)
        buffer.cells.resize(static_cast<std::size_t>(columns * rows));
    reset();
}

/**
 * @brief Сбрасывает экран, как @c ESC @c c (RIS).
 *
 * Обе сетки очищаются, показана основная, курсор виден и стоит в левом
 * верхнем углу, вид текста и режимы — по умолчанию. Весь экран отмечается
 * изменённым.
 */
void esp::vt::screen::reset()
{
    for (esp::vt::screen::buffer &buffer : _buffers)
        reset_buffer(buffer);
    _is_alternate = false;
    _is_cursor_visible = true;
    _is_autowrap_on = true;
    _pending_scroll = 0;
    mark_all();
}

/**
 * @brief Снимает отметки изменений: рисование показало экран.
 */
void esp::vt::screen::clear_changes()
{
    _changed_rows = 0;
    _pending_scroll = 0;
}

/**
 * @brief Сообщает, приходил ли BEL, и забывает о нём.
 * @return @c true, если с прошлого вызова пришёл хотя бы один BEL.
 */
bool esp::vt::screen::take_bell()
{
    const bool is_bell_pending = _is_bell_pending;
    _is_bell_pending = false;
    return is_bell_pending;
}

/**
 * @brief Выводит знак в позицию курсора текущим видом текста.
 *
 * Если курсор ждал переноса, он сначала переходит в начало следующей
 * строки, у нижнего края области прокрутки — с прокруткой. Знак в
 * последнем столбце оставляет курсор на месте и, если перенос включён,
 * откладывает перенос до следующего знака. Знак за пределами основной
 * плоскости Unicode заменяется знаком замены.
 *
 * @param code_point Код знака Unicode.
 */
void esp::vt::screen::print(std::uint32_t code_point)
{
    esp::vt::screen::buffer &buffer = active();
    if (buffer.is_wrap_pending && _is_autowrap_on)
    {
        buffer.cursor_column = 0;
        line_feed();
    }

    esp::vt::cell &target = *cell_at(buffer.cursor_row, buffer.cursor_column);
    target.code_point = static_cast<std::uint16_t>(
        code_point <= 0xFFFF ? code_point : esp::vt::REPLACEMENT_CHARACTER);
    target.style = buffer.style;
    mark_rows(buffer.cursor_row, buffer.cursor_row);

    if (buffer.cursor_column + 1 < _columns)
        ++buffer.cursor_column;
    else if (_is_autowrap_on)
        buffer.is_wrap_pending = true;
}

/**
 * @brief Выполняет управляющий байт C0.
 *
 * BEL запоминается для take_bell(), BS сдвигает курсор влево, HT — к
 * следующему столбцу, кратному @c esp::vt::TAB_WIDTH, LF, VT и FF переводят
 * строку (без возврата в начало), CR возвращает курсор в начало строки.
 * Остальные байты пропускаются.
 *
 * @param control Управляющий байт 0x00–0x1F.
 */
void esp::vt::screen::execute(std::uint8_t control)
{
    esp::vt::screen::buffer &buffer = active();
    switch (control)
    {
        case esp::vt::BEL:
            _is_bell_pending = true;
            break;

        case esp::vt::BS:
            buffer.is_wrap_pending = false;
            if (buffer.cursor_column > 0)
                --buffer.cursor_column;
            break;

        case esp::vt::HT:
            buffer.is_wrap_pending = false;
            buffer.cursor_column = std::min(
                (buffer.cursor_column / esp::vt::TAB_WIDTH + 1) * esp::vt::TAB_WIDTH, _columns - 1);
            break;

        case esp::vt::LF:
        case esp::vt::VT:
        case esp::vt::FF:
            line_feed();
            break;

        case esp::vt::CR:
            buffer.is_wrap_pending = false;
            buffer.cursor_column = 0;
            break;

        default:
            break;
    }
}

/**
 * @brief Выполняет последовательность CSI.
 *
 * Последовательность с промежуточными байтами пропускается, частная — тоже,
 * кроме @c CSI ?…h и @c CSI ?…l (режимы DEC). Все последовательности, кроме
 * @c SGR и @c CSI s, отменяют ожидание переноса: SGR между знаком в последнем
 * столбце и следующим знаком перенос не отменяет.
 *
 * @param command Разобранная последовательность.
 */
void esp::vt::screen::csi(const esp::vt::command &command)
{
    if (command.intermediate_count > 0)
        return;

    if (command.private_marker != 0)
    {
        if (command.private_marker == '?'
            && (command.final_byte == 'h' || command.final_byte == 'l'))
        {
            for (int i = 0; i < command.param_count; ++i)
                set_mode(command.param(i), command.final_byte == 'h');
        }
        return;
    }

    esp::vt::screen::buffer &buffer = active();
    if (command.final_byte != 'm' && command.final_byte != 's')
        buffer.is_wrap_pending = false;

    const int count = command.param_or_1(0);
    switch (command.final_byte)
    {
        case 'A': // CUU
            move_cursor(buffer.cursor_row - count, buffer.cursor_column);
            break;

        case 'B': // CUD
            move_cursor(buffer.cursor_row + count, buffer.cursor_column);
            break;

        case 'C': // CUF
            move_cursor(buffer.cursor_row, buffer.cursor_column + count);
            break;

        case 'D': // CUB
            move_cursor(buffer.cursor_row, buffer.cursor_column - count);
            break;

        case 'E': // CNL
            move_cursor(buffer.cursor_row + count, 0);
            break;

        case 'F': // CPL
            move_cursor(buffer.cursor_row - count, 0);
            break;

        case 'G': // CHA
            move_cursor(buffer.cursor_row, count - 1);
            break;

        case 'H': // CUP
        case 'f': // HVP
            move_cursor(count - 1, command.param_or_1(1) - 1);
            break;

        case 'd': // VPA
            move_cursor(count - 1, buffer.cursor_column);
            break;

        case 'J': // ED
            erase_display(command.param_or_0(0));
            break;

        case 'K': // EL
            erase_line(command.param_or_0(0));
            break;

        case 'L': // IL
            insert_lines(count);
            break;

        case 'M': // DL
            delete_lines(count);
            break;

        case '@': // ICH
            insert_characters(count);
            break;

        case 'P': // DCH
            delete_characters(count);
            break;

        case 'X': // ECH
            erase(buffer.cursor_row, buffer.cursor_column, buffer.cursor_column + count - 1);
            break;

        case 'S': // SU
            scroll_up(buffer.scroll_top, buffer.scroll_bottom, count);
            break;

        case 'T': // SD
            scroll_down(buffer.scroll_top, buffer.scroll_bottom, count);
            break;

        case 'r': // DECSTBM
            set_scroll_region(command);
            break;

        case 's': // SCOSC
            save_cursor();
            break;

        case 'u': // SCORC
            restore_cursor();
            break;

        case 'm': // SGR
            set_style(command);
            break;

        default:
            break;
    }
}

/**
 * @brief Выполняет последовательность ESC.
 *
 * Понимаются @c ESC @c 7 и @c ESC @c 8 (DECSC и DECRC), @c ESC @c D (IND),
 * @c ESC @c E (NEL), @c ESC @c M (RI) и @c ESC @c c (RIS). Последовательности
 * с промежуточными байтами — например, выбор набора знаков @c ESC @c ( @c B —
 * пропускаются.
 *
 * @param command Разобранная последовательность.
 */
void esp::vt::screen::esc(const esp::vt::command &command)
{
    if (command.intermediate_count > 0)
        return;

    switch (command.final_byte)
    {
        case '7': // DECSC
            save_cursor();
            break;

        case '8': // DECRC
            restore_cursor();
            break;

        case 'D': // IND
            line_feed();
            break;

        case 'E': // NEL
            active().cursor_column = 0;
            line_feed();
            break;

        case 'M': // RI
            reverse_index();
            break;

        case 'c': // RIS
            reset();
            break;

        default:
            break;
    }
}


// Закрытые

/**
 * @brief Возвращает пустое знакоместо для стирания.
 *
 * Пробел без признаков, с цветами текущего вида текста: стёртое место
 * окрашивается текущим фоном, как в xterm.
 *
 * @return Пустое знакоместо.
 */
esp::vt::cell esp::vt::screen::blank() const
{
    esp::vt::cell empty{};
    empty.style.foreground = active().style.foreground;
    empty.style.background = active().style.background;
    return empty;
}

/**
 * @brief Очищает сетку и возвращает её курсор, вид текста и область прокрутки к исходным.
 * @param buffer Сетка.
 */
void esp::vt::screen::reset_buffer(esp::vt::screen::buffer &buffer)
{
    std::fill(buffer.cells.begin(), buffer.cells.end(), esp::vt::cell{});
    buffer.cursor_row = 0;
    buffer.cursor_column = 0;
    buffer.style = esp::vt::style{};
    buffer.is_wrap_pending = false;
    buffer.scroll_top = 0;
    buffer.scroll_bottom = _rows - 1;
    buffer.has_saved = false;
}

/**
 * @brief Ставит курсор, удерживая его в пределах экрана, и отменяет ожидание переноса.
 * @param row Строка, с 0; вне экрана — ближайшая крайняя.
 * @param column Столбец, с 0; вне экрана — ближайший крайний.
 */
void esp::vt::screen::move_cursor(int row, int column)
{
    esp::vt::screen::buffer &buffer = active();
    buffer.cursor_row = std::clamp(row, 0, _rows - 1);
    buffer.cursor_column = std::clamp(column, 0, _columns - 1);
    buffer.is_wrap_pending = false;
}

/**
 * @brief Переводит курсор на строку вниз.
 *
 * На нижней строке области прокрутки область прокручивается вверх. Ниже
 * области на последней строке экрана курсор остаётся на месте.
 */
void esp::vt::screen::line_feed()
{
    esp::vt::screen::buffer &buffer = active();
    buffer.is_wrap_pending = false;
    if (buffer.cursor_row == buffer.scroll_bottom)
        scroll_up(buffer.scroll_top, buffer.scroll_bottom, 1);
    else if (buffer.cursor_row < _rows - 1)
        ++buffer.cursor_row;
}

/**
 * @brief Переводит курсор на строку вверх.
 *
 * На верхней строке области прокрутки область прокручивается вниз. Выше
 * области на первой строке экрана курсор остаётся на месте.
 */
void esp::vt::screen::reverse_index()
{
    esp::vt::screen::buffer &buffer = active();
    buffer.is_wrap_pending = false;
    if (buffer.cursor_row == buffer.scroll_top)
        scroll_down(buffer.scroll_top, buffer.scroll_bottom, 1);
    else if (buffer.cursor_row > 0)
        --buffer.cursor_row;
}

/**
 * @brief Прокручивает строки @p top–@p bottom вверх: верхние уходят, снизу появляются пустые.
 *
 * Прокрутка всего экрана учитывается в @c _pending_scroll, а отметки
 * изменённых строк сдвигаются вместе со строками; прокрутка части экрана
 * отмечает её строки изменёнными.
 *
 * @param top Верхняя строка, с 0.
 * @param bottom Нижняя строка, включительно.
 * @param count На сколько строк; больше высоты — очищается вся часть.
 */
void esp::vt::screen::scroll_up(int top, int bottom, int count)
{
    count = std::min(count, bottom - top + 1);
    if (count <= 0)
        return;

    esp::vt::cell *const first = cell_at(top, 0);
    esp::vt::cell *const end = cell_at(bottom + 1, 0);
    std::copy(first + count * _columns, end, first);
    std::fill(end - count * _columns, end, blank());

    if (top == 0 && bottom == _rows - 1)
    {
        _pending_scroll = std::min(_pending_scroll + count, _rows);
        _changed_rows = count < 64 ? _changed_rows >> count : 0;
        mark_rows(bottom - count + 1, bottom);
    }
    else
        mark_rows(top, bottom);
}

/**
 * @brief Прокручивает строки @p top–@p bottom вниз: нижние уходят, сверху появляются пустые.
 * @param top Верхняя строка, с 0.
 * @param bottom Нижняя строка, включительно.
 * @param count На сколько строк; больше высоты — очищается вся часть.
 */
void esp::vt::screen::scroll_down(int top, int bottom, int count)
{
    count = std::min(count, bottom - top + 1);
    if (count <= 0)
        return;

    esp::vt::cell *const first = cell_at(top, 0);
    esp::vt::cell *const end = cell_at(bottom + 1, 0);
    std::copy_backward(first, end - count * _columns, end);
    std::fill(first, first + count * _columns, blank());
    mark_rows(top, bottom);
}

/**
 * @brief Стирает знакоместа строки с @p first по @p last включительно.
 * @param row Строка, с 0.
 * @param first Первый столбец; меньше 0 — с начала строки.
 * @param last Последний столбец; за краем — до конца строки.
 */
void esp::vt::screen::erase(int row, int first, int last)
{
    first = std::max(first, 0);
    last = std::min(last, _columns - 1);
    if (first > last)
        return;

    std::fill(cell_at(row, first), cell_at(row, last) + 1, blank());
    mark_rows(row, row);
}

/**
 * @brief Стирает часть экрана, как @c CSI J (ED).
 * @param mode 0 — от курсора до конца экрана, 1 — от начала экрана до курсора,
 *             2 — весь экран; 3 (история экрана) и другие значения ничего не делают.
 */
void esp::vt::screen::erase_display(int mode)
{
    const esp::vt::screen::buffer &buffer = active();
    const int row = buffer.cursor_row;
    const int column = buffer.cursor_column;
    switch (mode)
    {
        case 0:
            erase(row, column, _columns - 1);
            for (int r = row + 1; r < _rows; ++r)
                erase(r, 0, _columns - 1);
            break;

        case 1:
            for (int r = 0; r < row; ++r)
                erase(r, 0, _columns - 1);
            erase(row, 0, column);
            break;

        case 2:
            for (int r = 0; r < _rows; ++r)
                erase(r, 0, _columns - 1);
            break;

        default:
            break;
    }
}

/**
 * @brief Стирает часть строки курсора, как @c CSI K (EL).
 * @param mode 0 — от курсора до конца строки, 1 — от начала строки до курсора,
 *             2 — всю строку; другие значения ничего не делают.
 */
void esp::vt::screen::erase_line(int mode)
{
    const esp::vt::screen::buffer &buffer = active();
    switch (mode)
    {
        case 0:
            erase(buffer.cursor_row, buffer.cursor_column, _columns - 1);
            break;

        case 1:
            erase(buffer.cursor_row, 0, buffer.cursor_column);
            break;

        case 2:
            erase(buffer.cursor_row, 0, _columns - 1);
            break;

        default:
            break;
    }
}

/**
 * @brief Вставляет пустые знакоместа в позицию курсора, сдвигая остаток строки вправо (ICH).
 *
 * Знаки, ушедшие за правый край, теряются.
 *
 * @param count Сколько знакомест вставить.
 */
void esp::vt::screen::insert_characters(int count)
{
    esp::vt::screen::buffer &buffer = active();
    const int column = buffer.cursor_column;
    count = std::min(count, _columns - column);

    esp::vt::cell *const first = cell_at(buffer.cursor_row, column);
    esp::vt::cell *const end = cell_at(buffer.cursor_row, 0) + _columns;
    std::copy_backward(first, end - count, end);
    std::fill(first, first + count, blank());
    mark_rows(buffer.cursor_row, buffer.cursor_row);
}

/**
 * @brief Удаляет знакоместа в позиции курсора, сдвигая остаток строки влево (DCH).
 *
 * Справа появляются пустые знакоместа.
 *
 * @param count Сколько знакомест удалить.
 */
void esp::vt::screen::delete_characters(int count)
{
    esp::vt::screen::buffer &buffer = active();
    const int column = buffer.cursor_column;
    count = std::min(count, _columns - column);

    esp::vt::cell *const first = cell_at(buffer.cursor_row, column);
    esp::vt::cell *const end = cell_at(buffer.cursor_row, 0) + _columns;
    std::copy(first + count, end, first);
    std::fill(end - count, end, blank());
    mark_rows(buffer.cursor_row, buffer.cursor_row);
}

/**
 * @brief Вставляет пустые строки в строку курсора, сдвигая строки ниже вниз (IL).
 *
 * Действует только внутри области прокрутки: строки, ушедшие за её нижний
 * край, теряются.
 *
 * @param count Сколько строк вставить.
 */
void esp::vt::screen::insert_lines(int count)
{
    const esp::vt::screen::buffer &buffer = active();
    if (buffer.cursor_row < buffer.scroll_top || buffer.cursor_row > buffer.scroll_bottom)
        return;
    scroll_down(buffer.cursor_row, buffer.scroll_bottom, count);
}

/**
 * @brief Удаляет строки начиная со строки курсора, сдвигая строки ниже вверх (DL).
 *
 * Действует только внутри области прокрутки: у её нижнего края появляются
 * пустые строки.
 *
 * @param count Сколько строк удалить.
 */
void esp::vt::screen::delete_lines(int count)
{
    const esp::vt::screen::buffer &buffer = active();
    if (buffer.cursor_row < buffer.scroll_top || buffer.cursor_row > buffer.scroll_bottom)
        return;
    scroll_up(buffer.cursor_row, buffer.scroll_bottom, count);
}

/**
 * @brief Задаёт область прокрутки, как @c CSI t;b r (DECSTBM), и ставит курсор в левый верхний угол.
 *
 * Пропущенная верхняя строка — первая, пропущенная нижняя — последняя.
 * Область меньше двух строк не принимается.
 *
 * @param command Последовательность с номерами верхней и нижней строк, с 1.
 */
void esp::vt::screen::set_scroll_region(const esp::vt::command &command)
{
    const int top = command.param_or_1(0) - 1;
    const int bottom = command.param(1) > 0 ? std::min(command.param(1), _rows) - 1 : _rows - 1;
    if (top >= bottom)
        return;

    esp::vt::screen::buffer &buffer = active();
    buffer.scroll_top = top;
    buffer.scroll_bottom = bottom;
    move_cursor(0, 0);
}

/**
 * @brief Запоминает курсор, вид текста и ожидание переноса (DECSC).
 */
void esp::vt::screen::save_cursor()
{
    esp::vt::screen::buffer &buffer = active();
    buffer.saved.row = buffer.cursor_row;
    buffer.saved.column = buffer.cursor_column;
    buffer.saved.style = buffer.style;
    buffer.saved.is_wrap_pending = buffer.is_wrap_pending;
    buffer.has_saved = true;
}

/**
 * @brief Восстанавливает запомненные курсор, вид текста и ожидание переноса (DECRC).
 *
 * Если курсор не запоминался, ничего не меняется.
 */
void esp::vt::screen::restore_cursor()
{
    esp::vt::screen::buffer &buffer = active();
    if (!buffer.has_saved)
        return;
    buffer.cursor_row = std::clamp(buffer.saved.row, 0, _rows - 1);
    buffer.cursor_column = std::clamp(buffer.saved.column, 0, _columns - 1);
    buffer.style = buffer.saved.style;
    buffer.is_wrap_pending = buffer.saved.is_wrap_pending;
}

/**
 * @brief Включает или выключает режим DEC (@c CSI ?n h и @c CSI ?n l).
 *
 * Понимаются режимы 7 (перенос в конце строки), 25 (видимость курсора) и
 * 1049 (альтернативный экран); остальные пропускаются.
 *
 * @param mode Номер режима; @c -1 (пропущен) ничего не делает.
 * @param is_set @c true — включить, @c false — выключить.
 */
void esp::vt::screen::set_mode(int mode, bool is_set)
{
    switch (mode)
    {
        case 7:
            _is_autowrap_on = is_set;
            if (!is_set)
                active().is_wrap_pending = false;
            break;

        case 25:
            _is_cursor_visible = is_set;
            break;

        case 1049:
            set_alternate(is_set);
            break;

        default:
            break;
    }
}

/**
 * @brief Переходит на альтернативный экран или возвращается с него (@c CSI ?1049h и @c CSI ?1049l).
 *
 * При переходе курсор основной сетки запоминается, как DECSC, а
 * альтернативная сетка очищается и получает курсор, вид текста основной и
 * область прокрутки на весь экран. При возврате курсор основной сетки
 * восстанавливается, как DECRC. Повторный переход и возврат без перехода
 * ничего не делают. Весь экран отмечается изменённым.
 *
 * @param is_alternate @c true — перейти, @c false — вернуться.
 */
void esp::vt::screen::set_alternate(bool is_alternate)
{
    if (is_alternate == _is_alternate)
        return;

    if (is_alternate)
    {
        save_cursor();
        const esp::vt::screen::buffer &main = _buffers[0];
        esp::vt::screen::buffer &alternate = _buffers[1];
        alternate.cursor_row = main.cursor_row;
        alternate.cursor_column = main.cursor_column;
        alternate.style = main.style;
        alternate.is_wrap_pending = false;
        alternate.scroll_top = 0;
        alternate.scroll_bottom = _rows - 1;
        alternate.has_saved = false;
        _is_alternate = true;
        std::fill(alternate.cells.begin(), alternate.cells.end(), blank());
    }
    else
    {
        _is_alternate = false;
        restore_cursor();
    }

    _pending_scroll = 0;
    mark_all();
}

/**
 * @brief Меняет вид текста, как @c CSI …m (SGR).
 *
 * Понимаются 0 (сброс), 1–4, 7, 8 (жирный, тусклый, курсив, подчёркнутый,
 * инверсия, скрытый), 22–24, 27, 28 (их отмена), 30–37, 90–97, 38 и 39
 * (цвет текста), 40–47, 100–107, 48 и 49 (цвет фона). Цвета 256 и RGB
 * заменяются ближайшими из 16 цветов ANSI. Остальное пропускается.
 *
 * @param command Последовательность с номерами; без номеров — сброс.
 */
void esp::vt::screen::set_style(const esp::vt::command &command)
{
    esp::vt::style &style = active().style;
    if (command.param_count == 0)
    {
        style = esp::vt::style{};
        return;
    }

    for (int i = 0; i < command.param_count; ++i)
    {
        const int number = command.param_or_0(i);
        if (number >= 30 && number <= 37)
            style.foreground = static_cast<std::uint8_t>(number - 30);
        else if (number >= 90 && number <= 97)
            style.foreground = static_cast<std::uint8_t>(number - 90 + 8);
        else if (number >= 40 && number <= 47)
            style.background = static_cast<std::uint8_t>(number - 40);
        else if (number >= 100 && number <= 107)
            style.background = static_cast<std::uint8_t>(number - 100 + 8);
        else
        {
            switch (number)
            {
                case 0:
                    style = esp::vt::style{};
                    break;

                case 1:
                    style.bold = true;
                    break;

                case 2:
                    style.dim = true;
                    break;

                case 3:
                    style.italic = true;
                    break;

                case 4:
                    style.underlined = true;
                    break;

                case 7:
                    style.inverse = true;
                    break;

                case 8:
                    style.hidden = true;
                    break;

                case 22:
                    style.bold = false;
                    style.dim = false;
                    break;

                case 23:
                    style.italic = false;
                    break;

                case 24:
                    style.underlined = false;
                    break;

                case 27:
                    style.inverse = false;
                    break;

                case 28:
                    style.hidden = false;
                    break;

                case 38:
                case 48:
                {
                    const int color = extended_color(command, i);
                    if (color >= 0 && number == 38)
                        style.foreground = static_cast<std::uint8_t>(color);
                    else if (color >= 0)
                        style.background = static_cast<std::uint8_t>(color);
                    break;
                }

                case 39:
                    style.foreground = esp::vt::DEFAULT_FOREGROUND;
                    break;

                case 49:
                    style.background = esp::vt::DEFAULT_BACKGROUND;
                    break;

                default:
                    break;
            }
        }
    }
}

/**
 * @brief Разбирает цвет после @c 38 или @c 48: @c 5;n (из 256) или @c 2;r;g;b.
 *
 * Номера 0–15 берутся как есть, остальные цвета заменяются ближайшими из
 * 16 цветов ANSI.
 *
 * @param command Последовательность SGR.
 * @param index Номер параметра @c 38 или @c 48; после вызова — номер
 *              последнего параметра, принадлежащего цвету.
 * @return Номер цвета ANSI 0–15 или @c -1, если цвет записан неверно.
 */
int esp::vt::screen::extended_color(const esp::vt::command &command, int &index) const
{
    const int kind = command.param(index + 1);
    if (kind == 5)
    {
        const int number = command.param(index + 2);
        index += 2;
        if (number < 0 || number > 255)
            return -1;
        return number < 16 ? number : esp::vt::p::nearest_color(esp::vt::p::xterm_color(number));
    }
    if (kind == 2)
    {
        const int red = command.param(index + 2);
        const int green = command.param(index + 3);
        const int blue = command.param(index + 4);
        index += 4;
        if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
            return -1;
        return esp::vt::p::nearest_color((static_cast<std::uint32_t>(red) << 16)
                                         | (static_cast<std::uint32_t>(green) << 8)
                                         | static_cast<std::uint32_t>(blue));
    }
    ++index;
    return -1;
}

/**
 * @brief Отмечает строки @p first–@p last изменёнными.
 * @param first Первая строка, с 0.
 * @param last Последняя строка, включительно.
 */
void esp::vt::screen::mark_rows(int first, int last)
{
    for (int row = std::max(first, 0); row <= std::min(last, _rows - 1); ++row)
        _changed_rows |= std::uint64_t{1} << row;
}

/**
 * @brief Отмечает изменёнными все строки.
 */
void esp::vt::screen::mark_all()
{
    mark_rows(0, _rows - 1);
}
