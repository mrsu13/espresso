// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include <alloca.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/rtc.h"

#include "esp_tools.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_utils.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


/*************************************************************************************/
/* GPIOStream definitions */

void gpio_stream::begin()
{
    _dma_started = false;
    _dma_buffer = nullptr;
    _dma_data = nullptr;
}


void gpio_stream::begin(bool div_1_on_gpio0, gpio_num_t div2, gpio_num_t div4, gpio_num_t div8,
                        gpio_num_t div16, gpio_num_t div32, gpio_num_t div64, gpio_num_t div128,
                        gpio_num_t div256)
{
    _dma_started = false;

    if (div_1_on_gpio0)
        setup_gpio(GPIO_NUM_0, -1, GPIO_MODE_OUTPUT); // note: GPIO_NUM_0 cannot be changed!
    setup_gpio(div2, 0, GPIO_MODE_OUTPUT);
    setup_gpio(div4, 1, GPIO_MODE_OUTPUT);
    setup_gpio(div8, 2, GPIO_MODE_OUTPUT);
    setup_gpio(div16, 3, GPIO_MODE_OUTPUT);
    setup_gpio(div32, 4, GPIO_MODE_OUTPUT);
    setup_gpio(div64, 5, GPIO_MODE_OUTPUT);
    setup_gpio(div128, 6, GPIO_MODE_OUTPUT);
    setup_gpio(div256, 7, GPIO_MODE_OUTPUT);

    _dma_data = (volatile uint8_t *)heap_caps_malloc(256, MALLOC_CAP_DMA);
    for (int i = 0; i < 256; ++i)
        _dma_data[i] = i;

    _dma_buffer = (volatile lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
    _dma_buffer->eof = 0;
    _dma_buffer->sosf = 0;
    _dma_buffer->owner = 1;
    _dma_buffer->qe.stqe_next = (lldesc_t *)_dma_buffer;
    _dma_buffer->offset = 0;
    _dma_buffer->size = 256;
    _dma_buffer->length = 256;
    _dma_buffer->buf = (uint8_t *)_dma_data;
}


void gpio_stream::end()
{
    stop();
}


// if bit is -1 = clock signal
// gpio = GPIO_UNUSED means not set
void gpio_stream::setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode)
{
    if (gpio != GPIO_UNUSED)
    {

        if (bit == -1)
        {
            // I2S1 clock out to CLK_OUT1 (fixed to GPIO0)
            WRITE_PERI_REG(PIN_CTRL, 0xF);
            PIN_FUNC_SELECT(GPIO_PIN_REG_0, FUNC_GPIO0_CLK_OUT1);
        }
        else
        {
            configure_gpio(gpio, mode);
            esp_rom_gpio_connect_out_signal(gpio, I2S1O_DATA_OUT0_IDX + bit, false, false);
        }
    }
}


void gpio_stream::play(int freq, lldesc_t volatile *dma_buffers)
{
    if (!_dma_started)
    {

        // Power on device
        esp::i2s_module_enable(1, true);

        // Initialize I2S device
        I2S1.conf.tx_reset = 1;
        I2S1.conf.tx_reset = 0;

        // Reset DMA
        I2S1.lc_conf.out_rst = 1;
        I2S1.lc_conf.out_rst = 0;

        // Reset FIFO
        I2S1.conf.tx_fifo_reset = 1;
        I2S1.conf.tx_fifo_reset = 0;

        // LCD mode
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

        // Reset AHB interface of DMA
        I2S1.lc_conf.ahbm_rst = 1;
        I2S1.lc_conf.ahbm_fifo_rst = 1;
        I2S1.lc_conf.ahbm_rst = 0;
        I2S1.lc_conf.ahbm_fifo_rst = 0;

        // Start DMA
        I2S1.lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
        I2S1.out_link.addr = (uint32_t)(dma_buffers ? &dma_buffers[0] : _dma_buffer);
        I2S1.out_link.start = 1;
        I2S1.conf.tx_start = 1;

        _dma_started = true;
    }
}


void gpio_stream::stop()
{
    if (_dma_started)
    {
        apll_enable(false);
        esp::i2s_module_enable(1, false);

        _dma_started = false;
    }
}


void gpio_stream::setup_clock(int freq)
{
    apll_params p = {0, 0, 0, 0};
    double error, out_freq;
    uint8_t a = 1, b = 0;
    apll_calc_params(freq, &p, &a, &b, &out_freq, &error);

    I2S1.clkm_conf.val = 0;
    I2S1.clkm_conf.clkm_div_b = b;
    I2S1.clkm_conf.clkm_div_a = a;
    I2S1.clkm_conf.clkm_div_num = 2; // not less than 2

    I2S1.sample_rate_conf.tx_bck_div_num = 1; // this makes I2S1O_BCK = I2S1_CLK

    apll_enable(true, p.sdm0, p.sdm1, p.sdm2, p.o_div);

    I2S1.clkm_conf.clka_en = 1;
}


} // end of namespace
