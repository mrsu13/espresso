#pragma once

#include "esp_gfx_canvas.h"
#include "esp_gfx_font.h"
#include "esp_gfx_rgb888.h"
#include "esp_non_copyable.h"
#include "esp_vga_vga16_controller.h"
#include "esp_vga_vga2_controller.h"
#include "esp_vga_vga4_controller.h"
#include "esp_vga_vga8_controller.h"
#include "esp_vga_vga_base_controller.h"

#include <optional>
#include <span>
#include <variant>


namespace esp::vt
{

class display : esp::non_copyable
{

public:

    display();
    ~display();

    void set_mode(int mode);
    void stop();

    inline int mode() const;
    inline bool is_on() const;
    inline esp::gfx::canvas &canvas();
    inline const esp::gfx::font &font() const;
    int columns() const;
    int rows() const;
    std::span<const esp::gfx::rgb888> palette() const;

    static int mode_count();
    static const char *mode_label(int mode);

private:

    template<typename T>
    void start(const char *modeline, std::span<const esp::gfx::rgb888> palette);

    /**
     * @brief Видеоконтроллер текущего видеорежима; @c std::monostate — вывод выключен.
     *
     * Существует только один видеоконтроллер: при смене видеорежима прежний
     * останавливается и уничтожается, и лишь потом создаётся новый — двум
     * буферам кадра памяти не хватит.
     */
    std::variant<std::monostate, esp::vga::vga16_controller, esp::vga::vga8_controller,
                 esp::vga::vga4_controller, esp::vga::vga2_controller>
        _controller;

    /** @brief Тот же видеоконтроллер через его основу; @c nullptr, когда вывод выключен. */
    esp::vga::vga_base_controller *_base;

    /** @brief Холст видеоконтроллера; создаётся после него и уничтожается до него. */
    std::optional<esp::gfx::canvas> _canvas;

    /** @brief Шрифт знакомест — Unicode Terminus 10 × 18. */
    esp::gfx::font _font;

    /** @brief Номер текущего видеорежима в таблице; @c -1 — вывод выключен. */
    int _mode;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает номер текущего видеорежима.
 * @return Номер в таблице видеорежимов или @c -1, если вывод выключен.
 */
int esp::vt::display::mode() const
{
    return _mode;
}

/**
 * @brief Сообщает, включён ли вывод.
 * @return @c true, если видеорежим задан и не остановлен.
 */
bool esp::vt::display::is_on() const
{
    return _base != nullptr;
}

/**
 * @brief Возвращает холст текущего видеорежима.
 *
 * Действителен до смены видеорежима или остановки вывода.
 *
 * @return Холст; вывод должен быть включён.
 */
esp::gfx::canvas &esp::vt::display::canvas()
{
    return *_canvas;
}

/**
 * @brief Возвращает шрифт знакомест.
 * @return Шрифт.
 */
const esp::gfx::font &esp::vt::display::font() const
{
    return _font;
}
