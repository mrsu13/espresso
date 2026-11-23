// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_vga_gpio_stream.h
 * @brief Объявление потока DMA → GPIO esp::vga::gpio_stream.
 */

/**
 * @file esp_vga_gpio_stream.cpp
 * @brief Поток DMA → GPIO через модуль I2S1 с тактированием от APLL.
 */

#include "esp_vga_gpio_stream.h"

#include "esp_tools.h"
#include "esp_vga_utils_p.h"

#include <cstdint>

#include <esp_heap_caps.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <soc/i2s_reg.h>
#include <soc/i2s_struct.h>
#include <soc/io_mux_reg.h>


#pragma GCC optimize("O2")


/**
 * @class esp::vga::gpio_stream
 * @brief Поток DMA → GPIO: модуль I2S1 в режиме LCD выводит байты из
 *        памяти на восемь выводов с частотой от генератора APLL.
 *
 * Так контроллеры VGA выводят изображение: play() запускает поток из их
 * буферов DMA. Без своих буферов поток выдаёт на выводы частоту,
 * делённую на 2, 4, …, 256 (begin() с выводами).
 *
 * Пока ESP_VGA_USE_APLL_AB_COEF равен 0, частоты — от 2 651 514 до
 * 62 500 000 Гц; погрешность в среднем 21 Гц и не больше 1000 Гц, кроме
 * двух участков: от 41 666 667 до 42 708 333 Гц частота остаётся
 * 41 666 666 Гц, от 42 708 334 до 43 748 999 Гц — 43 750 000 Гц. С
 * ESP_VGA_USE_APLL_AB_COEF = 1 частоты — от 82 500 до 62 500 000 Гц,
 * погрешность в среднем около 7 Гц и не больше 6349 Гц, но частота
 * сильно дрожит.
 *
 * @bug begin() с выводами выделяет буферы DMA при каждом вызове, а end()
 *      их не освобождает.
 */


// Открытые

/**
 * @brief Готовит поток к play() со своими буферами DMA.
 *
 * Поток, запущенный прежде, останавливается, буферы делителей частоты
 * освобождаются (см. end()).
 */
void esp::vga::gpio_stream::begin()
{
    end();
}

/**
 * @brief Готовит поток делителей частоты и подключает выводы.
 *
 * После play() на выводы выдаётся частота, делённая на 2, 4, …, 256:
 * @code
 * esp::vga::gpio_stream stream;
 * stream.begin(true, esp::GPIO_UNUSED, GPIO_NUM_5); // 25 МГц на GPIO0, 6,25 МГц на GPIO5.
 * stream.play(25000000);
 * @endcode
 *
 * @param div_1_on_gpio0 @c true — выдавать частоту без деления на GPIO0.
 * @param div2 Вывод частоты / 2 или @c esp::GPIO_UNUSED.
 * @param div4 Вывод частоты / 4 или @c esp::GPIO_UNUSED.
 * @param div8 Вывод частоты / 8 или @c esp::GPIO_UNUSED.
 * @param div16 Вывод частоты / 16 или @c esp::GPIO_UNUSED.
 * @param div32 Вывод частоты / 32 или @c esp::GPIO_UNUSED.
 * @param div64 Вывод частоты / 64 или @c esp::GPIO_UNUSED.
 * @param div128 Вывод частоты / 128 или @c esp::GPIO_UNUSED.
 * @param div256 Вывод частоты / 256 или @c esp::GPIO_UNUSED.
 */
void esp::vga::gpio_stream::begin(bool div_1_on_gpio0, gpio_num_t div2, gpio_num_t div4,
                                  gpio_num_t div8, gpio_num_t div16, gpio_num_t div32,
                                  gpio_num_t div64, gpio_num_t div128, gpio_num_t div256)
{
    // Прежний поток и его буферы не нужны.
    end();

    if (div_1_on_gpio0)
        setup_gpio(GPIO_NUM_0, -1, GPIO_MODE_OUTPUT); // Только GPIO0.
    setup_gpio(div2, 0, GPIO_MODE_OUTPUT);
    setup_gpio(div4, 1, GPIO_MODE_OUTPUT);
    setup_gpio(div8, 2, GPIO_MODE_OUTPUT);
    setup_gpio(div16, 3, GPIO_MODE_OUTPUT);
    setup_gpio(div32, 4, GPIO_MODE_OUTPUT);
    setup_gpio(div64, 5, GPIO_MODE_OUTPUT);
    setup_gpio(div128, 6, GPIO_MODE_OUTPUT);
    setup_gpio(div256, 7, GPIO_MODE_OUTPUT);

    _dma_data = static_cast<volatile std::uint8_t *>(heap_caps_malloc(256, MALLOC_CAP_DMA));
    for (int i = 0; i < 256; ++i)
        _dma_data[i] = i;

    _dma_buffer =
        static_cast<volatile lldesc_t *>(heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA));
    _dma_buffer->eof = 0;
    _dma_buffer->sosf = 0;
    _dma_buffer->owner = 1;
    _dma_buffer->qe.stqe_next = const_cast<lldesc_t *>(_dma_buffer);
    _dma_buffer->offset = 0;
    _dma_buffer->size = 256;
    _dma_buffer->length = 256;
    _dma_buffer->buf = const_cast<std::uint8_t *>(_dma_data);
}

/**
 * @brief Останавливает поток (см. stop()) и освобождает буферы делителей частоты.
 */
void esp::vga::gpio_stream::end()
{
    stop();
    free_buffers();
}

/**
 * @brief Запускает поток с заданной частотой байтов.
 *
 * Если поток уже запущен, ничего не делает.
 *
 * @param freq Частота, Гц.
 * @param dma_buffers Свои буферы DMA (кольцо дескрипторов) или @c nullptr —
 *        поток делителей частоты из begin() с выводами.
 */
void esp::vga::gpio_stream::play(int freq, lldesc_t volatile *dma_buffers)
{
    if (!_dma_started)
    {
        // Включаем модуль I2S1.
        esp::i2s_module_enable(1, true);

        // Сбрасываем передатчик, DMA и FIFO.
        I2S1.conf.tx_reset = 1;
        I2S1.conf.tx_reset = 0;

        I2S1.lc_conf.out_rst = 1;
        I2S1.lc_conf.out_rst = 0;

        I2S1.conf.tx_fifo_reset = 1;
        I2S1.conf.tx_fifo_reset = 0;

        // Режим LCD: байты выводятся параллельно на восемь разрядов.
        I2S1.conf2.val = 0;
        I2S1.conf2.lcd_en = 1;
        I2S1.conf2.lcd_tx_wrx2_en = 1;
        I2S1.conf2.lcd_tx_sdx2_en = 0;

        I2S1.sample_rate_conf.val = 0;
        I2S1.sample_rate_conf.tx_bits_mod = 8;

        setup_clock(freq);

        I2S1.fifo_conf.val = 0;
        I2S1.fifo_conf.tx_fifo_mod_force_en = 1;
        I2S1.fifo_conf.tx_fifo_mod = 1;
        I2S1.fifo_conf.tx_fifo_mod = 1;
        I2S1.fifo_conf.tx_data_num = 32;
        I2S1.fifo_conf.dscr_en = 1;

        I2S1.conf1.val = 0;
        I2S1.conf1.tx_stop_en = 0;
        I2S1.conf1.tx_pcm_bypass = 1;

        I2S1.conf_chan.val = 0;
        I2S1.conf_chan.tx_chan_mod = 1;

        I2S1.conf.tx_right_first = 1;

        I2S1.timing.val = 0;

        // Сбрасываем интерфейс AHB у DMA.
        I2S1.lc_conf.ahbm_rst = 1;
        I2S1.lc_conf.ahbm_fifo_rst = 1;
        I2S1.lc_conf.ahbm_rst = 0;
        I2S1.lc_conf.ahbm_fifo_rst = 0;

        // Запускаем DMA.
        I2S1.lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
        I2S1.out_link.addr =
            reinterpret_cast<std::uint32_t>(dma_buffers ? &dma_buffers[0] : _dma_buffer);
        I2S1.out_link.start = 1;
        I2S1.conf.tx_start = 1;

        _dma_started = true;
    }
}

/**
 * @brief Останавливает поток: выключает APLL и модуль I2S1.
 */
void esp::vga::gpio_stream::stop()
{
    if (_dma_started)
    {
        esp::vga::p::apll_enable(false);
        esp::i2s_module_enable(1, false);

        _dma_started = false;
    }
}

// Закрытые

/**
 * @brief Настраивает генератор APLL и делитель I2S1 на заданную частоту.
 * @param freq Частота, Гц.
 */
void esp::vga::gpio_stream::setup_clock(int freq)
{
    esp::vga::p::apll_params params{0, 0, 0, 0};
    double error, out_freq;
    std::uint8_t a = 1, b = 0;
    esp::vga::p::apll_calc_params(freq, &params, &a, &b, &out_freq, &error);

    I2S1.clkm_conf.val = 0;
    I2S1.clkm_conf.clkm_div_b = b;
    I2S1.clkm_conf.clkm_div_a = a;
    I2S1.clkm_conf.clkm_div_num = 2; // Не меньше 2.

    I2S1.sample_rate_conf.tx_bck_div_num = 1; // Тогда I2S1O_BCK = I2S1_CLK.

    esp::vga::p::apll_enable(true, params.sdm0, params.sdm1, params.sdm2, params.o_div);

    I2S1.clkm_conf.clka_en = 1;
}

/**
 * @brief Освобождает буферы потока делителей частоты, если они есть.
 *
 * Поток к этому времени остановлен: иначе DMA продолжал бы читать
 * освобождённую память.
 */
void esp::vga::gpio_stream::free_buffers()
{
    heap_caps_free(const_cast<lldesc_t *>(_dma_buffer));
    heap_caps_free(const_cast<std::uint8_t *>(_dma_data));
    _dma_buffer = nullptr;
    _dma_data = nullptr;
}

/**
 * @brief Подключает вывод к разряду выхода I2S1.
 * @param gpio Вывод или @c esp::GPIO_UNUSED — не подключать.
 * @param bit Разряд выхода: 0…7; −1 — тактовый сигнал I2S1 (только GPIO0).
 * @param mode Режим вывода.
 */
void esp::vga::gpio_stream::setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode)
{
    if (gpio != esp::GPIO_UNUSED)
    {
        if (bit == -1)
        {
            // Тактовый сигнал I2S1 — на CLK_OUT1, то есть на GPIO0.
            WRITE_PERI_REG(PIN_CTRL, 0xF);
            PIN_FUNC_SELECT(GPIO_PIN_REG_0, FUNC_GPIO0_CLK_OUT1);
        }
        else
        {
            esp::vga::p::configure_gpio(gpio, mode);
            esp_rom_gpio_connect_out_signal(gpio, I2S1O_DATA_OUT0_IDX + bit, false, false);
        }
    }
}
