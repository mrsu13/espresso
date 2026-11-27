#pragma once

#include "esp_non_copyable.h"
#include "esp_vt_config.h"
#include "esp_vt_key_event.h"
#include "esp_vt_line_history.h"
#include "esp_vt_writer.h"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>


namespace esp::vt
{

class line_edit : esp::non_copyable
{

public:

    /** @brief Чем кончилась обработка клавиши. */
    enum class result
    {
        /** @brief Строка ещё набирается. */
        editing,

        /** @brief Нажат Enter: строка готова, её возвращает line(). */
        accepted,

        /** @brief Нажат Ctrl+C: строка отменена. */
        cancelled,
    };

    line_edit(esp::vt::writer &output, const esp::vt::line_history &history);

    void set_words(std::span<const std::string_view> words);
    void start(std::string_view prompt, int columns, int column = 0);
    esp::vt::line_edit::result feed(esp::vt::key_event key);
    inline std::string_view line() const;

private:

    /** @brief Размер буфера вывода, байты: вывод уходит в @c _output кусками, а не по байту. */
    static constexpr std::size_t OUTPUT_CAPACITY = 512;

    /** @brief Размер приглашения поиска: постоянная часть и строка поиска, байты. */
    static constexpr std::size_t SEARCH_PROMPT_CAPACITY = esp::vt::MAX_LINE_BYTES + 32;

    esp::vt::line_edit::result edit(esp::vt::key_event key, bool was_tab);
    esp::vt::line_edit::result edit_search(esp::vt::key_event key);
    esp::vt::line_edit::result accept();
    esp::vt::line_edit::result cancel();
    void replace(std::size_t from, std::size_t to, std::string_view text);
    void kill(std::size_t from, std::size_t to);
    void move_to(std::size_t index);
    std::size_t word_left(std::size_t index) const;
    std::size_t word_right(std::size_t index) const;
    std::size_t word_start(std::size_t index) const;
    void set_line(std::string_view text, std::size_t cursor);
    void save_draft();
    void recall(int age);
    bool complete(bool was_tab);
    void list(std::span<std::string_view> matches);
    void start_search();
    void search(int age);
    void stop_search(bool is_kept);
    void update_search_prompt();
    std::string_view query() const;
    std::string_view shown_prompt() const;
    std::string_view shown_text() const;
    std::size_t shown_cursor() const;
    void redraw();
    void redraw_from(std::size_t index);
    int position(std::string_view text, std::size_t index) const;
    void move_cursor(int target);
    void put(std::string_view text);
    void write_csi(int count, char final_byte);
    void bell();
    void write(std::string_view bytes);
    void flush();

    /** @brief Куда выводить: терминал или, в проверках, разборщик с экраном. */
    esp::vt::writer &_output;

    /** @brief История строк для ↑, ↓ и поиска Ctrl+R; строки в неё добавляет вызывающий. */
    const esp::vt::line_history &_history;

    /** @brief Слова для дополнения по Tab. */
    std::span<const std::string_view> _words;

    /** @brief Приглашение; живёт, пока набирается строка. */
    std::string_view _prompt;

    /** @brief Ширина экрана в знакоместах: по ней длинная строка переносится. */
    int _columns;

    /** @brief Столбец первой строки экрана, с которого начинается приглашение. */
    int _origin;

    /** @brief Знакоместо, с которого начинается текст строки, — сразу за приглашением. */
    int _text_start;

    /** @brief Набираемая строка в UTF-8. */
    std::array<char, esp::vt::MAX_LINE_BYTES> _line;

    /** @brief Длина набираемой строки, байты. */
    std::size_t _length;

    /** @brief Курсор в строке, байты: начало знака или конец строки. */
    std::size_t _cursor;

    /** @brief Строка, которую набирали до перехода в историю: к ней возвращает ↓. */
    std::array<char, esp::vt::MAX_LINE_BYTES> _draft;

    /** @brief Длина @c _draft, байты. */
    std::size_t _draft_length;

    /** @brief Последний удалённый текст: его вставляет Ctrl+Y. */
    std::array<char, esp::vt::MAX_LINE_BYTES> _kill;

    /** @brief Длина @c _kill, байты. */
    std::size_t _kill_length;

    /** @brief Строка поиска по истории. */
    std::array<char, esp::vt::MAX_LINE_BYTES> _query;

    /** @brief Длина @c _query, байты. */
    std::size_t _query_length;

    /** @brief Приглашение поиска, как в bash: @c (reverse-i-search) и строка поиска в кавычках. */
    std::array<char, SEARCH_PROMPT_CAPACITY> _search_prompt;

    /** @brief Длина @c _search_prompt, байты. */
    std::size_t _search_prompt_length;

    /** @brief Какая строка истории показана: @c -1 — набираемая. */
    int _history_age;

    /** @brief Найденная поиском строка истории: @c -1 — пока ничего не найдено. */
    int _search_age;

    /** @brief Идёт ли поиск по истории (Ctrl+R). */
    bool _is_searching;

    /** @brief Не нашлось ли строки с нынешней строкой поиска. */
    bool _is_search_failed;

    /** @brief Был ли предыдущей клавишей Tab, который строку не изменил: следующий Tab выводит варианты. */
    bool _was_tab;

    /**
     * @brief Где на экране курсор: знакоместо, считая от начала первой строки экрана.
     *
     * Строка экрана — частное от деления на @c _columns, столбец — остаток.
     * По нему считаются движения курсора; строка начинается со знакоместа
     * @c _origin.
     */
    int _drawn_cursor;

    /** @brief Буфер вывода. */
    std::array<char, OUTPUT_CAPACITY> _out;

    /** @brief Сколько байтов в @c _out. */
    std::size_t _out_length;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает набранную строку.
 * @return Строка UTF-8; действительна до следующего вызова start() или feed().
 */
std::string_view esp::vt::line_edit::line() const
{
    return std::string_view{_line.data(), _length};
}
