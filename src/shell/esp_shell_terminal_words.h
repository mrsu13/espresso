#pragma once

// ff.h есть и у FatFs из ESP-IDF, поэтому главный заголовок ff — по пути
// от src/3rdparty; остальные заголовки ff находятся по путям ff::ff.
#include <ff/src/ff.h>

#include <cstdint>


namespace esp::shell
{

class forth_shell;

class terminal_words
{

public:

    terminal_words() = delete;

    static ff_error_t register_words(ff_t *ff);

private:

    /** @brief Слова терминала: таблица для ff_register(). */
    static const ff_native_word_t WORDS[];

    template<std::uint32_t VALUE>
    static void constant(ff_t *ff);

    static void key(ff_t *ff);
    static void key_question(ff_t *ff);
    static void ekey(ff_t *ff);
    static void ekey_question(ff_t *ff);
    static void ekey_to_char(ff_t *ff);
    static void ekey_to_fkey(ff_t *ff);
    static void accept(ff_t *ff);
    static void page(ff_t *ff);
    static void at_xy(ff_t *ff);
    static void foreground(ff_t *ff);
    static void background(ff_t *ff);
    static void ms(ff_t *ff);
    static void beep(ff_t *ff);
    static void tone(ff_t *ff);
    static void video_mode(ff_t *ff);
    static void board_test(ff_t *ff);
    static void set_colour(ff_t *ff, int base, int bright_base);
    static esp::shell::forth_shell &shell(ff_t *ff);
};

}
