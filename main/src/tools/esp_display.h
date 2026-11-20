#pragma once

#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <cstdint>


namespace esp
{

class display
{

public:

    /** @brief Ширина экрана в точках. */
    static constexpr int WIDTH = 135;

    /** @brief Высота экрана в точках. */
    static constexpr int HEIGHT = 240;

    /** @brief Чёрный цвет в формате RGB565. */
    static constexpr std::uint16_t BLACK = 0x0000;

    /** @brief Белый цвет в формате RGB565. */
    static constexpr std::uint16_t WHITE = 0xFFFF;

    display();
    ~display();

    bool fill(std::uint16_t color);

    inline bool ready() const;

private:

    bool init();
    void release();

    /** @brief Интерфейс панели: передача команд и данных по SPI. */
    esp_lcd_panel_io_handle_t _io;

    /** @brief Панель ST7789; @c nullptr, если инициализация не удалась. */
    esp_lcd_panel_handle_t _panel;

    /** @brief Счётный семафор: отдаётся по окончании передачи каждой полосы. */
    SemaphoreHandle_t _sent;

    /** @brief Буфер DMA для одной полосы строк. */
    std::uint16_t *_band;

    /** @brief @c true, если шину SPI инициализировал этот объект. */
    bool _bus;
};

}


// Реализация

/**
 * @brief Проверяет, инициализирован ли дисплей.
 * @return @c true, если дисплей готов к работе.
 */
bool esp::display::ready() const
{
    return _panel;
}
