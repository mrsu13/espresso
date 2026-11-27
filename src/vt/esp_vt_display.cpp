/**
 * @file esp_vt_display.h
 * @brief Объявление вывода терминала на VGA esp::vt::display.
 */

/**
 * @file esp_vt_display.cpp
 * @brief Вывод терминала на VGA: видеорежимы, палитры, холст и шрифт.
 */

#include "esp_vt_display.h"

#include "esp_gfx_color.h"
#include "esp_vga_modelines.h"
#include "esp_vt_config.h"
#include "ter_10x18.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include <esp_log.h>


/**
 * @class esp::vt::display
 * @brief Вывод терминала на VGA: видеоконтроллер, его холст и шрифт знакомест.
 *
 * Видеорежимы терминала — те, что проверены на плате: 640 × 480 с 16, 8, 4
 * и 2 цветами, 800 × 600 с 4 и 2 цветами, 1024 × 768 с 2 цветами в двух
 * развёртках. Палитра видеорежима: при 16 и 8 цветах — цвета ANSI, при 4 —
 * чёрный фон, светло-серый текст, жёлтый для выделения и красный для
 * ошибок, при 2 — чёрный и светло-серый.
 *
 * Знакоместо — знак шрифта Terminus 10 × 18, поэтому экран при 640 × 480 —
 * 64 × 26 знакомест, а под последней строкой остаются 12 неиспользуемых
 * точек.
 */


namespace esp::vt::p
{

/** @brief Видеоконтроллер, которым выводится видеорежим терминала. */
enum class driver
{
    /** @brief esp::vga::vga16_controller: 16 цветов. */
    vga16,

    /** @brief esp::vga::vga8_controller: 8 цветов. */
    vga8,

    /** @brief esp::vga::vga4_controller: 4 цвета. */
    vga4,

    /** @brief esp::vga::vga2_controller: 2 цвета. */
    vga2,
};

/** @brief Видеорежим терминала. */
struct video_mode
{
    /** @brief Видеоконтроллер. */
    esp::vt::p::driver driver;

    /** @brief Модлайн развёртки (esp_vga_modelines.h). */
    const char *modeline;

    /** @brief Название для списка видеорежимов. */
    const char *label;
};

/** @brief Видеорежимы терминала; номер видеорежима — номер в этой таблице. */
constexpr esp::vt::p::video_mode VIDEO_MODES[] = {
    {esp::vt::p::driver::vga16, esp::vga::VGA_640X480_60HZ, "640x480, 16 colours"},
    {esp::vt::p::driver::vga8, esp::vga::VGA_640X480_60HZ, "640x480, 8 colours"},
    {esp::vt::p::driver::vga4, esp::vga::VGA_640X480_60HZ, "640x480, 4 colours"},
    {esp::vt::p::driver::vga2, esp::vga::VGA_640X480_60HZ, "640x480, 2 colours"},
    {esp::vt::p::driver::vga4, esp::vga::SVGA_800X600_60HZ, "800x600, 4 colours"},
    {esp::vt::p::driver::vga2, esp::vga::SVGA_800X600_60HZ, "800x600, 2 colours"},
    {esp::vt::p::driver::vga2, esp::vga::SVGA_1024X768_60HZ, "1024x768, 2 colours"},
    {esp::vt::p::driver::vga2, esp::vga::SVGA_1024X768_60HZ_RB,
     "1024x768 reduced blanking, 2 colours"},
};

/**
 * @brief Переводит цвет 0xRRGGBB в esp::gfx::rgb888.
 * @param rgb Цвет.
 * @return Тот же цвет.
 */
constexpr esp::gfx::rgb888 to_rgb888(std::uint32_t rgb)
{
    return esp::gfx::rgb888(static_cast<std::uint8_t>(rgb >> 16),
                            static_cast<std::uint8_t>(rgb >> 8), static_cast<std::uint8_t>(rgb));
}

/** @brief Палитра 16 и 8 цветов: цвета ANSI по порядку; 8-цветным режимам хватает первых восьми. */
constexpr std::array<esp::gfx::rgb888, 16> ANSI_PALETTE{
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[0]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[1]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[2]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[3]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[4]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[5]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[6]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[7]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[8]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[9]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[10]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[11]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[12]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[13]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[14]),
    esp::vt::p::to_rgb888(esp::vt::ANSI_COLORS[15]),
};

/** @brief Палитра 4 цветов: чёрный фон, светло-серый текст, жёлтый для выделения, красный для ошибок. */
constexpr std::array<esp::gfx::rgb888, 4> PALETTE_4{
    esp::gfx::rgb888(0x00, 0x00, 0x00),
    esp::gfx::rgb888(0xAA, 0xAA, 0xAA),
    esp::gfx::rgb888(0xFF, 0xFF, 0x55),
    esp::gfx::rgb888(0xAA, 0x00, 0x00),
};

/** @brief Палитра 2 цветов: чёрный фон и светло-серый текст. */
constexpr std::array<esp::gfx::rgb888, 2> PALETTE_2{
    esp::gfx::rgb888(0x00, 0x00, 0x00),
    esp::gfx::rgb888(0xAA, 0xAA, 0xAA),
};

/** @brief Тег журнала модуля. */
const char *const TAG = "vt";

/**
 * @brief Возвращает палитру видеоконтроллера.
 * @param driver Видеоконтроллер.
 * @return Цвета палитры по порядку.
 */
std::span<const esp::gfx::rgb888> palette_for(esp::vt::p::driver driver)
{
    switch (driver)
    {
        case esp::vt::p::driver::vga16:
            return esp::vt::p::ANSI_PALETTE;

        case esp::vt::p::driver::vga8:
            return std::span<const esp::gfx::rgb888>{esp::vt::p::ANSI_PALETTE}.first(8);

        case esp::vt::p::driver::vga4:
            return esp::vt::p::PALETTE_4;

        case esp::vt::p::driver::vga2:
            break;
    }
    return esp::vt::p::PALETTE_2;
}

}


// Открытые

/**
 * @brief Создаёт вывод с выключенным видеорежимом.
 */
esp::vt::display::display()
    : _controller{}
    , _base{nullptr}
    , _canvas{}
    , _font{ter_10x18::WIDTH,  ter_10x18::HEIGHT, ter_10x18::CODEPOINTS,
            ter_10x18::GLYPHS, ter_10x18::COUNT,  ter_10x18::FALLBACK}
    , _mode{-1}
{
}

/**
 * @brief Останавливает вывод: видеоконтроллер сам вывод при уничтожении не останавливает.
 */
esp::vt::display::~display()
{
    stop();
}

/**
 * @brief Включает видеорежим.
 *
 * Прежний видеоконтроллер останавливается и уничтожается, новый
 * получает палитру видеорежима, а его холст очищается чёрным. Буфер
 * кадра оставляет в куче свободными @p heap_reserve байтов; если для
 * всего изображения памяти сверх них не хватает, изображение ниже
 * (esp::vga::vga_base_controller::set_heap_reserve()).
 *
 * @param mode Номер видеорежима, от 0 до mode_count() − 1.
 * @param heap_reserve Сколько байтов кучи оставить свободными.
 */
void esp::vt::display::set_mode(int mode, std::size_t heap_reserve)
{
    assert(mode >= 0 && mode < mode_count());
    stop();

    const esp::vt::p::video_mode &video_mode = esp::vt::p::VIDEO_MODES[mode];
    switch (video_mode.driver)
    {
        case esp::vt::p::driver::vga16:
            start<esp::vga::vga16_controller>(video_mode.modeline, heap_reserve,
                                              esp::vt::p::palette_for(video_mode.driver));
            break;

        case esp::vt::p::driver::vga8:
            start<esp::vga::vga8_controller>(video_mode.modeline, heap_reserve,
                                             esp::vt::p::palette_for(video_mode.driver));
            break;

        case esp::vt::p::driver::vga4:
            start<esp::vga::vga4_controller>(video_mode.modeline, heap_reserve,
                                             esp::vt::p::palette_for(video_mode.driver));
            break;

        case esp::vt::p::driver::vga2:
            start<esp::vga::vga2_controller>(video_mode.modeline, heap_reserve,
                                             esp::vt::p::palette_for(video_mode.driver));
            break;
    }
    _mode = mode;

    _canvas.emplace(_base);
    _canvas->set_brush_color(esp::gfx::color::black);
    _canvas->clear();

    ESP_LOGD(esp::vt::p::TAG, "VGA %s: %dx%d, %d colours.", video_mode.label, _canvas->width(),
             _canvas->height(), _base->colors_count());
}

/**
 * @brief Останавливает вывод и освобождает видеоконтроллер и его буфер кадра.
 *
 * Если вывод уже выключен, ничего не делает.
 */
void esp::vt::display::stop()
{
    if (!_base)
        return;
    _base->end();
    // Холст не должен пережить свой видеоконтроллер.
    _canvas.reset();
    _controller.emplace<std::monostate>();
    _base = nullptr;
    _mode = -1;
}

/**
 * @brief Возвращает ширину экрана в знакоместах.
 * @return Число столбцов; вывод должен быть включён.
 */
int esp::vt::display::columns() const
{
    return _canvas->width() / _font.width();
}

/**
 * @brief Возвращает высоту экрана в знакоместах.
 *
 * Не больше @c esp::vt::MAX_ROWS. Если памяти на буфер кадра не хватило,
 * видеоконтроллер уменьшает высоту изображения, и строк становится меньше.
 *
 * @return Число строк; вывод должен быть включён.
 */
int esp::vt::display::rows() const
{
    return std::min(_canvas->height() / _font.height(), esp::vt::MAX_ROWS);
}

/**
 * @brief Возвращает размер сетки знакомест видеорежима во всю его высоту.
 *
 * Столько знакомест будет на экране, если буфер кадра поместится
 * целиком; вывод для этого включать не нужно.
 *
 * @param mode Номер видеорежима, от 0 до mode_count() − 1.
 * @return Ширина — столбцы, высота — строки, не больше @c esp::vt::MAX_ROWS.
 */
esp::gfx::size esp::vt::display::grid_size(int mode) const
{
    assert(mode >= 0 && mode < mode_count());
    esp::vga::vga_timings timings{};
    const bool is_parsed = esp::vga::vga_base_controller::convert_modeline_to_timings(
        esp::vt::p::VIDEO_MODES[mode].modeline, &timings);
    assert(is_parsed);
    (void)is_parsed;
    return esp::gfx::size{timings.h_visible_area / _font.width(),
                          std::min(timings.v_visible_area / _font.height(), esp::vt::MAX_ROWS)};
}

/**
 * @brief Возвращает палитру текущего видеорежима.
 * @return Цвета палитры по порядку; вывод должен быть включён.
 */
std::span<const esp::gfx::rgb888> esp::vt::display::palette() const
{
    return esp::vt::p::palette_for(esp::vt::p::VIDEO_MODES[_mode].driver);
}

/**
 * @brief Возвращает число видеорежимов терминала.
 * @return Сколько видеорежимов в таблице.
 */
int esp::vt::display::mode_count()
{
    return static_cast<int>(std::size(esp::vt::p::VIDEO_MODES));
}

/**
 * @brief Возвращает название видеорежима.
 * @param mode Номер видеорежима, от 0 до mode_count() − 1.
 * @return Название, например «640x480, 4 colours».
 */
const char *esp::vt::display::mode_label(int mode)
{
    return esp::vt::p::VIDEO_MODES[mode].label;
}


// Закрытые

/**
 * @brief Создаёт и запускает видеоконтроллер и задаёт ему палитру.
 * @tparam T Класс видеоконтроллера с палитрой.
 * @param modeline Модлайн развёртки.
 * @param palette Цвета палитры по порядку.
 */
template<typename T>
void esp::vt::display::start(const char *modeline, std::size_t heap_reserve,
                             std::span<const esp::gfx::rgb888> palette)
{
    T &controller = _controller.emplace<T>();
    controller.begin();
    controller.set_heap_reserve(heap_reserve);
    controller.set_resolution(modeline);
    for (std::size_t index = 0; index < palette.size(); ++index)
        controller.set_palette_item(static_cast<int>(index), palette[index]);
    _base = &controller;
}
