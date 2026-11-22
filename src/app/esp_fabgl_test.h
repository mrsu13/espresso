#pragma once

#include "esp_gfx_canvas.h"
#include "esp_gfx_font.h"
#include "esp_ps2_utils.h"
#include "esp_sound_sound_generator.h"
#include "esp_vga_vga16_controller.h"

#include <string>


namespace esp
{

class fabgl_test
{

public:

    fabgl_test();

    void poll();
    void button_pressed();

private:

    int status_top() const;
    int paint_top() const;
    void draw_screen();
    void draw_line(int row, const char *text);
    void show_keyboard();
    void show_key(const esp::ps2::VirtualKeyItem &item);
    void show_input();
    void show_mouse();
    void show_button();
    void show_sound();
    void show_memory();
    void show_paint_color();
    void clear_paint_area();
    void paint(int x1, int y1, int x2, int y2);
    void on_key(const esp::ps2::VirtualKeyItem &item);
    void on_mouse(const esp::ps2::MouseStatus &status);
    void check_devices();
    void play(int frequency, int duration_ms);

    /** @brief Видеоконтроллер VGA на 16 цветов. */
    esp::vga::VGA16Controller _display;

    /** @brief Холст для рисования на экране видеоконтроллера _display. */
    esp::gfx::Canvas _canvas;

    /** @brief Шрифт всего текста на экране: Terminus 10 × 18 (таблицы ter_10x18). */
    esp::gfx::font _font;

    /** @brief Генератор звука: ЦАП на выводе GPIO25. */
    esp::sound::SoundGenerator _sound;

    /** @brief Строка, набранная на клавиатуре. */
    std::string _input;

    /** @brief Последнее полученное состояние мыши. */
    esp::ps2::MouseStatus _mouse;

    /** @brief Форма звуковой волны: номер в таблице WAVEFORMS. */
    int _waveform;

    /** @brief Цвет рисования мышью: номер цвета палитры, от 0 до 15. */
    int _paint_color;

    /** @brief Сколько раз нажата кнопка K1. */
    int _presses;

    /** @brief Была ли клавиатура найдена при последней проверке. */
    bool _keyboard_found;

    /** @brief Была ли мышь найдена при последней проверке. */
    bool _mouse_found;
};

}
