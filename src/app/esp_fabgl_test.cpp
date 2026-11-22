/**
 * @file esp_fabgl_test.h
 * @brief Объявление класса проверки FabGL esp::fabgl_test.
 */

/**
 * @file esp_fabgl_test.cpp
 * @brief Проверка FabGL на плате TTGO VGA32: VGA, клавиатура и мышь PS/2,
 *        звук, шрифт Unicode.
 */

#include "esp_fabgl_test.h"

#include "esp_gfx_display_controller.h"
#include "esp_gfx_utils.h"
#include "esp_ps2_keyboard.h"
#include "esp_ps2_keyboard_layouts.h"
#include "esp_ps2_mouse.h"
#include "esp_ps2_ps2_controller.h"
#include "esp_vga_modelines.h"
#include "ter_10x18.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <cstring>
#include <iterator>


/**
 * @class esp::fabgl_test
 * @brief Проверка FabGL на плате TTGO VGA32 v1.4.
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
 * форму звуковой волны.
 *
 * Мышь: левая кнопка рисует, правая очищает поле, средняя подаёт звук,
 * колесо меняет цвет рисования.
 *
 * Кнопка K1 (button_pressed()) подаёт звук и заново ищет клавиатуру и
 * мышь, если их не было. События клавиатуры и мыши обрабатывает poll(),
 * которую приложение вызывает каждые несколько миллисекунд.
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "test";

/** @brief Режим VGA: 640 × 480 точек, 60 Гц. */
const char *const VGA_MODE = VGA_640x480_60Hz;

/** @brief Поле между краем изображения и содержимым экрана, точки. */
constexpr int MARGIN = 8;

/** @brief Верхний край текста заголовка. */
constexpr int TITLE_TOP = 2;

/** @brief Высота цветных полос, точки. */
constexpr int BAR_HEIGHT = 24;

/** @brief Число цветов палитры esp::vga::vga16_controller. */
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
    row_keyboard, /**< Клавиатура и её раскладка. */
    row_key, /**< Последняя нажатая или отпущенная клавиша. */
    row_input, /**< Набранная строка. */
    row_mouse, /**< Положение и кнопки мыши. */
    row_button, /**< Кнопка K1. */
    row_sound, /**< Форма звуковой волны. */
    row_memory, /**< Свободная память. */
    row_samples, /**< Первая строка образцов текста SAMPLES. */
    row_help = row_samples
        + static_cast<int>(std::size(SAMPLES)), /**< Подсказка о мыши и образец цвета рисования. */
    rows /**< Число строк. */
};

/** @brief Толщина линии при рисовании мышью, точки. */
constexpr int PAINT_PEN_WIDTH = 3;

/** @brief Наибольшая длина набираемой строки, символы. */
constexpr size_t INPUT_MAX = 60;

/** @brief Громкость звуков: от 0 до 127. */
constexpr int VOLUME = 64;

/** @brief Частоты нот для клавиш F1–F8: гамма до мажор от C5 до C6, Гц. */
constexpr int NOTES[] = {523, 587, 659, 698, 784, 880, 988, 1047};

/** @brief Длительность ноты, мс. */
constexpr int NOTE_MS = 250;

/** @brief Названия форм звуковой волны в порядке номеров (см. esp::fabgl_test::play()). */
const char *const WAVEFORMS[] = {"sine", "square", "triangle", "sawtooth", "noise"};

static_assert(std::size(WAVEFORMS) == 5, "WAVEFORMS must match esp::fabgl_test::play()");

/** @brief Указатель мыши. */
constexpr esp::gfx::cursor_name CURSOR = esp::gfx::cursor_pointer_shadowed;

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
 * Выводы — по умолчанию FabGL; они совпадают с разводкой платы TTGO VGA32:
 * красный 21 и 22, зелёный 18 и 19, синий 4 и 5, HSync 23, VSync 15;
 * клавиатура CLK 33, DATA 32; мышь CLK 26, DATA 27; звук — ЦАП на GPIO25.
 */
esp::fabgl_test::fabgl_test()
    : _canvas(&_display)
    , _font(ter_10x18::WIDTH, ter_10x18::HEIGHT, ter_10x18::CODEPOINTS, ter_10x18::GLYPHS,
            ter_10x18::COUNT, ter_10x18::FALLBACK)
    , _waveform(0)
    , _paint_color(esp::gfx::bright_yellow)
    , _presses(0)
    , _keyboard_found(false)
    , _mouse_found(false)
{
    // По два бита на каждую составляющую цвета; контроллер выбирает из
    // этих 64 цветов палитру из 16. Кадр — 4 бита на точку, 150 КБ памяти.
    _display.begin();
    _display.set_resolution(VGA_MODE);
    ESP_LOGI(TAG, "VGA %dx%d, %d colours.", _canvas.width(), _canvas.height(), COLORS);

    // Знаки шрифта заливают фон знакоместа цветом кисти, поэтому новый
    // текст закрывает прежний.
    _canvas.set_glyph_options(esp::gfx::glyph_options().set_fill_background(true));

    // FabGL ищет устройства только здесь и при их сбросе (check_devices()):
    // клавиатуру и мышь лучше подключить до включения платы.
    esp::ps2::ps2_controller::begin(esp::ps2::ps2_preset::keyboard_port_0_mouse_port_1);
    esp::ps2::ps2_controller::keyboard()->set_layout(&esp::ps2::US_LAYOUT);
    _keyboard_found = esp::ps2::ps2_controller::keyboard()->is_keyboard_available();

    // Указатель мыши двигает сама FabGL, а новые состояния мыши она кладёт
    // в очередь, которую читает poll(). Указатель начинает с центра экрана.
    esp::ps2::mouse *const mouse = esp::ps2::ps2_controller::mouse();
    mouse->setup_absolute_positioner(_canvas.width(), _canvas.height(), true, &_display);
    _mouse = mouse->status();
    _mouse_found = mouse->is_mouse_available();
    if (_mouse_found)
        _display.set_mouse_cursor(CURSOR);

    ESP_LOGI(TAG, "Keyboard %s, mouse %s.", _keyboard_found ? "found" : "not found",
             _mouse_found ? "found" : "not found");

    draw_screen();

    // Два звука при запуске проверяют звук и без клавиатуры.
    play(NOTES[2], 120);
    vTaskDelay(pdMS_TO_TICKS(150));
    play(NOTES[7], 200);

    ESP_LOGI(TAG, "Free memory %zu kB, largest block %zu kB.",
             heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024);
}

/**
 * @brief Обрабатывает накопившиеся события клавиатуры и мыши.
 *
 * Событий не ждёт; приложение вызывает метод каждые несколько миллисекунд.
 */
void esp::fabgl_test::poll()
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
void esp::fabgl_test::button_pressed()
{
    ++_presses;
    show_button();
    play(NOTES[0], 100);
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
int esp::fabgl_test::status_top() const
{
    return TITLE_TOP + _font.height() + 6 + BAR_HEIGHT + 2 + _font.height() + 6;
}

/**
 * @brief Возвращает верхний край поля для рисования: оно под строками текста.
 * @return Ордината, точки.
 */
int esp::fabgl_test::paint_top() const
{
    return status_top() + rows * _font.height() + 6;
}

/**
 * @brief Рисует проверочный экран целиком.
 */
void esp::fabgl_test::draw_screen()
{
    const int width = _canvas.width();
    const int height = _canvas.height();
    const int line = _font.height();

    _canvas.set_brush_color(esp::gfx::black);
    _canvas.clear();

    // Рамка по самому краю изображения: по ней видно, помещается ли
    // изображение на экране монитора целиком и не сдвинуто ли оно.
    _canvas.set_pen_color(esp::gfx::bright_white);
    _canvas.draw_rectangle(0, 0, width - 1, height - 1);

    // Заголовок: белый текст на синем; справа — режим (в строке только
    // знаки ASCII, поэтому число знаков равно её длине).
    _canvas.set_brush_color(esp::gfx::blue);
    _canvas.fill_rectangle(1, 1, width - 2, TITLE_TOP + line);
    _font.draw_text(_canvas, MARGIN, TITLE_TOP, "Espresso: FabGL test");
    char mode[32];
    std::snprintf(mode, sizeof(mode), "%dx%d, %d colours", width, height, COLORS);
    _font.draw_text(_canvas, width - MARGIN - _font.width() * static_cast<int>(std::strlen(mode)),
                    TITLE_TOP, mode);

    // Полосы всех цветов палитры с номерами под ними; между полосами
    // остаётся чёрный промежуток в одну точку.
    const int bars_top = TITLE_TOP + line + 6;
    const int bar_width = (width - 2 * MARGIN) / COLORS;
    const int bars_left = (width - bar_width * COLORS) / 2;
    for (int i = 0; i < COLORS; ++i)
    {
        const int x = bars_left + i * bar_width;
        _canvas.set_brush_color(color(i));
        _canvas.fill_rectangle(x, bars_top, x + bar_width - 2, bars_top + BAR_HEIGHT - 1);

        char number[4];
        std::snprintf(number, sizeof(number), "%d", i);
        _canvas.set_pen_color(esp::gfx::white);
        _canvas.set_brush_color(esp::gfx::black);
        _font.draw_text(_canvas,
                        x + (bar_width - _font.width() * static_cast<int>(std::strlen(number))) / 2,
                        bars_top + BAR_HEIGHT + 2, number);
    }

    show_keyboard();
    draw_line(row_key, "Key:      none yet");
    show_input();
    show_mouse();
    show_button();
    show_sound();
    show_memory();
    for (size_t i = 0; i < std::size(SAMPLES); ++i)
        draw_line(row_samples + static_cast<int>(i), SAMPLES[i]);
    draw_line(row_help, "Mouse: left draws, right clears, wheel changes the colour.");
    show_paint_color();

    _canvas.set_pen_color(esp::gfx::white);
    _canvas.draw_rectangle(MARGIN, paint_top(), width - MARGIN - 1, height - MARGIN - 1);
}

/**
 * @brief Выводит строку текста под цветными полосами.
 *
 * Текст, который не помещается до правого поля, обрезается. Остаток
 * строки закрашивается фоном, поэтому новый текст полностью закрывает
 * прежний, более длинный.
 *
 * @param row Номер строки (перечисление row).
 * @param text Текст в кодировке UTF-8.
 */
void esp::fabgl_test::draw_line(int row, const char *text)
{
    const int line = _font.height();
    const int y = status_top() + row * line;
    const int right = _canvas.width() - MARGIN - 1;

    // Отсечение по правому полю: при широком знакоместе длинная строка
    // иначе залезла бы на рамку по краю изображения.
    _canvas.set_clipping_rect(esp::gfx::rect(MARGIN, y, right, y + line - 1));
    _canvas.set_pen_color(esp::gfx::bright_white);
    _canvas.set_brush_color(esp::gfx::black);
    const int cells = _font.draw_text(_canvas, MARGIN, y, text);

    // Число знаков в UTF-8 не равно числу байтов, поэтому хвост строки
    // стирается прямоугольником, а не дополнением пробелами.
    const int x = MARGIN + cells * _font.width();
    if (x <= right)
        _canvas.fill_rectangle(x, y, right, y + line - 1);
    _canvas.set_clipping_rect(esp::gfx::rect(0, 0, _canvas.width() - 1, _canvas.height() - 1));
}

/**
 * @brief Показывает, найдена ли клавиатура, и её раскладку.
 */
void esp::fabgl_test::show_keyboard()
{
    char text[96];
    if (_keyboard_found)
        std::snprintf(text, sizeof(text), "Keyboard: found, layout %s",
                      esp::ps2::ps2_controller::keyboard()->layout()->desc);
    else
        std::snprintf(text, sizeof(text), "%s", "Keyboard: not found; connect it and press K1");
    draw_line(row_keyboard, text);
}

/**
 * @brief Показывает последнюю нажатую или отпущенную клавишу.
 * @param item Событие клавиатуры.
 */
void esp::fabgl_test::show_key(const esp::ps2::virtual_key_item &item)
{
    char text[96];
    std::snprintf(text, sizeof(text), "Key:      %s %s, ASCII 0x%02x",
                  esp::ps2::keyboard::virtual_key_to_string(item.vk), item.down ? "down" : "up",
                  item.ascii);
    draw_line(row_key, text);
}

/**
 * @brief Показывает набранную строку.
 */
void esp::fabgl_test::show_input()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Input:    %s_", _input.c_str());
    draw_line(row_input, text);
}

/**
 * @brief Показывает положение и кнопки мыши или её отсутствие.
 */
void esp::fabgl_test::show_mouse()
{
    char text[96];
    if (_mouse_found)
        std::snprintf(text, sizeof(text), "Mouse:    X %3d, Y %3d, buttons %c%c%c", _mouse.x,
                      _mouse.y, _mouse.buttons.left ? 'L' : '-', _mouse.buttons.middle ? 'M' : '-',
                      _mouse.buttons.right ? 'R' : '-');
    else
        std::snprintf(text, sizeof(text), "%s", "Mouse:    not found; connect it and press K1");
    draw_line(row_mouse, text);
}

/**
 * @brief Показывает число нажатий кнопки K1.
 */
void esp::fabgl_test::show_button()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Button:   K1 (GPIO36), presses: %d", _presses);
    draw_line(row_button, text);
}

/**
 * @brief Показывает форму звуковой волны и подсказку о клавишах звука.
 */
void esp::fabgl_test::show_sound()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
                  WAVEFORMS[_waveform]);
    draw_line(row_sound, text);
}

/**
 * @brief Показывает объём свободной памяти и наибольший свободный блок.
 */
void esp::fabgl_test::show_memory()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Memory:   %zu kB free, largest block %zu kB",
                  heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024);
    draw_line(row_memory, text);
}

/**
 * @brief Показывает образец цвета рисования в конце строки подсказки.
 */
void esp::fabgl_test::show_paint_color()
{
    const int right = _canvas.width() - MARGIN - 1;
    const int top = status_top() + row_help * _font.height() + 1;
    const int bottom = top + _font.height() - 3;

    _canvas.set_pen_color(esp::gfx::white);
    _canvas.set_brush_color(color(_paint_color));
    _canvas.fill_rectangle(right - 31, top, right, bottom);
    _canvas.draw_rectangle(right - 31, top, right, bottom);
}

/**
 * @brief Очищает поле для рисования, не трогая его рамку.
 */
void esp::fabgl_test::clear_paint_area()
{
    _canvas.set_brush_color(esp::gfx::black);
    _canvas.fill_rectangle(MARGIN + 1, paint_top() + 1, _canvas.width() - MARGIN - 2,
                           _canvas.height() - MARGIN - 2);
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
void esp::fabgl_test::paint(int x1, int y1, int x2, int y2)
{
    const int width = _canvas.width();
    const int height = _canvas.height();

    // Отсечение по внутренней части поля: рамка и строки над полем
    // остаются нетронутыми. Затем отсечение и толщина линии
    // возвращаются к обычным.
    _canvas.set_clipping_rect(
        esp::gfx::rect(MARGIN + 1, paint_top() + 1, width - MARGIN - 2, height - MARGIN - 2));
    _canvas.set_pen_color(color(_paint_color));
    _canvas.set_pen_width(PAINT_PEN_WIDTH);
    _canvas.draw_line(x1, y1, x2, y2);
    _canvas.set_pen_width(1);
    _canvas.set_clipping_rect(esp::gfx::rect(0, 0, width - 1, height - 1));
}

/**
 * @brief Обрабатывает событие клавиатуры.
 *
 * Нажатия выводятся в консоль, отпускания — только в отладочный журнал.
 *
 * @param item Событие клавиатуры.
 */
void esp::fabgl_test::on_key(const esp::ps2::virtual_key_item &item)
{
    show_key(item);

    const char *const name = esp::ps2::keyboard::virtual_key_to_string(item.vk);
    if (!item.down)
    {
        ESP_LOGD(TAG, "Key %s up.", name);
        return;
    }

    ESP_LOGI(TAG, "Key %s down, ASCII 0x%02x.", name, item.ascii);

    // Коды F1–F8 в перечислении esp::ps2::virtual_key идут подряд.
    if (item.vk >= esp::ps2::vk_f1 && item.vk <= esp::ps2::vk_f8)
    {
        play(NOTES[item.vk - esp::ps2::vk_f1], NOTE_MS);
        return;
    }

    switch (item.vk)
    {
        case esp::ps2::vk_f9:
            _waveform = (_waveform + 1) % static_cast<int>(std::size(WAVEFORMS));
            show_sound();
            ESP_LOGI(TAG, "Waveform %s.", WAVEFORMS[_waveform]);
            play(NOTES[0], NOTE_MS);
            return;

        case esp::ps2::vk_escape:
            _input.clear();
            show_input();
            return;

        case esp::ps2::vk_backspace:
            if (!_input.empty())
                _input.pop_back();
            show_input();
            return;

        case esp::ps2::vk_return:
        case esp::ps2::vk_kp_enter:
            ESP_LOGI(TAG, "Input: \"%s\".", _input.c_str());
            _input.clear();
            show_input();
            return;

        default:
            break;
    }

    // В строку попадают только печатные символы ASCII; FabGL уже учла
    // Shift, Caps Lock и раскладку.
    if (item.ascii >= 0x20 && item.ascii < 0x7f && _input.size() < INPUT_MAX)
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
 * рисования. Нажатия и отпускания кнопок выводятся в консоль.
 *
 * @param status Новое состояние мыши.
 */
void esp::fabgl_test::on_mouse(const esp::ps2::mouse_status &status)
{
    const esp::ps2::mouse_status previous = _mouse;
    _mouse = status;

    // Линия от прежнего положения делает след сплошным и при быстром
    // движении; в момент нажатия рисуется точка.
    if (status.buttons.left)
    {
        if (previous.buttons.left)
            paint(previous.x, previous.y, status.x, status.y);
        else
            paint(status.x, status.y, status.x, status.y);
    }

    if (status.buttons.right && !previous.buttons.right)
        clear_paint_area();

    if (status.buttons.middle && !previous.buttons.middle)
        play(NOTES[4], 100);

    // Колесо перебирает 16 цветов палитры по кругу в обе стороны.
    if (status.wheel_delta)
    {
        _paint_color = (_paint_color + (status.wheel_delta > 0 ? 1 : COLORS - 1)) % COLORS;
        show_paint_color();
        ESP_LOGD(TAG, "Paint colour %d.", _paint_color);
    }

    const auto report = [&status](bool now, bool before, const char *name)
    {
        if (now != before)
            ESP_LOGI(TAG, "Mouse %s button %s at %d, %d.", name, now ? "pressed" : "released",
                     status.x, status.y);
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
void esp::fabgl_test::check_devices()
{
    if (!_keyboard_found)
    {
        draw_line(row_keyboard, "Keyboard: searching...");
        esp::ps2::keyboard *const keyboard = esp::ps2::ps2_controller::keyboard();
        _keyboard_found = keyboard->reset();
        // Сброс ставит раскладку US; здесь она та же, но задаётся явно.
        keyboard->set_layout(&esp::ps2::US_LAYOUT);
        ESP_LOGI(TAG, "Keyboard %s.", _keyboard_found ? "found" : "not found");
        show_keyboard();
    }

    if (!_mouse_found)
    {
        draw_line(row_mouse, "Mouse:    searching...");
        _mouse_found = esp::ps2::ps2_controller::mouse()->reset();
        if (_mouse_found)
            _display.set_mouse_cursor(CURSOR);
        ESP_LOGI(TAG, "Mouse %s.", _mouse_found ? "found" : "not found");
        show_mouse();
    }
}

/**
 * @brief Подаёт звук текущей формы волны.
 *
 * Не ждёт окончания звука: FabGL сама удаляет генератор волны, когда
 * длительность истекает.
 *
 * @param frequency Частота, Гц; для шума не важна.
 * @param duration_ms Длительность, мс.
 */
void esp::fabgl_test::play(int frequency, int duration_ms)
{
    // esp::sound::sound_generator::play_sound() — шаблон, тип генератора выбирается при компиляции,
    // поэтому форму волны выбирает переключатель.
    switch (_waveform)
    {
        case 0:
            _sound.play_sound(esp::sound::sine_waveform_generator(), frequency, duration_ms,
                              VOLUME);
            break;
        case 1:
            _sound.play_sound(esp::sound::square_waveform_generator(), frequency, duration_ms,
                              VOLUME);
            break;
        case 2:
            _sound.play_sound(esp::sound::triangle_waveform_generator(), frequency, duration_ms,
                              VOLUME);
            break;
        case 3:
            _sound.play_sound(esp::sound::sawtooth_waveform_generator(), frequency, duration_ms,
                              VOLUME);
            break;
        default:
            _sound.play_sound(esp::sound::noise_waveform_generator(), frequency, duration_ms,
                              VOLUME);
            break;
    }
}
