#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_cell.h"
#include "esp_vt_command.h"
#include "esp_vt_interpreter.h"
#include "esp_vt_scrollback.h"
#include "esp_vt_style.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>


namespace esp::vt
{

class screen : public esp::vt::interpreter, esp::non_copyable
{

public:

    screen(int columns, int rows);

    void reset();

    inline int columns() const;
    inline int rows() const;
    inline const esp::vt::cell &cell(int row, int column) const;
    inline int cursor_row() const;
    inline int cursor_column() const;
    inline bool is_wrap_pending() const;
    inline bool is_cursor_visible() const;
    inline bool is_alternate() const;

    inline const esp::vt::scrollback &scrollback() const;
    inline int view_offset() const;
    void scroll_view(int count);
    void reset_view();
    esp::vt::cell view_cell(int row, int column) const;

    inline std::uint64_t changed_rows() const;
    inline int pending_scroll() const;
    void clear_changes();
    bool take_bell();

    void print(std::uint32_t code_point) override;
    void execute(std::uint8_t control) override;
    void csi(const esp::vt::command &command) override;
    void esc(const esp::vt::command &command) override;

private:

    /**
     * @brief Запомненное положение курсора (@c ESC @c 7, @c CSI @c s, вход на альтернативный экран).
     */
    struct saved_cursor
    {
        /** @brief Строка курсора, с 0. */
        int row = 0;

        /** @brief Столбец курсора, с 0. */
        int column = 0;

        /** @brief Вид текста, действовавший при запоминании. */
        esp::vt::style style{};

        /** @brief Ждал ли курсор переноса в последнем столбце. */
        bool is_wrap_pending = false;
    };

    /**
     * @brief Сетка знакомест с её курсором — основная или альтернативная.
     *
     * У каждой сетки свои курсор, вид текста, область прокрутки и
     * запомненный курсор, поэтому полноэкранная программа на альтернативном
     * экране не сбивает их у оболочки на основном.
     */
    struct buffer
    {
        /** @brief Знакоместа по строкам: строка @c r, столбец @c c — элемент @c r × столбцов + @c c. */
        std::vector<esp::vt::cell> cells;

        /** @brief Строка курсора, с 0. */
        int cursor_row = 0;

        /** @brief Столбец курсора, с 0. */
        int cursor_column = 0;

        /** @brief Текущий вид текста: с ним выводятся знаки, его фоном стирается экран. */
        esp::vt::style style{};

        /**
         * @brief Знак выведен в последний столбец, и курсор ждёт переноса.
         *
         * Перенос откладывается до следующего знака, как в VT220: CR после
         * знака в последнем столбце возвращает курсор в начало той же
         * строки, а не следующей.
         */
        bool is_wrap_pending = false;

        /** @brief Верхняя строка области прокрутки, с 0. */
        int scroll_top = 0;

        /** @brief Нижняя строка области прокрутки, с 0, включительно. */
        int scroll_bottom = 0;

        /** @brief Запомненный курсор; действителен, если @c has_saved. */
        esp::vt::screen::saved_cursor saved{};

        /** @brief Запоминался ли курсор: без этого его восстановление ничего не делает. */
        bool has_saved = false;
    };

    inline esp::vt::screen::buffer &active();
    inline const esp::vt::screen::buffer &active() const;
    inline std::size_t index(int row, int column) const;
    inline esp::vt::cell *cell_at(int row, int column);

    esp::vt::cell blank() const;
    void reset_buffer(esp::vt::screen::buffer &buffer);
    void move_cursor(int row, int column);
    void line_feed();
    void reverse_index();
    void scroll_up(int top, int bottom, int count);
    void scroll_down(int top, int bottom, int count);
    void erase(int row, int first, int last);
    void erase_display(int mode);
    void erase_line(int mode);
    void insert_characters(int count);
    void delete_characters(int count);
    void insert_lines(int count);
    void delete_lines(int count);
    void set_scroll_region(const esp::vt::command &command);
    void save_cursor();
    void restore_cursor();
    void set_mode(int mode, bool is_set);
    void set_alternate(bool is_alternate);
    void set_style(const esp::vt::command &command);
    int extended_color(const esp::vt::command &command, int &index) const;
    void mark_rows(int first, int last);
    void mark_all();

    /** @brief Число столбцов. */
    int _columns;

    /** @brief Число строк, не больше @c esp::vt::MAX_ROWS. */
    int _rows;

    /** @brief Сетки: [0] — основная, [1] — альтернативная; обе выделяются в конструкторе. */
    std::array<esp::vt::screen::buffer, 2> _buffers;

    /**
     * @brief История экрана: строки, ушедшие за верхний край основного экрана.
     *
     * Строка попадает сюда, когда перевод строки прокручивает основной экран
     * целиком; память выделяется в конструкторе.
     */
    esp::vt::scrollback _scrollback;

    /**
     * @brief На сколько строк показ сдвинут вверх, в историю; 0 — показаны текущие строки.
     *
     * Сдвигает scroll_view(), а любой вывод возвращает в 0. На
     * альтернативном экране всегда 0.
     */
    int _view_offset;

    /** @brief Показан ли альтернативный экран (@c CSI ?1049h). */
    bool _is_alternate;

    /** @brief Виден ли курсор (@c CSI ?25h и @c CSI ?25l); общий для обеих сеток. */
    bool _is_cursor_visible;

    /** @brief Включён ли перенос в конце строки (@c CSI ?7h и @c CSI ?7l); общий для обеих сеток. */
    bool _is_autowrap_on;

    /**
     * @brief Изменённые строки показанной сетки: разряд @c r — строка @c r.
     *
     * Рисование перерисовывает отмеченные строки и вызывает clear_changes().
     */
    std::uint64_t _changed_rows;

    /**
     * @brief На сколько строк весь экран прокрутился вверх после clear_changes().
     *
     * Рисование сдвигает изображение на столько же строк — это быстрее, чем
     * рисовать строки заново, — а затем перерисовывает отмеченные в
     * @c _changed_rows. Отметки при прокрутке сдвигаются вместе со строками.
     * Прокрутка части экрана сдвигом не учитывается: её строки просто
     * отмечаются изменёнными.
     */
    int _pending_scroll;

    /** @brief Пришёл ли BEL после последнего take_bell(). */
    bool _is_bell_pending;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает число столбцов.
 * @return Ширина экрана в знакоместах.
 */
int esp::vt::screen::columns() const
{
    return _columns;
}

/**
 * @brief Возвращает число строк.
 * @return Высота экрана в знакоместах.
 */
int esp::vt::screen::rows() const
{
    return _rows;
}

/**
 * @brief Возвращает знакоместо показанной сетки.
 * @param row Строка, с 0, меньше rows().
 * @param column Столбец, с 0, меньше columns().
 * @return Знакоместо; ссылка действительна до следующего изменения экрана.
 */
const esp::vt::cell &esp::vt::screen::cell(int row, int column) const
{
    return active().cells[index(row, column)];
}

/**
 * @brief Возвращает строку курсора показанной сетки.
 * @return Строка, с 0.
 */
int esp::vt::screen::cursor_row() const
{
    return active().cursor_row;
}

/**
 * @brief Возвращает столбец курсора показанной сетки.
 *
 * Когда курсор ждёт переноса, это последний столбец.
 *
 * @return Столбец, с 0.
 */
int esp::vt::screen::cursor_column() const
{
    return active().cursor_column;
}

/**
 * @brief Сообщает, ждёт ли курсор показанной сетки переноса.
 *
 * Так бывает после знака в последнем столбце: следующий знак встанет в
 * начало следующей строки, а не в этот столбец.
 *
 * @return @c true, если перенос отложен до следующего знака.
 */
bool esp::vt::screen::is_wrap_pending() const
{
    return active().is_wrap_pending;
}

/**
 * @brief Сообщает, виден ли курсор.
 * @return @c false после @c CSI ?25l, @c true после @c CSI ?25h и сначала.
 */
bool esp::vt::screen::is_cursor_visible() const
{
    return _is_cursor_visible;
}

/**
 * @brief Сообщает, показан ли альтернативный экран.
 * @return @c true между @c CSI ?1049h и @c CSI ?1049l.
 */
bool esp::vt::screen::is_alternate() const
{
    return _is_alternate;
}

/**
 * @brief Возвращает историю экрана.
 * @return История: строки, ушедшие за верхний край основного экрана.
 */
const esp::vt::scrollback &esp::vt::screen::scrollback() const
{
    return _scrollback;
}

/**
 * @brief Возвращает, на сколько строк показ сдвинут в историю.
 * @return 0 — показаны текущие строки; больше — столько строк истории над ними.
 */
int esp::vt::screen::view_offset() const
{
    return _view_offset;
}

/**
 * @brief Возвращает отметки изменённых строк.
 * @return Разряд @c r установлен, если строку @c r нужно перерисовать.
 */
std::uint64_t esp::vt::screen::changed_rows() const
{
    return _changed_rows;
}

/**
 * @brief Возвращает, на сколько строк весь экран прокрутился вверх с последнего clear_changes().
 * @return Число строк, от 0 до rows().
 */
int esp::vt::screen::pending_scroll() const
{
    return _pending_scroll;
}


// Закрытые

/**
 * @brief Возвращает показанную сетку.
 * @return Основная или альтернативная сетка.
 */
esp::vt::screen::buffer &esp::vt::screen::active()
{
    return _buffers[_is_alternate ? 1 : 0];
}

/**
 * @brief Возвращает показанную сетку.
 * @return Основная или альтернативная сетка.
 */
const esp::vt::screen::buffer &esp::vt::screen::active() const
{
    return _buffers[_is_alternate ? 1 : 0];
}

/**
 * @brief Переводит строку и столбец в номер элемента сетки.
 * @param row Строка, с 0.
 * @param column Столбец, с 0.
 * @return Номер элемента в @c buffer::cells.
 */
std::size_t esp::vt::screen::index(int row, int column) const
{
    return static_cast<std::size_t>(row * _columns + column);
}

/**
 * @brief Возвращает указатель на знакоместо показанной сетки.
 *
 * Знакоместа строк идут подряд, поэтому указатель годится и как начало
 * отрезка для @c std::copy и @c std::fill; строка @c rows() со столбцом 0
 * даёт указатель за конец сетки.
 *
 * @param row Строка, с 0, не больше rows().
 * @param column Столбец, с 0.
 * @return Указатель на знакоместо.
 */
esp::vt::cell *esp::vt::screen::cell_at(int row, int column)
{
    return active().cells.data() + index(row, column);
}
