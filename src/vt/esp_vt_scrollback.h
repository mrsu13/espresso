#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_cell.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>


namespace esp::vt
{

class scrollback : esp::non_copyable
{

public:

    scrollback(int cell_capacity, int row_capacity);

    void push(std::span<const esp::vt::cell> cells);
    void clear();

    inline int rows() const;
    esp::vt::cell cell(int row, int column) const;

private:

    /**
     * @brief Запись о строке истории: где лежат её знакоместа и сколько их.
     */
    struct entry
    {
        /** @brief Номер первого знакоместа строки в @c _cells; дальше знакоместа идут по кругу. */
        std::uint16_t start = 0;

        /** @brief Число знакомест строки; пустые знакоместа в её конце не хранятся. */
        std::uint16_t length = 0;
    };

    void drop_oldest();

    /**
     * @brief Знакоместа строк, подряд по кругу.
     *
     * Занятая часть начинается с первого знакоместа самой старой строки и
     * кончается перед @c _next_cell; новые строки пишутся в свободную часть
     * за ней.
     */
    std::vector<esp::vt::cell> _cells;

    /** @brief Записи о строках по кругу: от самой старой (@c _first_row) к самой новой. */
    std::vector<esp::vt::scrollback::entry> _entries;

    /** @brief Куда в @c _cells пишется следующая строка. */
    std::size_t _next_cell;

    /** @brief Сколько знакомест в @c _cells занято строками. */
    std::size_t _used_cells;

    /** @brief Номер записи самой старой строки в @c _entries. */
    std::size_t _first_row;

    /** @brief Сколько строк в истории. */
    std::size_t _row_count;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает число строк в истории.
 * @return Сколько строк сейчас хранится, от 0 до заданного в конструкторе предела.
 */
int esp::vt::scrollback::rows() const
{
    return static_cast<int>(_row_count);
}
