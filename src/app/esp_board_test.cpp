/**
 * @file esp_board_test.h
 * @brief Объявление класса проверки платы esp::board_test.
 */

/**
 * @file esp_board_test.cpp
 * @brief Проверка платы TTGO VGA32: VGA, клавиатура и мышь PS/2,
 *        звук, шрифт Unicode.
 */

#include "esp_board_test.h"

#include "esp_gfx_color.h"
#include "esp_gfx_cursor_name.h"
#include "esp_gfx_rect.h"
#include "esp_ps2_keyboard.h"
#include "esp_ps2_keyboard_layouts.h"
#include "esp_ps2_mouse.h"
#include "esp_ps2_ps2_controller.h"
#include "esp_sound_noise_waveform_generator.h"
#include "esp_sound_sawtooth_waveform_generator.h"
#include "esp_sound_sine_waveform_generator.h"
#include "esp_sound_square_waveform_generator.h"
#include "esp_sound_triangle_waveform_generator.h"
#include "esp_vga_modelines.h"
#include "ter_10x18.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


/**
 * @class esp::board_test
 * @brief Проверка платы TTGO VGA32 v1.4.
 *
 * Конструктор включает вывод на VGA (640 × 480, 16 цветов), порты PS/2
 * (клавиатура на порту 0, мышь на порту 1) и звук (ЦАП, GPIO25), рисует
 * проверочный экран и подаёт два звука.
 *
 * На экране — рамка по краю изображения, заголовок, полосы всех 16 цветов
 * палитры, строки состояния, образцы русского текста и псевдографики и
 * поле для рисования мышью. Весь текст выводится растровым шрифтом
 * Unicode Terminus 10 × 18 (esp::gfx::font, таблицы из src/fonts); разметка
 * экрана рассчитывается по размеру знакоместа шрифта.
 *
 * Клавиатура: набранные символы появляются в строке ввода; Backspace
 * стирает последний, Enter выводит строку в консоль и очищает её, Esc
 * просто очищает. F1–F8 играют гамму до мажор (523–1047 Гц), F9 меняет
 * форму звуковой волны, F10 показывает на поле для рисования проверку
 * фигур и отсечения (draw_clipping_test()) и убирает её. F12 по кругу
 * переключает видеорежимы (esp::p::DISPLAY_MODES): 640 × 480 с 16, 8, 4 и
 * 2 цветами, 800 × 600 с 4 и 2 цветами, 400 × 300 и 320 × 240 с 64
 * цветами; на маленьком экране видны не все строки, а поля для рисования
 * нет.
 *
 * Мышь: левая кнопка рисует, правая очищает поле, средняя подаёт звук,
 * колесо меняет цвет рисования.
 *
 * Кнопка K1 (button_pressed()) подаёт звук и заново ищет клавиатуру и
 * мышь, если их не было. События клавиатуры и мыши обрабатывает poll(),
 * которую приложение вызывает каждые несколько миллисекунд.
 */


namespace esp::p
{

/** @brief Тег журнала модуля. */
const char *const TAG = "test";

/** @brief Видеоконтроллер, которым выводится видеорежим проверки. */
enum class vga_driver
{
    /** @brief esp::vga::vga16_controller: 16 цветов, 4 разряда на точку. */
    vga16,

    /** @brief esp::vga::vga8_controller: 8 цветов, 3 разряда на точку. */
    vga8,

    /** @brief esp::vga::vga4_controller: 4 цвета, 2 разряда на точку. */
    vga4,

    /** @brief esp::vga::vga2_controller: 2 цвета, разряд на точку. */
    vga2,

    /** @brief esp::vga::vga_controller: 64 цвета, байт на точку. */
    vga64,
};

/** @brief Видеорежим проверки: видеоконтроллер и развёртка. */
struct display_mode
{
    /** @brief Видеоконтроллер. */
    vga_driver driver;

    /** @brief Модлайн развёртки (esp_vga_modelines.h). */
    const char *modeline;
};

/**
 * @brief Видеорежимы, которые по кругу перебирает F12; первый включается при запуске.
 *
 * Буфер кадра при 640 × 480 занимает 150 КБ при 16 цветах, 113 КБ при 8,
 * 75 КБ при 4 и 38 КБ при 2; при 800 × 600 — 117 КБ при 4 цветах и 59 КБ
 * при 2; при 64 цветах (байт на точку) — 117 КБ при 400 × 300 и 75 КБ при
 * 320 × 240. Если памяти не хватает, видеоконтроллер уменьшает высоту
 * области вывода.
 */
constexpr display_mode DISPLAY_MODES[] = {
    {vga_driver::vga16, esp::vga::VGA_640X480_60HZ},
    {vga_driver::vga8, esp::vga::VGA_640X480_60HZ},
    {vga_driver::vga4, esp::vga::VGA_640X480_60HZ},
    {vga_driver::vga2, esp::vga::VGA_640X480_60HZ},
    {vga_driver::vga4, esp::vga::SVGA_800X600_60HZ},
    {vga_driver::vga2, esp::vga::SVGA_800X600_60HZ},
    {vga_driver::vga64, esp::vga::VGA_400X300_60HZ},
    {vga_driver::vga64, esp::vga::QVGA_320X240_60HZ},
};

/** @brief Поле между краем изображения и содержимым экрана, точки. */
constexpr int MARGIN = 8;

/** @brief Верхний край текста заголовка. */
constexpr int TITLE_TOP = 2;

/** @brief Высота цветных полос, точки. */
constexpr int BAR_HEIGHT = 24;

/**
 * @brief Число именованных цветов esp::gfx::color.
 *
 * Столько цветных полос рисует проверочный экран и столько цветов
 * перебирает колесо мыши; при меньшей палитре видеоконтроллер выводит
 * вместо каждого ближайший цвет своей палитры.
 */
constexpr int COLORS = 16;

/**
 * @brief Образцы текста: русский текст, алфавит и псевдографика.
 *
 * Строки не длиннее 62 знаков: столько помещается в строку экрана при
 * ширине знакоместа 10 точек. Три последние строки стоят друг под другом:
 * по ним видно, стыкуются ли линии рамок соседних знакомест.
 */
const char *const SAMPLES[] = {
    "Съешь же ещё этих мягких французских булок, да выпей чаю. №1",
    "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ",
    "абвгдеёжзийклмнопрстуфхцчшщъыьэюя",
    "┌──┬──┐ ╔══╦══╗ ╒══╤══╕ ┏━━┳━━┓ ╭───╮ ░▒▓█ ←↑→↓ ±×÷≠≤≥",
    "│Да│Не│ ║Ок║Ой║ │ 1│ 2│ ┃Ab┃Бв┃ │ Ж │ ▀▄▌▐ «ёлка» — …",
    "└──┴──┘ ╚══╩══╝ ╘══╧══╛ ┗━━┻━━┛ ╰───╯ ▖▗▘▝ ∞ √ ∑ π λ Ω",
};

/** @brief Строки текста под цветными полосами сверху вниз. */
enum row
{
    /** @brief Клавиатура и её раскладка. */
    row_keyboard,

    /** @brief Последняя нажатая или отпущенная клавиша. */
    row_key,

    /** @brief Набранная строка. */
    row_input,

    /** @brief Положение и кнопки мыши. */
    row_mouse,

    /** @brief Кнопка K1. */
    row_button,

    /** @brief Форма звуковой волны. */
    row_sound,

    /** @brief Свободная память. */
    row_memory,

    /** @brief Первая строка образцов текста SAMPLES. */
    row_samples,

    /** @brief Подсказка о мыши и образец цвета рисования. */
    row_help = row_samples + static_cast<int>(std::size(SAMPLES)),

    /** @brief Число строк. */
    rows
};

/** @brief Толщина линии при рисовании мышью, точки. */
constexpr int PAINT_PEN_WIDTH = 3;

/** @brief Наименьшая высота поля для рисования без рамки, точки; на более низком экране поля нет. */
constexpr int PAINT_AREA_MIN_HEIGHT = 32;

/**
 * @brief Промежуток между рамкой поля для рисования и рамкой окна проверки
 *        отсечения слева и справа, точки.
 *
 * В промежутках рисуются фигуры, которые целиком вне окна и потому не
 * должны появиться (см. esp::board_test::draw_clipping_test()).
 */
constexpr int TEST_GAP_X = 24;

/** @brief То же сверху и снизу, точки. */
constexpr int TEST_GAP_Y = 12;

/**
 * @brief Наименьшая ширина окна проверки отсечения, точки.
 *
 * Фигуры проверки стоят на заданных расстояниях от краёв окна; в более
 * узком окне они налезли бы друг на друга (см.
 * esp::board_test::draw_clipping_test()).
 */
constexpr int TEST_WINDOW_MIN_WIDTH = 548;

/** @brief Наименьшая высота окна проверки отсечения, точки. */
constexpr int TEST_WINDOW_MIN_HEIGHT = 110;

/** @brief Наибольшая длина набираемой строки, символы. */
constexpr std::size_t INPUT_MAX = 60;

/** @brief Громкость звуков: от 0 до 127. */
constexpr int VOLUME = 64;

/** @brief Частоты нот для клавиш F1–F8: гамма до мажор от C5 до C6, Гц. */
constexpr int NOTES[] = {523, 587, 659, 698, 784, 880, 988, 1047};

/** @brief Длительность ноты, мс. */
constexpr int NOTE_MS = 250;

/** @brief Названия форм звуковой волны в порядке номеров (см. esp::board_test::play()). */
const char *const WAVEFORMS[] = {"sine", "square", "triangle", "sawtooth", "noise"};

static_assert(std::size(WAVEFORMS) == 5, "WAVEFORMS must match esp::board_test::play()");

/** @brief Указатель мыши. */
constexpr esp::gfx::cursor_name CURSOR = esp::gfx::cursor_name::pointer_shadowed;

/**
 * @brief Возвращает цвет палитры по номеру.
 * @param index Номер цвета, от 0 до 15.
 * @return Цвет палитры.
 */
esp::gfx::color color(int index)
{
    return static_cast<esp::gfx::color>(index);
}

}


// Открытые

/**
 * @brief Включает VGA, PS/2 и звук и рисует проверочный экран.
 *
 * Выводы — по умолчанию модулей esp::vga, esp::ps2 и esp::sound; они
 * совпадают с разводкой платы TTGO VGA32:
 * красный 21 и 22, зелёный 18 и 19, синий 4 и 5, HSync 23, VSync 15;
 * клавиатура CLK 33, DATA 32; мышь CLK 26, DATA 27; звук — ЦАП на GPIO25.
 */
esp::board_test::board_test()
    : _display{nullptr}
    , _font{ter_10x18::WIDTH,  ter_10x18::HEIGHT, ter_10x18::CODEPOINTS,
            ter_10x18::GLYPHS, ter_10x18::COUNT,  ter_10x18::FALLBACK}
    , _waveform{0}
    , _paint_color{static_cast<int>(esp::gfx::color::bright_yellow)}
    , _presses{0}
    , _keyboard_found{false}
    , _mouse_found{false}
    , _clipping_test_shown{false}
    , _video_mode{0}
{
    set_video_mode(0);

    // Контроллер PS/2 ищет устройства только здесь и при их сбросе
    // (check_devices()):
    // клавиатуру и мышь лучше подключить до включения платы.
    esp::ps2::ps2_controller::begin(esp::ps2::ps2_preset::keyboard_port_0_mouse_port_1);
    esp::ps2::ps2_controller::keyboard()->set_layout(&esp::ps2::US_LAYOUT);
    _keyboard_found = esp::ps2::ps2_controller::keyboard()->is_keyboard_available();

    _mouse_found = esp::ps2::ps2_controller::mouse()->is_mouse_available();
    attach_mouse();

    ESP_LOGI(esp::p::TAG, "Keyboard %s, mouse %s.", _keyboard_found ? "found" : "not found",
             _mouse_found ? "found" : "not found");

    draw_screen();

    // Два звука при запуске проверяют звук и без клавиатуры.
    play(esp::p::NOTES[2], 120);
    vTaskDelay(pdMS_TO_TICKS(150));
    play(esp::p::NOTES[7], 200);

    ESP_LOGI(esp::p::TAG, "Free memory %zu kB, largest block %zu kB.",
             heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024);
}

/**
 * @brief Обрабатывает накопившиеся события клавиатуры и мыши.
 *
 * Событий не ждёт; приложение вызывает метод каждые несколько миллисекунд.
 */
void esp::board_test::poll()
{
    // Нулевой тайм-аут: забрать то, что уже есть в очереди, и не ждать.
    esp::ps2::keyboard *const keyboard = esp::ps2::ps2_controller::keyboard();
    esp::ps2::virtual_key_item item;
    while (keyboard->get_next_virtual_key(&item, 0))
        on_key(item);

    // При быстром движении состояний мыши приходит много: рисуются все,
    // а строка состояния обновляется один раз.
    esp::ps2::mouse *const mouse = esp::ps2::ps2_controller::mouse();
    bool moved = false;
    while (mouse->available_status())
    {
        on_mouse(mouse->get_next_status(0));
        moved = true;
    }

    if (moved)
        show_mouse();
}

/**
 * @brief Реакция на нажатие кнопки K1.
 *
 * Подаёт звук, обновляет счётчик нажатий и сведения о памяти и заново
 * ищет клавиатуру и мышь, если их не было (см. check_devices()).
 */
void esp::board_test::button_pressed()
{
    ++_presses;
    show_button();
    play(esp::p::NOTES[0], 100);
    check_devices();
    show_memory();
}


// Закрытые

/**
 * @brief Возвращает верхний край первой строки состояния.
 *
 * Сверху вниз идут заголовок, цветные полосы и номера под ними; высота
 * заголовка и номеров — высота знакоместа шрифта.
 *
 * @return Ордината, точки.
 */
int esp::board_test::status_top() const
{
    return esp::p::TITLE_TOP + _font.height() + 6 + esp::p::BAR_HEIGHT + 2 + _font.height() + 6;
}

/**
 * @brief Возвращает верхний край поля для рисования: оно под строками текста.
 * @return Ордината, точки.
 */
int esp::board_test::paint_top() const
{
    return status_top() + esp::p::rows * _font.height() + 6;
}

/**
 * @brief Сообщает, есть ли на экране поле для рисования.
 *
 * На низком экране (меньше 640 × 480) под строками текста места для поля
 * не остаётся; тогда мышь не рисует, а F10 не показывает проверку.
 *
 * @return @c true, если поле без рамки не ниже esp::p::PAINT_AREA_MIN_HEIGHT.
 */
bool esp::board_test::has_paint_area() const
{
    return _canvas->height() - esp::p::MARGIN - 2 - paint_top() >= esp::p::PAINT_AREA_MIN_HEIGHT;
}

/**
 * @brief Сообщает, помещается ли строка текста на экране.
 * @param row Номер строки (перечисление esp::p::row).
 * @return @c true, если строка целиком выше нижнего поля экрана.
 */
bool esp::board_test::row_visible(int row) const
{
    return status_top() + (row + 1) * _font.height() <= _canvas->height() - esp::p::MARGIN;
}

/**
 * @brief Сообщает, помещается ли проверка отсечения на поле для рисования.
 *
 * Окно проверки — поле без рамки и промежутков вокруг окна (см.
 * draw_clipping_test()).
 *
 * @return @c true, если окно не меньше esp::p::TEST_WINDOW_MIN_WIDTH на
 *         esp::p::TEST_WINDOW_MIN_HEIGHT.
 */
bool esp::board_test::clipping_test_fits() const
{
    const int window_width =
        _canvas->width() - 2 * esp::p::MARGIN - 2 - 2 * (esp::p::TEST_GAP_X + 1);
    const int window_height =
        _canvas->height() - esp::p::MARGIN - 2 - paint_top() - 2 * (esp::p::TEST_GAP_Y + 1);
    return window_width >= esp::p::TEST_WINDOW_MIN_WIDTH
        && window_height >= esp::p::TEST_WINDOW_MIN_HEIGHT;
}

/**
 * @brief Включает видеорежим из таблицы esp::p::DISPLAY_MODES.
 *
 * Прежний видеоконтроллер, если он был, останавливается и уничтожается:
 * его буфер кадра, очередь примитивов и прерывание освобождаются. На его
 * месте создаётся и запускается видеоконтроллер нового режима, для него —
 * новый холст, и указатель мыши переходит к нему. Экран после этого
 * пуст: его рисует draw_screen().
 *
 * @param index Номер видеорежима в esp::p::DISPLAY_MODES.
 */
void esp::board_test::set_video_mode(int index)
{
    if (_display)
    {
        detach_mouse();
        // Деструктор видеоконтроллера вывод не останавливает.
        _display->end();
        // Холст не должен ссылаться на уничтоженный видеоконтроллер.
        _canvas.reset();
    }

    const esp::p::display_mode &mode = esp::p::DISPLAY_MODES[index];
    switch (mode.driver)
    {
        case esp::p::vga_driver::vga16:
            _display = &_display_storage.emplace<esp::vga::vga16_controller>();
            break;

        case esp::p::vga_driver::vga8:
            _display = &_display_storage.emplace<esp::vga::vga8_controller>();
            break;

        case esp::p::vga_driver::vga4:
            _display = &_display_storage.emplace<esp::vga::vga4_controller>();
            break;

        case esp::p::vga_driver::vga2:
            _display = &_display_storage.emplace<esp::vga::vga2_controller>();
            break;

        case esp::p::vga_driver::vga64:
            _display = &_display_storage.emplace<esp::vga::vga_controller>();
            break;
    }
    _video_mode = index;
    _clipping_test_shown = false;

    _display->begin();
    _display->set_resolution(mode.modeline);

    _canvas.emplace(_display);
    // Знаки шрифта заливают фон знакоместа цветом кисти, поэтому новый
    // текст закрывает прежний.
    _canvas->set_glyph_options(esp::gfx::glyph_options().set_fill_background(true));

    // При запуске контроллер PS/2 ещё не включён: мышь подключает конструктор.
    if (esp::ps2::ps2_controller::initialized())
        attach_mouse();

    ESP_LOGI(esp::p::TAG, "VGA %dx%d, %d colours.", _canvas->width(), _canvas->height(),
             _display->colors_count());
}

/**
 * @brief Отдаёт мышь текущему видеоконтроллеру.
 *
 * Указатель двигает сам видеоконтроллер, а новые состояния мыши её
 * драйвер кладёт в очередь, которую читает poll(). Указатель начинает с
 * центра экрана.
 */
void esp::board_test::attach_mouse()
{
    esp::ps2::mouse *const mouse = esp::ps2::ps2_controller::mouse();
    mouse->setup_absolute_positioner(_canvas->width(), _canvas->height(), true, _display);
    _mouse = mouse->status();
    if (_mouse_found)
        _display->set_mouse_cursor(esp::p::CURSOR);
}

/**
 * @brief Отнимает мышь у текущего видеоконтроллера перед его остановкой.
 *
 * Задача мыши перестаёт двигать указатель контроллера; перемещение,
 * которое она уже начала ему передавать, успевает закончиться за паузу.
 * Очередь состояний мыши остаётся.
 */
void esp::board_test::detach_mouse()
{
    esp::ps2::ps2_controller::mouse()->setup_absolute_positioner(_canvas->width(),
                                                                 _canvas->height(), true, nullptr);
    vTaskDelay(pdMS_TO_TICKS(50));
}

/**
 * @brief Рисует проверочный экран целиком.
 */
void esp::board_test::draw_screen()
{
    const int width = _canvas->width();
    const int height = _canvas->height();
    const int line = _font.height();

    _canvas->set_brush_color(esp::gfx::color::black);
    _canvas->clear();

    // Рамка по самому краю изображения: по ней видно, помещается ли
    // изображение на экране монитора целиком и не сдвинуто ли оно.
    _canvas->set_pen_color(esp::gfx::color::bright_white);
    _canvas->draw_rectangle(0, 0, width - 1, height - 1);

    // Заголовок: белый текст на синем; справа — видеорежим и клавиша,
    // которая его меняет (в строках только знаки ASCII, поэтому число
    // знаков равно длине строки). На узком экране название короче или его
    // нет вовсе.
    _canvas->set_brush_color(esp::gfx::color::blue);
    _canvas->fill_rectangle(1, 1, width - 2, esp::p::TITLE_TOP + line);
    char mode[40];
    std::snprintf(mode, sizeof(mode), "%dx%d, %d colours (F12)", width, height,
                  _display->colors_count());
    const int mode_length = static_cast<int>(std::strlen(mode));
    const int cells = (width - 2 * esp::p::MARGIN) / _font.width();
    for (const char *title : {"Espresso: board test", "Espresso"})
    {
        if (static_cast<int>(std::strlen(title)) + 1 + mode_length <= cells)
        {
            _font.draw_text(*_canvas, esp::p::MARGIN, esp::p::TITLE_TOP, title);
            break;
        }
    }
    _font.draw_text(*_canvas, width - esp::p::MARGIN - _font.width() * mode_length,
                    esp::p::TITLE_TOP, mode);

    // Полосы всех цветов палитры с номерами под ними; между полосами
    // остаётся чёрный промежуток в одну точку.
    const int bars_top = esp::p::TITLE_TOP + line + 6;
    const int bar_width = (width - 2 * esp::p::MARGIN) / esp::p::COLORS;
    const int bars_left = (width - bar_width * esp::p::COLORS) / 2;
    for (int i = 0; i < esp::p::COLORS; ++i)
    {
        const int x = bars_left + i * bar_width;
        _canvas->set_brush_color(esp::p::color(i));
        _canvas->fill_rectangle(x, bars_top, x + bar_width - 2, bars_top + esp::p::BAR_HEIGHT - 1);

        char number[4];
        std::snprintf(number, sizeof(number), "%d", i);
        _canvas->set_pen_color(esp::gfx::color::white);
        _canvas->set_brush_color(esp::gfx::color::black);
        _font.draw_text(*_canvas,
                        x + (bar_width - _font.width() * static_cast<int>(std::strlen(number))) / 2,
                        bars_top + esp::p::BAR_HEIGHT + 2, number);
    }

    show_keyboard();
    draw_line(esp::p::row_key, "Key:      none yet");
    show_input();
    show_mouse();
    show_button();
    show_sound();
    show_memory();
    for (std::size_t i = 0; i < std::size(esp::p::SAMPLES); ++i)
        draw_line(esp::p::row_samples + static_cast<int>(i), esp::p::SAMPLES[i]);
    show_help();

    if (has_paint_area())
    {
        _canvas->set_pen_color(esp::gfx::color::white);
        _canvas->draw_rectangle(esp::p::MARGIN, paint_top(), width - esp::p::MARGIN - 1,
                                height - esp::p::MARGIN - 1);
    }
}

/**
 * @brief Выводит строку текста под цветными полосами.
 *
 * Текст, который не помещается до правого поля, обрезается. Остаток
 * строки закрашивается фоном, поэтому новый текст полностью закрывает
 * прежний, более длинный. Строка, которая не помещается на экране по
 * высоте, не выводится.
 *
 * @param row Номер строки (перечисление esp::p::row).
 * @param text Текст в кодировке UTF-8.
 */
void esp::board_test::draw_line(int row, const char *text)
{
    if (!row_visible(row))
        return;

    const int line = _font.height();
    const int y = status_top() + row * line;
    const int right = _canvas->width() - esp::p::MARGIN - 1;

    // Отсечение по правому полю: при широком знакоместе длинная строка
    // иначе залезла бы на рамку по краю изображения.
    _canvas->set_clipping_rect(esp::gfx::rect(esp::p::MARGIN, y, right, y + line - 1));
    _canvas->set_pen_color(esp::gfx::color::bright_white);
    _canvas->set_brush_color(esp::gfx::color::black);
    const int cells = _font.draw_text(*_canvas, esp::p::MARGIN, y, text);

    // Число знаков в UTF-8 не равно числу байтов, поэтому хвост строки
    // стирается прямоугольником, а не дополнением пробелами.
    const int x = esp::p::MARGIN + cells * _font.width();
    if (x <= right)
        _canvas->fill_rectangle(x, y, right, y + line - 1);
    _canvas->set_clipping_rect(esp::gfx::rect(0, 0, _canvas->width() - 1, _canvas->height() - 1));
}

/**
 * @brief Показывает, найдена ли клавиатура, и её раскладку.
 */
void esp::board_test::show_keyboard()
{
    char text[96];
    if (_keyboard_found)
        std::snprintf(text, sizeof(text), "Keyboard: found, layout %s",
                      esp::ps2::ps2_controller::keyboard()->layout()->desc);
    else
        std::snprintf(text, sizeof(text), "%s", "Keyboard: not found; connect it and press K1");
    draw_line(esp::p::row_keyboard, text);
}

/**
 * @brief Показывает последнюю нажатую или отпущенную клавишу.
 * @param item Событие клавиатуры.
 */
void esp::board_test::show_key(const esp::ps2::virtual_key_item &item)
{
    char text[96];
    std::snprintf(text, sizeof(text), "Key:      %s %s, ASCII 0x%02x",
                  esp::ps2::keyboard::virtual_key_to_string(item.vk), item.down ? "down" : "up",
                  item.ascii);
    draw_line(esp::p::row_key, text);
}

/**
 * @brief Показывает набранную строку.
 */
void esp::board_test::show_input()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Input:    %s_", _input.c_str());
    draw_line(esp::p::row_input, text);
}

/**
 * @brief Показывает положение и кнопки мыши или её отсутствие.
 */
void esp::board_test::show_mouse()
{
    char text[96];
    if (_mouse_found)
        std::snprintf(text, sizeof(text), "Mouse:    X %3d, Y %3d, buttons %c%c%c", _mouse.x,
                      _mouse.y, _mouse.buttons.left ? 'L' : '-', _mouse.buttons.middle ? 'M' : '-',
                      _mouse.buttons.right ? 'R' : '-');
    else
        std::snprintf(text, sizeof(text), "%s", "Mouse:    not found; connect it and press K1");
    draw_line(esp::p::row_mouse, text);
}

/**
 * @brief Показывает число нажатий кнопки K1.
 */
void esp::board_test::show_button()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Button:   K1 (GPIO36), presses: %d", _presses);
    draw_line(esp::p::row_button, text);
}

/**
 * @brief Показывает форму звуковой волны и подсказку о клавишах звука.
 */
void esp::board_test::show_sound()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
                  esp::p::WAVEFORMS[_waveform]);
    draw_line(esp::p::row_sound, text);
}

/**
 * @brief Показывает объём свободной памяти и наибольший свободный блок.
 */
void esp::board_test::show_memory()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Memory:   %zu kB free, largest block %zu kB",
                  heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024);
    draw_line(esp::p::row_memory, text);
}

/**
 * @brief Показывает строку подсказки и образец цвета рисования в её конце.
 *
 * Пока показана проверка отсечения, подсказка напоминает, что фигуры не
 * должны выходить за внутреннюю рамку. Если поля для рисования нет,
 * подсказка говорит об этом, а образца цвета нет.
 */
void esp::board_test::show_help()
{
    if (!has_paint_area())
    {
        draw_line(esp::p::row_help, "No room to draw in this video mode; F12 changes it.");
        return;
    }
    draw_line(esp::p::row_help,
              _clipping_test_shown ? "Test: shapes must stay inside the inner frame. F10: back"
                                   : "Mouse: left draws, right clears, wheel: colour. F10: test");
    show_paint_color();
}

/**
 * @brief Показывает образец цвета рисования в конце строки подсказки.
 *
 * Без поля для рисования или строки подсказки на экране образца нет.
 */
void esp::board_test::show_paint_color()
{
    if (!has_paint_area() || !row_visible(esp::p::row_help))
        return;

    const int right = _canvas->width() - esp::p::MARGIN - 1;
    const int top = status_top() + esp::p::row_help * _font.height() + 1;
    const int bottom = top + _font.height() - 3;

    _canvas->set_pen_color(esp::gfx::color::white);
    _canvas->set_brush_color(esp::p::color(_paint_color));
    _canvas->fill_rectangle(right - 31, top, right, bottom);
    _canvas->draw_rectangle(right - 31, top, right, bottom);
}

/**
 * @brief Очищает поле для рисования, не трогая его рамку.
 *
 * Если на поле была проверка отсечения, возвращает обычную подсказку.
 * Без поля для рисования ничего не делает.
 */
void esp::board_test::clear_paint_area()
{
    if (!has_paint_area())
        return;

    _canvas->set_brush_color(esp::gfx::color::black);
    _canvas->fill_rectangle(esp::p::MARGIN + 1, paint_top() + 1,
                            _canvas->width() - esp::p::MARGIN - 2,
                            _canvas->height() - esp::p::MARGIN - 2);
    if (_clipping_test_shown)
    {
        _clipping_test_shown = false;
        show_help();
    }
}

/**
 * @brief Рисует отрезок текущим цветом рисования.
 *
 * Части отрезка вне поля для рисования отсекаются.
 *
 * @param x1 Абсцисса начала.
 * @param y1 Ордината начала.
 * @param x2 Абсцисса конца.
 * @param y2 Ордината конца.
 */
void esp::board_test::paint(int x1, int y1, int x2, int y2)
{
    const int width = _canvas->width();
    const int height = _canvas->height();

    // Отсечение по внутренней части поля: рамка и строки над полем
    // остаются нетронутыми. Затем отсечение и толщина линии
    // возвращаются к обычным.
    _canvas->set_clipping_rect(esp::gfx::rect(esp::p::MARGIN + 1, paint_top() + 1,
                                              width - esp::p::MARGIN - 2,
                                              height - esp::p::MARGIN - 2));
    _canvas->set_pen_color(esp::p::color(_paint_color));
    _canvas->set_pen_width(esp::p::PAINT_PEN_WIDTH);
    _canvas->draw_line(x1, y1, x2, y2);
    _canvas->set_pen_width(1);
    _canvas->set_clipping_rect(esp::gfx::rect(0, 0, width - 1, height - 1));
}

/**
 * @brief Рисует на поле для рисования проверку фигур и отсечения.
 *
 * Посреди поля — окно в ярко-белой рамке. Все фигуры рисуются с
 * отсечением по окну, поэтому между рамкой окна и рамкой поля всё должно
 * остаться чёрным. Внутри окна на синем фоне:
 * - по углам — четверти прямоугольников, у левого края — половина
 *   залитого эллипса, у правого — половина контура эллипса; бирюзовые
 *   горизонтальная, вертикальная и наклонная линии доходят до рамки;
 * - слева — эллипсы высотой в одну строку шириной 41, 21, 9, 3 и 1 точка;
 * - внизу — инвертированный прямоугольник, который выходит за окно: синий
 *   фон в нём ярко-жёлтый, бирюзовая линия — тёмно-красная;
 * - вверху — красные полосы; в их верхней части, которую задевает
 *   выходящий за окно прямоугольник, цвета пера и кисти обменяны: полосы
 *   там синие на красном.
 *
 * Фигуры целиком вне окна следа не оставляют: инверсия слева, эллипсы
 * высотой в строку справа и обмен цветов снизу. Прежде от них на
 * внутренней стороне рамки оставались линии в одну точку: ярко-жёлтые
 * слева и справа над половинами эллипсов и красная внизу под полосами.
 * Последними фигуры рисуются через пустые области отсечения (ширина или
 * высота −1 точка) посередине окна: от них не появляется ничего.
 */
void esp::board_test::draw_clipping_test()
{
    // Поле для рисования без его рамки.
    const int left = esp::p::MARGIN + 1;
    const int top = paint_top() + 1;
    const int right = _canvas->width() - esp::p::MARGIN - 2;
    const int bottom = _canvas->height() - esp::p::MARGIN - 2;

    // Окно, по которому идёт отсечение; его рамка — на точку снаружи.
    const int x1 = left + esp::p::TEST_GAP_X + 1;
    const int y1 = top + esp::p::TEST_GAP_Y + 1;
    const int x2 = right - esp::p::TEST_GAP_X - 1;
    const int y2 = bottom - esp::p::TEST_GAP_Y - 1;
    const int center_x = (x1 + x2) / 2;
    const int center_y = (y1 + y2) / 2;

    clear_paint_area();
    _canvas->set_pen_color(esp::gfx::color::bright_white);
    _canvas->draw_rectangle(x1 - 1, y1 - 1, x2 + 1, y2 + 1);

    _canvas->set_clipping_rect(esp::gfx::rect(x1, y1, x2, y2));
    _canvas->set_brush_color(esp::gfx::color::blue);
    _canvas->fill_rectangle(x1, y1, x2, y2);

    // Прямоугольники 48 × 24 с центрами в углах окна: видна четверть каждого.
    const auto corner = [this](int x, int y, esp::gfx::color color)
    {
        _canvas->set_brush_color(color);
        _canvas->fill_rectangle(x - 24, y - 12, x + 23, y + 11);
    };
    corner(x1, y1, esp::gfx::color::bright_red);
    corner(x2, y1, esp::gfx::color::bright_green);
    corner(x1, y2, esp::gfx::color::bright_magenta);
    corner(x2, y2, esp::gfx::color::white);

    // Эллипсы с центрами на левом и правом краях: видна половина каждого.
    _canvas->set_brush_color(esp::gfx::color::bright_green);
    _canvas->fill_ellipse(x1, center_y, 72, 56);
    _canvas->set_pen_color(esp::gfx::color::bright_white);
    _canvas->draw_ellipse(x2, center_y, 72, 56);

    // Эллипсы высотой в одну строку; последний — в одну точку.
    _canvas->set_brush_color(esp::gfx::color::bright_yellow);
    const int widths[] = {41, 21, 9, 3, 1};
    for (int i = 0; i < static_cast<int>(std::size(widths)); ++i)
        _canvas->fill_ellipse(x1 + 90, center_y - 12 + 6 * i, widths[i], 1);

    // Линии через всё поле: видны только их части внутри окна.
    _canvas->set_pen_color(esp::gfx::color::bright_cyan);
    _canvas->draw_line(left, y2 - 20, right, y2 - 20);
    _canvas->draw_line(x1 + 120, top, x1 + 120, bottom);
    _canvas->draw_line(x1 + 430, top, x1 + 510, bottom);

    // Инверсия прямоугольника, который выходит за нижний край окна.
    _canvas->invert_rectangle(x1 + 150, y2 - 24, x1 + 230, bottom - 2);

    // Красные полосы, затем обмен цветов пера и кисти в прямоугольнике,
    // который выходит за верхний край окна.
    _canvas->set_brush_color(esp::gfx::color::bright_red);
    for (int x = x1 + 280; x < x1 + 360; x += 8)
        _canvas->fill_rectangle(x, y1, x + 3, y1 + 30);
    _canvas->set_pen_color(esp::gfx::color::bright_red);
    _canvas->set_brush_color(esp::gfx::color::blue);
    _canvas->swap_rectangle(x1 + 270, top + 2, x1 + 370, y1 + 15);

    // Фигуры целиком вне окна: инверсия слева, обмен цветов снизу,
    // эллипсы высотой в строку справа. Следов остаться не должно.
    _canvas->invert_rectangle(left + 4, y1 + 14, x1 - 6, y1 + 25);
    _canvas->swap_rectangle(x1 + 270, y2 + 3, x1 + 370, bottom - 2);
    _canvas->set_brush_color(esp::gfx::color::bright_yellow);
    for (int y = y1 + 14; y <= y1 + 25; ++y)
        _canvas->fill_ellipse(x2 + 1 + esp::p::TEST_GAP_X / 2, y, 15, 1);

    // Пустые области отсечения (ширина или высота −1 точка): через них не
    // рисуется ничего, иначе посередине окна появилась бы белая линия.
    const esp::gfx::rect empty_clips[] = {esp::gfx::rect(center_x, y1, center_x - 1, y2),
                                          esp::gfx::rect(x1, center_y, x2, center_y - 1)};
    for (const esp::gfx::rect &clip : empty_clips)
    {
        _canvas->set_clipping_rect(clip);
        _canvas->set_brush_color(esp::gfx::color::bright_white);
        _canvas->fill_rectangle(x1, y1, x2, y2);
        _canvas->fill_ellipse(center_x, center_y, 200, 60);
        _canvas->fill_ellipse(center_x, center_y, 200, 1);
        _canvas->set_pen_color(esp::gfx::color::bright_white);
        _canvas->draw_line(x1, center_y, x2, center_y);
        _canvas->draw_line(center_x, y1, center_x, y2);
        _canvas->draw_line(x1, y1, x2, y2);
        _canvas->invert_rectangle(x1, y1, x2, y2);
        _canvas->set_pen_color(esp::gfx::color::bright_red);
        _canvas->set_brush_color(esp::gfx::color::blue);
        _canvas->swap_rectangle(x1, y1, x2, y2);
    }

    _canvas->set_clipping_rect(esp::gfx::rect(0, 0, _canvas->width() - 1, _canvas->height() - 1));

    _clipping_test_shown = true;
    show_help();
}

/**
 * @brief Обрабатывает событие клавиатуры.
 *
 * Нажатия выводятся в консоль, отпускания — только в отладочный журнал.
 *
 * @param item Событие клавиатуры.
 */
void esp::board_test::on_key(const esp::ps2::virtual_key_item &item)
{
    show_key(item);

    const char *const name = esp::ps2::keyboard::virtual_key_to_string(item.vk);
    if (!item.down)
    {
        ESP_LOGD(esp::p::TAG, "Key %s up.", name);
        return;
    }

    ESP_LOGI(esp::p::TAG, "Key %s down, ASCII 0x%02x.", name, item.ascii);

    // Коды F1–F8 в перечислении esp::ps2::virtual_key идут подряд.
    if (item.vk >= esp::ps2::virtual_key::vk_f1 && item.vk <= esp::ps2::virtual_key::vk_f8)
    {
        play(esp::p::NOTES[static_cast<int>(item.vk)
                           - static_cast<int>(esp::ps2::virtual_key::vk_f1)],
             esp::p::NOTE_MS);
        return;
    }

    switch (item.vk)
    {
        case esp::ps2::virtual_key::vk_f9:
            _waveform = (_waveform + 1) % static_cast<int>(std::size(esp::p::WAVEFORMS));
            show_sound();
            ESP_LOGI(esp::p::TAG, "Waveform %s.", esp::p::WAVEFORMS[_waveform]);
            play(esp::p::NOTES[0], esp::p::NOTE_MS);
            return;

        case esp::ps2::virtual_key::vk_f10:
            if (_clipping_test_shown)
                clear_paint_area();
            else if (clipping_test_fits())
                draw_clipping_test();
            return;

        case esp::ps2::virtual_key::vk_f12:
            set_video_mode((_video_mode + 1) % static_cast<int>(std::size(esp::p::DISPLAY_MODES)));
            draw_screen();
            return;

        case esp::ps2::virtual_key::vk_escape:
            _input.clear();
            show_input();
            return;

        case esp::ps2::virtual_key::vk_backspace:
            if (!_input.empty())
                _input.pop_back();
            show_input();
            return;

        case esp::ps2::virtual_key::vk_return:
        case esp::ps2::virtual_key::vk_kp_enter:
            ESP_LOGI(esp::p::TAG, "Input: \"%s\".", _input.c_str());
            _input.clear();
            show_input();
            return;

        default:
            break;
    }

    // В строку попадают только печатные символы ASCII; клавиатура уже
    // учла Shift, Caps Lock и раскладку.
    if (item.ascii >= 0x20 && item.ascii < 0x7f && _input.size() < esp::p::INPUT_MAX)
    {
        _input += static_cast<char>(item.ascii);
        show_input();
    }
}

/**
 * @brief Обрабатывает новое состояние мыши.
 *
 * Пока нажата левая кнопка, рисует линию от прежнего положения к новому;
 * правая кнопка очищает поле, средняя подаёт звук, колесо меняет цвет
 * рисования. Без поля для рисования левая и правая кнопки ничего не
 * делают. Нажатия и отпускания кнопок выводятся в консоль.
 *
 * @param status Новое состояние мыши.
 */
void esp::board_test::on_mouse(const esp::ps2::mouse_status &status)
{
    const esp::ps2::mouse_status previous = _mouse;
    _mouse = status;

    // Линия от прежнего положения делает след сплошным и при быстром
    // движении; в момент нажатия рисуется точка.
    if (status.buttons.left && has_paint_area())
    {
        if (previous.buttons.left)
            paint(previous.x, previous.y, status.x, status.y);
        else
            paint(status.x, status.y, status.x, status.y);
    }

    if (status.buttons.right && !previous.buttons.right)
        clear_paint_area();

    if (status.buttons.middle && !previous.buttons.middle)
        play(esp::p::NOTES[4], 100);

    // Колесо перебирает 16 цветов палитры по кругу в обе стороны.
    if (status.wheel_delta)
    {
        _paint_color =
            (_paint_color + (status.wheel_delta > 0 ? 1 : esp::p::COLORS - 1)) % esp::p::COLORS;
        show_paint_color();
        ESP_LOGD(esp::p::TAG, "Paint colour %d.", _paint_color);
    }

    const auto report = [&status](bool now, bool before, const char *name)
    {
        if (now != before)
            ESP_LOGI(esp::p::TAG, "Mouse %s button %s at %d, %d.", name,
                     now ? "pressed" : "released", status.x, status.y);
    };
    report(status.buttons.left, previous.buttons.left, "left");
    report(status.buttons.middle, previous.buttons.middle, "middle");
    report(status.buttons.right, previous.buttons.right, "right");
}

/**
 * @brief Заново ищет клавиатуру и мышь, которых не было.
 *
 * Найденное устройство не трогается. Сброс устройства занимает до двух
 * секунд, всё это время основной цикл стоит.
 */
void esp::board_test::check_devices()
{
    if (!_keyboard_found)
    {
        draw_line(esp::p::row_keyboard, "Keyboard: searching...");
        esp::ps2::keyboard *const keyboard = esp::ps2::ps2_controller::keyboard();
        _keyboard_found = keyboard->reset();
        // Сброс ставит раскладку US; здесь она та же, но задаётся явно.
        keyboard->set_layout(&esp::ps2::US_LAYOUT);
        ESP_LOGI(esp::p::TAG, "Keyboard %s.", _keyboard_found ? "found" : "not found");
        show_keyboard();
    }

    if (!_mouse_found)
    {
        draw_line(esp::p::row_mouse, "Mouse:    searching...");
        _mouse_found = esp::ps2::ps2_controller::mouse()->reset();
        if (_mouse_found)
            _display->set_mouse_cursor(esp::p::CURSOR);
        ESP_LOGI(esp::p::TAG, "Mouse %s.", _mouse_found ? "found" : "not found");
        show_mouse();
    }
}

/**
 * @brief Подаёт звук текущей формы волны.
 *
 * Не ждёт окончания звука: генератор звука сам освобождает голос, когда
 * длительность истекает. Если все голоса заняты, звук пропускается.
 *
 * @param frequency Частота, Гц; для шума не важна.
 * @param duration_ms Длительность, мс.
 */
void esp::board_test::play(int frequency, int duration_ms)
{
    // esp::sound::sound_generator::play_sound() — шаблон, тип генератора выбирается при компиляции,
    // поэтому форму волны выбирает переключатель.
    switch (_waveform)
    {
        case 0:
            _sound.play_sound(esp::sound::sine_waveform_generator(), frequency, duration_ms,
                              esp::p::VOLUME);
            break;
        case 1:
            _sound.play_sound(esp::sound::square_waveform_generator(), frequency, duration_ms,
                              esp::p::VOLUME);
            break;
        case 2:
            _sound.play_sound(esp::sound::triangle_waveform_generator(), frequency, duration_ms,
                              esp::p::VOLUME);
            break;
        case 3:
            _sound.play_sound(esp::sound::sawtooth_waveform_generator(), frequency, duration_ms,
                              esp::p::VOLUME);
            break;
        default:
            _sound.play_sound(esp::sound::noise_waveform_generator(), frequency, duration_ms,
                              esp::p::VOLUME);
            break;
    }
}
