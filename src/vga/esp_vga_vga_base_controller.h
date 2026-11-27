// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_attributes.h"
#include "esp_gfx_generic_bitmapped_display_controller.h"
#include "esp_gfx_rgb222.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_vga_timings.h"

#include <cstdint>

#include <driver/gpio.h>
#include <esp_intr_alloc.h>


namespace esp::vga
{

class vga_base_controller : public esp::gfx::generic_bitmapped_display_controller
{

public:

    /** @brief Первый разряд красного канала в байте пикселя; второй — следующий. */
    static constexpr int RED_BIT = 0;

    /** @brief Первый разряд зелёного канала в байте пикселя; второй — следующий. */
    static constexpr int GREEN_BIT = 2;

    /** @brief Первый разряд синего канала в байте пикселя; второй — следующий. */
    static constexpr int BLUE_BIT = 4;

    /** @brief Разряд строчного синхроимпульса в байте пикселя. */
    static constexpr int HSYNC_BIT = 6;

    /** @brief Разряд кадрового синхроимпульса в байте пикселя. */
    static constexpr int VSYNC_BIT = 7;

    /** @brief Маска разрядов синхроимпульсов в байте пикселя. */
    static constexpr int SYNC_MASK = (1 << HSYNC_BIT) | (1 << VSYNC_BIT);

    vga_base_controller();

    void begin(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
               gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio);
    void begin(gpio_num_t red_1gpio, gpio_num_t red_0gpio, gpio_num_t green_1gpio,
               gpio_num_t green_0gpio, gpio_num_t blue_1gpio, gpio_num_t blue_0gpio,
               gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio);
    void begin() override;
    virtual void end();

    static bool convert_modeline_to_timings(const char *modeline, vga_timings *timings);

    void suspend_background_primitive_execution() override;
    void resume_background_primitive_execution() override;

    void set_resolution(const char *modeline, int view_port_width = -1, int view_port_height = -1,
                        bool double_buffered = false) override;
    virtual void set_resolution(const vga_timings &timings, int view_port_width = -1,
                                int view_port_height = -1, bool double_buffered = false);

    void set_heap_reserve(std::size_t bytes);

    inline int view_port_col() const;
    inline int view_port_row() const;
    void move_screen(int offset_x, int offset_y);
    void shrink_screen(int shrink_x, int shrink_y);
    inline vga_timings *resolution_timings();

    inline std::uint8_t bits_per_channel() const;
    inline int colors_count() override;

    inline std::uint8_t *scanline(int y);
    inline std::uint8_t create_raw_pixel(esp::gfx::rgb222 rgb);
    inline std::uint8_t create_blank_raw_pixel() const;

    bool suspend_double_buffering(bool value) override;

protected:

    static void setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode);

    void start_gpio_stream();

    void free_buffers();
    virtual void free_view_port();

    virtual void init();

    bool set_dma_buffers_count(int buffers_count);

    std::uint8_t pack_hv_sync(bool h_sync, bool v_sync);
    inline ESP_ALWAYS_INLINE_ATTR std::uint8_t prepare_pixel(esp::gfx::rgb222 rgb);
    std::uint8_t prepare_pixel_with_sync(esp::gfx::rgb222 rgb, bool h_sync, bool v_sync);

    void fill_vert_buffers(int offset_y);
    void fill_horiz_buffers(int offset_x);
    int fill(std::uint8_t volatile *buffer, int start_pos, int length, std::uint8_t red,
             std::uint8_t green, std::uint8_t blue, bool h_sync, bool v_sync);

    int calc_required_dma_buffers_count(int view_port_height);
    bool is_multi_scan_black_line(int scan);

    void set_dma_buffer_blank(int index, void volatile *address, int length, int scan,
                              bool is_start_of_vert_front_porch);
    void set_dma_buffer_view(int index, int row, int scan, volatile std::uint8_t **view_port,
                             bool on_visible_dma);
    void set_dma_buffer_view(int index, int row, int scan, bool is_start_of_vert_front_porch);

    virtual void on_setup_dma_buffer(lldesc_t volatile *buffer, bool is_start_of_vert_front_porch,
                                     int scan, bool is_visible, int visible_row) = 0;

    void volatile *get_dma_buffer(int index, int *length);

    void allocate_view_port(std::uint32_t alloc_caps, int rowlen);
    virtual void allocate_view_port() = 0;
    inline virtual void check_view_port_size();

    void swap_buffers() override;

    template<typename T>
    inline static ESP_ALWAYS_INLINE_ATTR T &pixel_in_row(T *row, int x);
    inline ESP_ALWAYS_INLINE_ATTR volatile std::uint8_t &pixel(int x, int y);
    inline ESP_ALWAYS_INLINE_ATTR void invert_pixel(int x, int y);

    /**
     * @brief Строки буфера рисования: указатель на начало каждой строки.
     *
     * Без двойной буферизации совпадает с @c _view_port_visible.
     */
    volatile std::uint8_t **_view_port;

    /** @brief Строки видимого буфера, который выводит DMA. */
    volatile std::uint8_t **_view_port_visible;

    /** @brief Оба буфера кадра: на них указывают @c _view_port и @c _view_port_visible. */
    volatile std::uint8_t **_view_ports[2];

    /** @brief Двойная буферизация сменой цепочек DMA, а не указателей строк. */
    bool _double_buffer_over_dma;

    /** @brief Сколько раз приостановлено выполнение примитивов; 0 — выполняются. */
    volatile int _primitive_processing_suspended;

    /** @brief Обработчик прерывания контроллера; @c nullptr, если не установлен. */
    intr_handle_t _isr_handle;

    /** @brief Текущая развёртка. */
    vga_timings _timings;

    /** @brief Полная длина строки со всеми гасящими интервалами, пиксели. */
    std::int16_t _h_line_size;

    /** @brief Левый столбец области вывода на экране; кратен 4. */
    volatile std::int16_t _view_port_col;

    /** @brief Верхняя строка области вывода на экране; кратна 4. */
    volatile std::int16_t _view_port_row;

    /** @brief Разряды синхроимпульсов видимой строки: их ИЛИ с цветом даёт байт пикселя. */
    volatile std::uint8_t _hv_sync;

private:

    /** @brief Разрядов на канал цвета: 1 — 8 цветов, 2 — 64; задаёт begin(). */
    int _bits_per_channel;

    /** @brief Поток DMA → GPIO, который выводит кадр. */
    gpio_stream _gpio_stream;

    /** @brief Дескрипторы DMA кадра; при двойной буферизации через DMA — буфера рисования. */
    lldesc_t volatile *_dma_buffers;

    /** @brief Число дескрипторов DMA кадра. */
    int _dma_buffers_count;

    /**
     * @brief Головной дескриптор кольца при двойной буферизации через DMA.
     *
     * Ссылается на первый дескриптор видимого кадра; swap_buffers()
     * переключает его на другую цепочку.
     */
    lldesc_t volatile *_dma_buffers_head;

    /** @brief Дескрипторы видимого кадра; без двойной буферизации через DMA — @c _dma_buffers. */
    lldesc_t volatile *_dma_buffers_visible;

    /**
     * @brief Пустая строка с кадровым синхроимпульсом.
     *
     * Гасящие интервалы, синхроимпульс и пустая видимая часть идут в
     * порядке @c vga_timings::h_starting_block.
     */
    volatile std::uint8_t *_h_blank_line_with_v_sync;

    /** @brief Пустая строка без кадрового синхроимпульса. */
    volatile std::uint8_t *_h_blank_line;

    /** @brief Блоки памяти буфера кадра; список кончается @c nullptr. */
    std::uint8_t **_view_port_memory_pool;

    /** @brief Полная высота кадра со всеми гасящими интервалами, строки. */
    std::int16_t _raw_frame_height;

    /** @brief Сколько байтов кучи остаётся свободным после буфера кадра (set_heap_reserve()). */
    std::size_t _heap_reserve;
};

}


// Реализация

// Открытые

/**
 * @brief Возвращает левый столбец области вывода на экране.
 * @return Столбец; не 0, если область уже экрана.
 */
int esp::vga::vga_base_controller::view_port_col() const
{
    return _view_port_col;
}

/**
 * @brief Возвращает верхнюю строку области вывода на экране.
 * @return Строка; не 0, если область ниже экрана.
 */
int esp::vga::vga_base_controller::view_port_row() const
{
    return _view_port_row;
}

/**
 * @brief Возвращает текущую развёртку; её можно менять и передавать в set_resolution().
 * @return Указатель на развёртку.
 */
esp::vga::vga_timings *esp::vga::vga_base_controller::resolution_timings()
{
    return &_timings;
}

/**
 * @brief Возвращает число разрядов на канал цвета.
 * @return 1 (8 цветов) или 2 (64 цвета) — смотря какой begin() вызван.
 */
std::uint8_t esp::vga::vga_base_controller::bits_per_channel() const
{
    return _bits_per_channel;
}

/**
 * @brief Возвращает число цветов вывода.
 * @return 8 или 64.
 */
int esp::vga::vga_base_controller::colors_count()
{
    return 1 << (3 * _bits_per_channel);
}

/**
 * @brief Возвращает строку буфера рисования.
 *
 * Строку заполняют байты пикселей (create_raw_pixel()) в порядке байтов
 * 2, 3, 0, 1 в каждом 32-разрядном слове (см. pixel_in_row()).
 *
 * @param y Строка, 0 — верхняя.
 * @return Указатель на строку.
 */
std::uint8_t *esp::vga::vga_base_controller::scanline(int y)
{
    return const_cast<std::uint8_t *>(_view_port[y]);
}

/**
 * @brief Возвращает байт пикселя: цвет и разряды синхроимпульсов видимой строки.
 * @param rgb Цвет, по 2 разряда на канал.
 * @return Байт пикселя.
 */
std::uint8_t esp::vga::vga_base_controller::create_raw_pixel(esp::gfx::rgb222 rgb)
{
    return prepare_pixel(rgb);
}

/**
 * @brief Возвращает байт чёрного пикселя с разрядами синхроимпульсов видимой строки.
 * @return Байт пикселя.
 */
std::uint8_t esp::vga::vga_base_controller::create_blank_raw_pixel() const
{
    return _hv_sync;
}

// Защищённые

/**
 * @brief Собирает байт пикселя из цвета и разрядов синхроимпульсов видимой строки.
 *
 * Всегда встраивается: её вызывают обработчики прерываний.
 *
 * @param rgb Цвет, по 2 разряда на канал.
 * @return Байт пикселя.
 */
std::uint8_t esp::vga::vga_base_controller::prepare_pixel(esp::gfx::rgb222 rgb)
{
    return _hv_sync | (rgb.b << BLUE_BIT) | (rgb.g << GREEN_BIT) | (rgb.r << RED_BIT);
}

/**
 * @brief Подгоняет размер области вывода под требования контроллера.
 *
 * Здесь ничего не делает; производные контроллеры уточняют.
 */
void esp::vga::vga_base_controller::check_view_port_size() {}

/**
 * @brief Возвращает байт пикселя @p x в строке буфера.
 *
 * Модуль I2S выводит 32-разрядные слова, начиная со старшей половины,
 * поэтому пиксели в каждом слове идут в порядке байтов 2, 3, 0, 1:
 * пиксель 0 — байт 2, пиксель 1 — байт 3, пиксель 2 — байт 0, пиксель 3 —
 * байт 1, пиксель 4 — байт 6 и так далее. Формулу предложил Paul Scott
 * Robson (https://github.com/paulscottrobson). Всегда встраивается: её
 * вызывают обработчики прерываний.
 *
 * @tparam T Тип байта строки: @c std::uint8_t или @c volatile @c std::uint8_t.
 * @param row Строка.
 * @param x Пиксель.
 * @return Ссылка на байт пикселя.
 */
template<typename T>
T &esp::vga::vga_base_controller::pixel_in_row(T *row, int x)
{
    return row[x ^ 2];
}

/**
 * @brief Возвращает байт пикселя (@p x, @p y) буфера рисования; всегда встраивается.
 * @param x Столбец.
 * @param y Строка.
 * @return Ссылка на байт пикселя.
 */
volatile std::uint8_t &esp::vga::vga_base_controller::pixel(int x, int y)
{
    return pixel_in_row(_view_port[y], x);
}

/**
 * @brief Инвертирует цвет пикселя (@p x, @p y) буфера рисования, не трогая синхроимпульсы.
 *
 * Всегда встраивается.
 *
 * @param x Столбец.
 * @param y Строка.
 */
void esp::vga::vga_base_controller::invert_pixel(int x, int y)
{
    volatile std::uint8_t *const px = &pixel(x, y);
    *px = ~(*px ^ SYNC_MASK);
}
