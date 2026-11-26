#pragma once

#include "esp_non_copyable.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>


namespace esp::vt
{

class line_history : esp::non_copyable
{

public:

    line_history(int byte_capacity, int line_capacity);

    bool add(std::string_view text);
    void clear();

    inline int size() const;
    std::string_view line(int age) const;
    int find(std::string_view text, int age) const;

private:

    /**
     * @brief Запись о строке истории: где лежат её байты и сколько их.
     */
    struct entry
    {
        /** @brief Номер первого байта строки в @c _text; байты строки идут подряд. */
        std::uint16_t start = 0;

        /** @brief Длина строки, байты. */
        std::uint16_t length = 0;
    };

    std::size_t reserve(std::size_t length);
    void drop_oldest();

    /**
     * @brief Байты строк.
     *
     * Каждая строка лежит подряд, чтобы line() могла вернуть её как есть.
     * Новая строка пишется за самой новой, а если там места до конца
     * буфера нет — с начала буфера; конец буфера тогда пустует, пока не
     * уйдут строки перед ним.
     */
    std::vector<char> _text;

    /** @brief Записи о строках по кругу: от самой старой (@c _first) к самой новой. */
    std::vector<esp::vt::line_history::entry> _entries;

    /** @brief Куда в @c _text пишется следующая строка: сразу за самой новой. */
    std::size_t _next;

    /** @brief Номер записи самой старой строки в @c _entries. */
    std::size_t _first;

    /** @brief Сколько строк в истории. */
    std::size_t _count;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает число строк в истории.
 * @return Сколько строк сейчас хранится.
 */
int esp::vt::line_history::size() const
{
    return static_cast<int>(_count);
}
