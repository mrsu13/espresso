/**
 * @file esp_vt_scrollback.h
 * @brief Объявление истории экрана esp::vt::scrollback.
 */

/**
 * @file esp_vt_scrollback.cpp
 * @brief История экрана: строки, ушедшие за верхний край, в кольцевом буфере.
 */

#include "esp_vt_scrollback.h"

#include <algorithm>
#include <cassert>


/**
 * @class esp::vt::scrollback
 * @brief История экрана: строки, ушедшие за верхний край основного экрана.
 *
 * Строки хранятся в кольцевом буфере постоянного размера, без пустых
 * знакомест с видом текста по умолчанию в конце: короткая строка занимает
 * мало места. Знакоместа строк лежат подряд по кругу в одном массиве, а
 * начало и длина каждой строки — в отдельном кольцевом списке. Когда места
 * для новой строки нет — не хватает знакомест или записей, — уходят самые
 * старые строки. Вся память выделяется в конструкторе, и добавление строки
 * ничего не выделяет.
 *
 * Строки нумеруются от самой новой: строка 0 ушла с экрана последней.
 */


// Открытые

/**
 * @brief Создаёт пустую историю и выделяет её память.
 * @param cell_capacity Сколько знакомест вмещает история, от 1 до 65536.
 * @param row_capacity Сколько строк она помнит, больше 0.
 */
esp::vt::scrollback::scrollback(int cell_capacity, int row_capacity)
    : _cells{}
    , _entries{}
    , _next_cell{0}
    , _used_cells{0}
    , _first_row{0}
    , _row_count{0}
{
    assert(cell_capacity > 0 && cell_capacity <= 65536 && row_capacity > 0);
    _cells.resize(static_cast<std::size_t>(cell_capacity));
    _entries.resize(static_cast<std::size_t>(row_capacity));
}

/**
 * @brief Добавляет строку как самую новую.
 *
 * Пустые знакоместа с видом текста по умолчанию в конце строки не
 * хранятся. Если места нет, сначала уходят самые старые строки; строка
 * длиннее всей истории обрезается до её размера.
 *
 * @param cells Знакоместа строки слева направо.
 */
void esp::vt::scrollback::push(std::span<const esp::vt::cell> cells)
{
    std::size_t length = cells.size();
    while (length > 0 && cells[length - 1] == esp::vt::cell{})
        --length;
    length = std::min(length, _cells.size());

    while (_row_count > 0
           && (_row_count == _entries.size() || _cells.size() - _used_cells < length))
        drop_oldest();

    esp::vt::scrollback::entry &entry = _entries[(_first_row + _row_count) % _entries.size()];
    entry.start = static_cast<std::uint16_t>(_next_cell);
    entry.length = static_cast<std::uint16_t>(length);
    for (std::size_t i = 0; i < length; ++i)
        _cells[(_next_cell + i) % _cells.size()] = cells[i];

    _next_cell = (_next_cell + length) % _cells.size();
    _used_cells += length;
    ++_row_count;
}

/**
 * @brief Забывает все строки.
 */
void esp::vt::scrollback::clear()
{
    _next_cell = 0;
    _used_cells = 0;
    _first_row = 0;
    _row_count = 0;
}

/**
 * @brief Возвращает знакоместо строки истории.
 * @param row Строка, от 0 (ушла с экрана последней) до rows() − 1.
 * @param column Столбец, с 0; за концом хранимой части строки — пустое знакоместо.
 * @return Знакоместо.
 */
esp::vt::cell esp::vt::scrollback::cell(int row, int column) const
{
    assert(row >= 0 && static_cast<std::size_t>(row) < _row_count);
    const esp::vt::scrollback::entry &entry =
        _entries[(_first_row + _row_count - 1 - static_cast<std::size_t>(row)) % _entries.size()];
    if (column < 0 || column >= entry.length)
        return esp::vt::cell{};
    return _cells[(entry.start + static_cast<std::size_t>(column)) % _cells.size()];
}


// Закрытые

/**
 * @brief Забывает самую старую строку и освобождает её знакоместа.
 */
void esp::vt::scrollback::drop_oldest()
{
    _used_cells -= _entries[_first_row].length;
    _first_row = (_first_row + 1) % _entries.size();
    --_row_count;
}
