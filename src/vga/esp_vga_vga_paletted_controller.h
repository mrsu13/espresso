// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_native_pixel_format.h"
#include "esp_gfx_rgb222.h"
#include "esp_gfx_rgb888.h"
#include "esp_gfx_rgba8888.h"
#include "esp_vga_vga_base_controller.h"
#include "esp_vga_vga_timings.h"

#include <cstdint>

#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace esp::vga
{

class vga_paletted_controller : public vga_base_controller
{

public:

    vga_paletted_controller(int lines_count, int columns_quantum,
                            esp::gfx::native_pixel_format native_pixel_format,
                            int view_port_ratio_div, int view_port_ratio_mul,
                            intr_handler_t isr_handler);
    ~vga_paletted_controller();

    void end() override;

    void suspend_background_primitive_execution() override;

    using vga_base_controller::set_resolution;
    void set_resolution(const vga_timings &timings, int view_port_width = -1,
                        int view_port_height = -1, bool double_buffered = false) override;

    int palette_size();
    inline int colors_count() override;

    inline void set_process_primitives_on_blank(bool value);

    inline static std::uint8_t *sget_scanline(int y);

    inline esp::gfx::native_pixel_format native_pixel_format() override;

    bool suspend_double_buffering(bool value) override;

protected:

    void init() override;

    virtual void setup_default_palette() = 0;

    void update_rgb2_palette_lut();
    void calculate_available_cycles_for_drawings();
    static void primitive_exec_task(void *arg);

    inline std::uint8_t rgb888_to_palette_index(const esp::gfx::rgb888 &rgb);
    inline std::uint8_t rgb2222_to_palette_index(std::uint8_t value);
    inline std::uint8_t rgb8888_to_palette_index(esp::gfx::rgba8888 value);

    void swap_buffers() override;

    /** @brief Задача, которая рисует примитивы; @c nullptr, пока её нет. */
    TaskHandle_t _primitive_exec_task;

    /** @brief Строки DMA: их обработчик прерывания заполняет байтами пикселей. */
    volatile std::uint8_t **_lines;

    /** @brief Копия @c _view_port для обработчика прерывания: статическая переменная быстрее. */
    static volatile std::uint8_t **_isr_view_port;

    /** @brief Копия @c _view_port_visible для обработчика прерывания. */
    static volatile std::uint8_t **_isr_view_port_visible;

    /** @brief Дескриптор DMA первой строки области вывода: по нему обработчик узнаёт начало кадра. */
    static lldesc_t volatile *_frame_reset_desc;

    /** @brief Строка области вывода, с которой обработчик прерывания продолжит. */
    static volatile int _scan_line;

    /** @brief Палитра: цвета по номерам. */
    esp::gfx::rgb222 *_palette;

private:

    void allocate_view_port() override;
    void free_view_port() override;
    void check_view_port_size() override;
    void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch, int scan,
                             bool is_visible, int visible_row) override;

    /** @brief Сколько тактов процессора задача может рисовать примитивы за кадр. */
    volatile std::uint32_t _primitive_exec_timeout_cycles;

    /** @brief Рисует ли задача сейчас примитивы. */
    volatile bool _task_processing_primitives;

    /**
     * @brief Рисовать ли примитивы только во время обратного хода луча.
     *
     * @c true — медленно, но без мерцания; @c false — до половины кадра:
     * быстро, но изображение может мерцать.
     */
    bool _process_primitives_on_blank;

    /** @brief Ближайший номер палитры для каждого цвета RGB222, упакованного в 6 разрядов. */
    std::uint8_t _packed_rgb222_to_palette_index[64];

    /** @brief Число строк DMA; высота области вывода кратна ему. */
    int _lines_count;

    /** @brief Ширина области вывода кратна этому числу пикселей. */
    int _columns_quantum;

    /** @brief Формат пикселя буфера кадра. */
    esp::gfx::native_pixel_format _native_pixel_format;

    /** @brief Делитель длины строки: строка буфера — ширина / div × mul байтов. */
    int _view_port_ratio_div;

    /** @brief Множитель длины строки: строка буфера — ширина / div × mul байтов. */
    int _view_port_ratio_mul;

    /** @brief Обработчик прерывания I2S1 производного контроллера. */
    intr_handler_t _isr_handler;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает число цветов: размер палитры.
 * @return 2, 4, 8 или 16.
 */
int esp::vga::vga_paletted_controller::colors_count()
{
    return palette_size();
}

/**
 * @brief Задаёт, когда задача рисует примитивы.
 * @param value @c true — только во время обратного хода луча: медленно, но
 *        без мерцания; @c false — до половины кадра: быстро, но может мерцать.
 */
void esp::vga::vga_paletted_controller::set_process_primitives_on_blank(bool value)
{
    _process_primitives_on_blank = value;
}

/**
 * @brief Возвращает строку буфера рисования по статической копии указателей строк.
 * @param y Строка, 0 — верхняя.
 * @return Указатель на строку.
 */
std::uint8_t *esp::vga::vga_paletted_controller::sget_scanline(int y)
{
    return const_cast<std::uint8_t *>(_isr_view_port[y]);
}

/**
 * @brief Возвращает формат пикселя буфера кадра.
 * @return Формат с палитрой на 2, 4, 8 или 16 цветов.
 */
esp::gfx::native_pixel_format esp::vga::vga_paletted_controller::native_pixel_format()
{
    return _native_pixel_format;
}

// Защищённые

/**
 * @brief Возвращает номер цвета палитры, ближайшего к цвету RGB888.
 * @param rgb Цвет.
 * @return Номер цвета палитры.
 */
std::uint8_t esp::vga::vga_paletted_controller::rgb888_to_palette_index(const esp::gfx::rgb888 &rgb)
{
    return _packed_rgb222_to_palette_index[esp::gfx::rgb888_to_packed_rgb222(rgb)];
}

/**
 * @brief Возвращает номер цвета палитры, ближайшего к цвету RGBA2222.
 * @param value Цвет; разряды прозрачности не учитываются.
 * @return Номер цвета палитры.
 */
std::uint8_t esp::vga::vga_paletted_controller::rgb2222_to_palette_index(std::uint8_t value)
{
    return _packed_rgb222_to_palette_index[value & 0b00111111];
}

/**
 * @brief Возвращает номер цвета палитры, ближайшего к цвету RGBA8888.
 * @param value Цвет; прозрачность не учитывается.
 * @return Номер цвета палитры.
 */
std::uint8_t esp::vga::vga_paletted_controller::rgb8888_to_palette_index(esp::gfx::rgba8888 value)
{
    return rgb888_to_palette_index(esp::gfx::rgb888(value.r, value.g, value.b));
}
