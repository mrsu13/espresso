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
 * Unicode Terminus 10 × 18 (esp::font, компонент consolefonts); разметка
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

/** @brief Число цветов палитры VGA16Controller. */
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
    ROW_KEYBOARD, /**< Клавиатура и её раскладка. */
    ROW_KEY, /**< Последняя нажатая или отпущенная клавиша. */
    ROW_INPUT, /**< Набранная строка. */
    ROW_MOUSE, /**< Положение и кнопки мыши. */
    ROW_BUTTON, /**< Кнопка K1. */
    ROW_SOUND, /**< Форма звуковой волны. */
    ROW_MEMORY, /**< Свободная память. */
    ROW_SAMPLES, /**< Первая строка образцов текста SAMPLES. */
    ROW_HELP = ROW_SAMPLES
        + static_cast<int>(std::size(SAMPLES)), /**< Подсказка о мыши и образец цвета рисования. */
    ROWS /**< Число строк. */
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
constexpr fabgl::CursorName CURSOR = fabgl::CursorPointerShadowed;

/**
 * @brief Возвращает цвет палитры по номеру.
 * @param index Номер цвета, от 0 до 15.
 * @return Цвет палитры.
 */
fabgl::Color color(int index)
{
    return static_cast<fabgl::Color>(index);
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
    , _paint_color(fabgl::BrightYellow)
    , _presses(0)
    , _keyboard_found(false)
    , _mouse_found(false)
{
    // По два бита на каждую составляющую цвета; контроллер выбирает из
    // этих 64 цветов палитру из 16. Кадр — 4 бита на точку, 150 КБ памяти.
    _display.begin();
    _display.setResolution(VGA_MODE);
    ESP_LOGI(TAG, "VGA %dx%d, %d colours.", _canvas.getWidth(), _canvas.getHeight(), COLORS);

    // Знаки шрифта заливают фон знакоместа цветом кисти, поэтому новый
    // текст закрывает прежний.
    _canvas.setGlyphOptions(fabgl::GlyphOptions().FillBackground(true));

    // FabGL ищет устройства только здесь и при их сбросе (check_devices()):
    // клавиатуру и мышь лучше подключить до включения платы.
    fabgl::PS2Controller::begin(fabgl::PS2Preset::KeyboardPort0_MousePort1);
    fabgl::PS2Controller::keyboard()->setLayout(&fabgl::USLayout);
    _keyboard_found = fabgl::PS2Controller::keyboard()->isKeyboardAvailable();

    // Указатель мыши двигает сама FabGL, а новые состояния мыши она кладёт
    // в очередь, которую читает poll(). Указатель начинает с центра экрана.
    fabgl::Mouse *const mouse = fabgl::PS2Controller::mouse();
    mouse->setupAbsolutePositioner(_canvas.getWidth(), _canvas.getHeight(), true, &_display);
    _mouse = mouse->status();
    _mouse_found = mouse->isMouseAvailable();
    if (_mouse_found)
        _display.setMouseCursor(CURSOR);

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
    fabgl::Keyboard *const keyboard = fabgl::PS2Controller::keyboard();
    fabgl::VirtualKeyItem item;
    while (keyboard->getNextVirtualKey(&item, 0))
        on_key(item);

    // При быстром движении состояний мыши приходит много: рисуются все,
    // а строка состояния обновляется один раз.
    fabgl::Mouse *const mouse = fabgl::PS2Controller::mouse();
    bool moved = false;
    while (mouse->availableStatus())
    {
        on_mouse(mouse->getNextStatus(0));
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
    return status_top() + ROWS * _font.height() + 6;
}

/**
 * @brief Рисует проверочный экран целиком.
 */
void esp::fabgl_test::draw_screen()
{
    const int width = _canvas.getWidth();
    const int height = _canvas.getHeight();
    const int line = _font.height();

    _canvas.setBrushColor(fabgl::Black);
    _canvas.clear();

    // Рамка по самому краю изображения: по ней видно, помещается ли
    // изображение на экране монитора целиком и не сдвинуто ли оно.
    _canvas.setPenColor(fabgl::BrightWhite);
    _canvas.drawRectangle(0, 0, width - 1, height - 1);

    // Заголовок: белый текст на синем; справа — режим (в строке только
    // знаки ASCII, поэтому число знаков равно её длине).
    _canvas.setBrushColor(fabgl::Blue);
    _canvas.fillRectangle(1, 1, width - 2, TITLE_TOP + line);
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
        _canvas.setBrushColor(color(i));
        _canvas.fillRectangle(x, bars_top, x + bar_width - 2, bars_top + BAR_HEIGHT - 1);

        char number[4];
        std::snprintf(number, sizeof(number), "%d", i);
        _canvas.setPenColor(fabgl::White);
        _canvas.setBrushColor(fabgl::Black);
        _font.draw_text(_canvas,
                        x + (bar_width - _font.width() * static_cast<int>(std::strlen(number))) / 2,
                        bars_top + BAR_HEIGHT + 2, number);
    }

    show_keyboard();
    draw_line(ROW_KEY, "Key:      none yet");
    show_input();
    show_mouse();
    show_button();
    show_sound();
    show_memory();
    for (size_t i = 0; i < std::size(SAMPLES); ++i)
        draw_line(ROW_SAMPLES + static_cast<int>(i), SAMPLES[i]);
    draw_line(ROW_HELP, "Mouse: left draws, right clears, wheel changes the colour.");
    show_paint_color();

    _canvas.setPenColor(fabgl::White);
    _canvas.drawRectangle(MARGIN, paint_top(), width - MARGIN - 1, height - MARGIN - 1);
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
    const int right = _canvas.getWidth() - MARGIN - 1;

    // Отсечение по правому полю: при широком знакоместе длинная строка
    // иначе залезла бы на рамку по краю изображения.
    _canvas.setClippingRect(fabgl::Rect(MARGIN, y, right, y + line - 1));
    _canvas.setPenColor(fabgl::BrightWhite);
    _canvas.setBrushColor(fabgl::Black);
    const int cells = _font.draw_text(_canvas, MARGIN, y, text);

    // Число знаков в UTF-8 не равно числу байтов, поэтому хвост строки
    // стирается прямоугольником, а не дополнением пробелами.
    const int x = MARGIN + cells * _font.width();
    if (x <= right)
        _canvas.fillRectangle(x, y, right, y + line - 1);
    _canvas.setClippingRect(fabgl::Rect(0, 0, _canvas.getWidth() - 1, _canvas.getHeight() - 1));
}

/**
 * @brief Показывает, найдена ли клавиатура, и её раскладку.
 */
void esp::fabgl_test::show_keyboard()
{
    char text[96];
    if (_keyboard_found)
        std::snprintf(text, sizeof(text), "Keyboard: found, layout %s",
                      fabgl::PS2Controller::keyboard()->getLayout()->desc);
    else
        std::snprintf(text, sizeof(text), "%s", "Keyboard: not found; connect it and press K1");
    draw_line(ROW_KEYBOARD, text);
}

/**
 * @brief Показывает последнюю нажатую или отпущенную клавишу.
 * @param item Событие клавиатуры.
 */
void esp::fabgl_test::show_key(const fabgl::VirtualKeyItem &item)
{
    char text[96];
    std::snprintf(text, sizeof(text), "Key:      %s %s, ASCII 0x%02x",
                  fabgl::Keyboard::virtualKeyToString(item.vk), item.down ? "down" : "up",
                  item.ASCII);
    draw_line(ROW_KEY, text);
}

/**
 * @brief Показывает набранную строку.
 */
void esp::fabgl_test::show_input()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Input:    %s_", _input.c_str());
    draw_line(ROW_INPUT, text);
}

/**
 * @brief Показывает положение и кнопки мыши или её отсутствие.
 */
void esp::fabgl_test::show_mouse()
{
    char text[96];
    if (_mouse_found)
        std::snprintf(text, sizeof(text), "Mouse:    X %3d, Y %3d, buttons %c%c%c", _mouse.X,
                      _mouse.Y, _mouse.buttons.left ? 'L' : '-', _mouse.buttons.middle ? 'M' : '-',
                      _mouse.buttons.right ? 'R' : '-');
    else
        std::snprintf(text, sizeof(text), "%s", "Mouse:    not found; connect it and press K1");
    draw_line(ROW_MOUSE, text);
}

/**
 * @brief Показывает число нажатий кнопки K1.
 */
void esp::fabgl_test::show_button()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Button:   K1 (GPIO36), presses: %d", _presses);
    draw_line(ROW_BUTTON, text);
}

/**
 * @brief Показывает форму звуковой волны и подсказку о клавишах звука.
 */
void esp::fabgl_test::show_sound()
{
    char text[96];
    std::snprintf(text, sizeof(text), "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
                  WAVEFORMS[_waveform]);
    draw_line(ROW_SOUND, text);
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
    draw_line(ROW_MEMORY, text);
}

/**
 * @brief Показывает образец цвета рисования в конце строки подсказки.
 */
void esp::fabgl_test::show_paint_color()
{
    const int right = _canvas.getWidth() - MARGIN - 1;
    const int top = status_top() + ROW_HELP * _font.height() + 1;
    const int bottom = top + _font.height() - 3;

    _canvas.setPenColor(fabgl::White);
    _canvas.setBrushColor(color(_paint_color));
    _canvas.fillRectangle(right - 31, top, right, bottom);
    _canvas.drawRectangle(right - 31, top, right, bottom);
}

/**
 * @brief Очищает поле для рисования, не трогая его рамку.
 */
void esp::fabgl_test::clear_paint_area()
{
    _canvas.setBrushColor(fabgl::Black);
    _canvas.fillRectangle(MARGIN + 1, paint_top() + 1, _canvas.getWidth() - MARGIN - 2,
                          _canvas.getHeight() - MARGIN - 2);
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
    const int width = _canvas.getWidth();
    const int height = _canvas.getHeight();

    // Отсечение по внутренней части поля: рамка и строки над полем
    // остаются нетронутыми. Затем отсечение и толщина линии
    // возвращаются к обычным.
    _canvas.setClippingRect(
        fabgl::Rect(MARGIN + 1, paint_top() + 1, width - MARGIN - 2, height - MARGIN - 2));
    _canvas.setPenColor(color(_paint_color));
    _canvas.setPenWidth(PAINT_PEN_WIDTH);
    _canvas.drawLine(x1, y1, x2, y2);
    _canvas.setPenWidth(1);
    _canvas.setClippingRect(fabgl::Rect(0, 0, width - 1, height - 1));
}

/**
 * @brief Обрабатывает событие клавиатуры.
 *
 * Нажатия выводятся в консоль, отпускания — только в отладочный журнал.
 *
 * @param item Событие клавиатуры.
 */
void esp::fabgl_test::on_key(const fabgl::VirtualKeyItem &item)
{
    show_key(item);

    const char *const name = fabgl::Keyboard::virtualKeyToString(item.vk);
    if (!item.down)
    {
        ESP_LOGD(TAG, "Key %s up.", name);
        return;
    }

    ESP_LOGI(TAG, "Key %s down, ASCII 0x%02x.", name, item.ASCII);

    // Коды F1–F8 в перечислении VirtualKey идут подряд.
    if (item.vk >= fabgl::VK_F1 && item.vk <= fabgl::VK_F8)
    {
        play(NOTES[item.vk - fabgl::VK_F1], NOTE_MS);
        return;
    }

    switch (item.vk)
    {
        case fabgl::VK_F9:
            _waveform = (_waveform + 1) % static_cast<int>(std::size(WAVEFORMS));
            show_sound();
            ESP_LOGI(TAG, "Waveform %s.", WAVEFORMS[_waveform]);
            play(NOTES[0], NOTE_MS);
            return;

        case fabgl::VK_ESCAPE:
            _input.clear();
            show_input();
            return;

        case fabgl::VK_BACKSPACE:
            if (!_input.empty())
                _input.pop_back();
            show_input();
            return;

        case fabgl::VK_RETURN:
        case fabgl::VK_KP_ENTER:
            ESP_LOGI(TAG, "Input: \"%s\".", _input.c_str());
            _input.clear();
            show_input();
            return;

        default:
            break;
    }

    // В строку попадают только печатные символы ASCII; FabGL уже учла
    // Shift, Caps Lock и раскладку.
    if (item.ASCII >= 0x20 && item.ASCII < 0x7f && _input.size() < INPUT_MAX)
    {
        _input += static_cast<char>(item.ASCII);
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
void esp::fabgl_test::on_mouse(const fabgl::MouseStatus &status)
{
    const fabgl::MouseStatus previous = _mouse;
    _mouse = status;

    // Линия от прежнего положения делает след сплошным и при быстром
    // движении; в момент нажатия рисуется точка.
    if (status.buttons.left)
    {
        if (previous.buttons.left)
            paint(previous.X, previous.Y, status.X, status.Y);
        else
            paint(status.X, status.Y, status.X, status.Y);
    }

    if (status.buttons.right && !previous.buttons.right)
        clear_paint_area();

    if (status.buttons.middle && !previous.buttons.middle)
        play(NOTES[4], 100);

    // Колесо перебирает 16 цветов палитры по кругу в обе стороны.
    if (status.wheelDelta)
    {
        _paint_color = (_paint_color + (status.wheelDelta > 0 ? 1 : COLORS - 1)) % COLORS;
        show_paint_color();
        ESP_LOGD(TAG, "Paint colour %d.", _paint_color);
    }

    const auto report = [&status](bool now, bool before, const char *name)
    {
        if (now != before)
            ESP_LOGI(TAG, "Mouse %s button %s at %d, %d.", name, now ? "pressed" : "released",
                     status.X, status.Y);
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
        draw_line(ROW_KEYBOARD, "Keyboard: searching...");
        fabgl::Keyboard *const keyboard = fabgl::PS2Controller::keyboard();
        _keyboard_found = keyboard->reset();
        // Сброс ставит раскладку US; здесь она та же, но задаётся явно.
        keyboard->setLayout(&fabgl::USLayout);
        ESP_LOGI(TAG, "Keyboard %s.", _keyboard_found ? "found" : "not found");
        show_keyboard();
    }

    if (!_mouse_found)
    {
        draw_line(ROW_MOUSE, "Mouse:    searching...");
        _mouse_found = fabgl::PS2Controller::mouse()->reset();
        if (_mouse_found)
            _display.setMouseCursor(CURSOR);
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
    // playSound() — шаблон, тип генератора выбирается при компиляции,
    // поэтому форму волны выбирает переключатель.
    switch (_waveform)
    {
        case 0:
            _sound.playSound(fabgl::SineWaveformGenerator(), frequency, duration_ms, VOLUME);
            break;
        case 1:
            _sound.playSound(fabgl::SquareWaveformGenerator(), frequency, duration_ms, VOLUME);
            break;
        case 2:
            _sound.playSound(fabgl::TriangleWaveformGenerator(), frequency, duration_ms, VOLUME);
            break;
        case 3:
            _sound.playSound(fabgl::SawtoothWaveformGenerator(), frequency, duration_ms, VOLUME);
            break;
        default:
            _sound.playSound(fabgl::NoiseWaveformGenerator(), frequency, duration_ms, VOLUME);
            break;
    }
}
