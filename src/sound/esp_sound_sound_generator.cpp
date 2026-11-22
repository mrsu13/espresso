// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "soc/i2s_reg.h"
#include "soc/rtc.h"
#include <soc/sens_reg.h>


#include "esp_sound_sound_generator.h"
#include "esp_sound_utils.h"


#pragma GCC optimize("O2")


namespace esp::sound
{


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SineWaveformGenerator


static const int8_t SIN_TABLE[257] = {
    0,    3,    6,    9,    12,   16,   19,   22,   25,   28,   31,   34,   37,   40,   43,   46,
    49,   51,   54,   57,   60,   63,   65,   68,   71,   73,   76,   78,   81,   83,   85,   88,
    90,   92,   94,   96,   98,   100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
    117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,
    127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  122,  121,  120,  118,
    117,  116,  115,  113,  112,  111,  109,  107,  106,  104,  102,  100,  98,   96,   94,   92,
    90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,   60,   57,   54,   51,
    49,   46,   43,   40,   37,   34,   31,   28,   25,   22,   19,   16,   12,   9,    6,    3,
    0,    -3,   -6,   -9,   -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
    -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
    -90,  -92,  -94,  -96,  -98,  -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
    -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
    -117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100, -98,  -96,  -94,  -92,
    -90,  -88,  -85,  -83,  -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
    -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,  -12,  -9,   -6,   -3,
    0,
};


sine_waveform_generator::sine_waveform_generator()
    : _phase_inc(0)
    , _phase_acc(0)
    , _frequency(0)
    , _last_sample(0)
{
}


void sine_waveform_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value;
        _phase_inc = (((uint32_t)_frequency * 256) << 11) / sample_rate();
    }
}


int sine_waveform_generator::sample()
{
    if (_frequency == 0 || duration() == 0)
    {
        if (_last_sample > 0)
            --_last_sample;
        else if (_last_sample < 0)
            ++_last_sample;
        else
            _phase_acc = 0;
        return _last_sample;
    }

    // get sample  (-128...+127)
    uint32_t index = _phase_acc >> 11;
    int sample = SIN_TABLE[index];

    // process volume
    sample = sample * volume() / 127;

    _last_sample = sample;

    _phase_acc = (_phase_acc + _phase_inc) & 0x7ffff;

    dec_duration();

    return sample;
}


// SineWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SquareWaveformGenerator


square_waveform_generator::square_waveform_generator()
    : _phase_inc(0)
    , _phase_acc(0)
    , _frequency(0)
    , _last_sample(0)
    , _duty_cycle(127)
{
}


void square_waveform_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value;
        _phase_inc = (((uint32_t)_frequency * 256) << 11) / sample_rate();
    }
}


// dutyCycle: 0..255 (255=100%)
void square_waveform_generator::set_duty_cycle(int duty_cycle)
{
    _duty_cycle = duty_cycle;
}


int square_waveform_generator::sample()
{
    if (_frequency == 0 || duration() == 0)
    {
        if (_last_sample > 0)
            --_last_sample;
        else if (_last_sample < 0)
            ++_last_sample;
        else
            _phase_acc = 0;
        return _last_sample;
    }

    uint32_t index = _phase_acc >> 11;
    int sample = (index <= _duty_cycle ? 127 : -127);

    // process volume
    sample = sample * volume() / 127;

    _last_sample = sample;

    _phase_acc = (_phase_acc + _phase_inc) & 0x7ffff;

    dec_duration();

    return sample;
}

// SquareWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TriangleWaveformGenerator


triangle_waveform_generator::triangle_waveform_generator()
    : _phase_inc(0)
    , _phase_acc(0)
    , _frequency(0)
    , _last_sample(0)
{
}


void triangle_waveform_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value;
        _phase_inc = (((uint32_t)_frequency * 256) << 11) / sample_rate();
    }
}


int triangle_waveform_generator::sample()
{
    if (_frequency == 0 || duration() == 0)
    {
        if (_last_sample > 0)
            --_last_sample;
        else if (_last_sample < 0)
            ++_last_sample;
        else
            _phase_acc = 0;
        return _last_sample;
    }

    uint32_t index = _phase_acc >> 11;
    int sample = (index & 0x80 ? -1 : 1) * ((index & 0x3F) * 2 - (index & 0x40 ? 0 : 127));

    // process volume
    sample = sample * volume() / 127;

    _last_sample = sample;

    _phase_acc = (_phase_acc + _phase_inc) & 0x7ffff;

    dec_duration();

    return sample;
}

// TriangleWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SawtoothWaveformGenerator


sawtooth_waveform_generator::sawtooth_waveform_generator()
    : _phase_inc(0)
    , _phase_acc(0)
    , _frequency(0)
    , _last_sample(0)
{
}


void sawtooth_waveform_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value;
        _phase_inc = (((uint32_t)_frequency * 256) << 11) / sample_rate();
    }
}


int sawtooth_waveform_generator::sample()
{
    if (_frequency == 0 || duration() == 0)
    {
        if (_last_sample > 0)
            --_last_sample;
        else if (_last_sample < 0)
            ++_last_sample;
        else
            _phase_acc = 0;
        return _last_sample;
    }

    uint32_t index = _phase_acc >> 11;
    int sample = index - 128;

    // process volume
    sample = sample * volume() / 127;

    _last_sample = sample;

    _phase_acc = (_phase_acc + _phase_inc) & 0x7ffff;

    dec_duration();

    return sample;
}

// TriangleWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoiseWaveformGenerator


noise_waveform_generator::noise_waveform_generator()
    : _noise(0xFAB7)
{
}


void noise_waveform_generator::set_frequency(int value) {}


int noise_waveform_generator::sample()
{
    if (duration() == 0)
    {
        return 0;
    }

    // noise generator based on Galois LFSR
    _noise = (_noise >> 1) ^ (-(_noise & 1) & 0xB400u);
    int sample = 127 - (_noise >> 8);

    // process volume
    sample = sample * volume() / 127;

    dec_duration();

    return sample;
}


// NoiseWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////
// VICNoiseGenerator
// "tries" to emulate VIC6561 noise generator
// derived from a reverse engineering VHDL code: http://sleepingelephant.com/ipw-web/bulletin/bb/viewtopic.php?f=11&t=8733&fbclid=IwAR16x6OMMb670bA2ZtYcbB0Zat_X-oHNB0NxxYigPffXa8G_InSIoNAEiPU


vic_noise_generator::vic_noise_generator()
    : _frequency(0)
    , _counter(0)
    , _lfsr(LFSRINIT)
    , _out_sr(0)
{
}


void vic_noise_generator::set_frequency(int value)
{
    if (_frequency != value)
    {
        _frequency = value >= 127 ? 0 : value;
        _lfsr = LFSRINIT;
        _counter = 0;
        _out_sr = 0;
    }
}


int vic_noise_generator::sample()
{
    if (duration() == 0)
    {
        return 0;
    }

    const int reduc = CLK / 8 / sample_rate(); // resample to sampleRate() (ie 16000Hz)

    int sample = 0;

    for (int i = 0; i < reduc; ++i)
    {

        if (_counter >= 127)
        {

            // reset counter
            _counter = _frequency;

            if (_lfsr & 1)
                _out_sr = ((_out_sr << 1) | ~(_out_sr >> 7));

            _lfsr <<= 1;
            int bit3 = (_lfsr >> 3) & 1;
            int bit12 = (_lfsr >> 12) & 1;
            int bit14 = (_lfsr >> 14) & 1;
            int bit15 = (_lfsr >> 15) & 1;
            _lfsr |= (bit3 ^ bit12) ^ (bit14 ^ bit15);
        }
        else
            ++_counter;

        sample += _out_sr & 1 ? 127 : -128;
    }

    // simple mean of all samples

    sample = sample / reduc;

    // process volume
    sample = sample * volume() / 127;

    dec_duration();

    return sample;
}


// VICNoiseGenerator
/////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SamplesGenerator


samples_generator::samples_generator(int8_t const *data, int length)
    : _data(data)
    , _length(length)
    , _index(0)
{
}


void samples_generator::set_frequency(int value) {}


int samples_generator::sample()
{

    if (duration() == 0)
    {
        return 0;
    }

    int sample = _data[_index++];

    if (_index == _length)
        _index = 0;

    // process volume
    sample = sample * volume() / 127;

    dec_duration();

    return sample;
}


// SamplesGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundGenerator


sound_generator::sound_generator(int sample_rate, gpio_num_t gpio, sound_gen_method gen_method)
    : _channels(nullptr)
    , _sample_buffer{0}
    , _volume(100)
    , _sample_rate(sample_rate)
    , _play(false)
    , _gpio(gpio)
    , _isr_handle(nullptr)
    , _dma_chain(nullptr)
    , _gen_method(gen_method)
    , _init_done(false)
    , _timer_handle(nullptr)
{
}


sound_generator::~sound_generator()
{
    clear();

    if (_isr_handle)
    {
        // cleanup DAC mode
        esp::i2s_module_enable(0, false);
        esp_intr_free(_isr_handle);
        for (int i = 0; i < 2; ++i)
            heap_caps_free(_sample_buffer[i]);
        heap_caps_free((void *)_dma_chain);
    }

    if (_timer_handle)
    {
        // cleanup sigmadelta mode
        esp_timer_stop(_timer_handle);
        esp_timer_delete(_timer_handle);
        _timer_handle = nullptr;
        sdm_channel_disable(_sdm_channel);
        sdm_del_channel(_sdm_channel);
        _sdm_channel = nullptr;
    }
}


void sound_generator::clear()
{
    play(false);
    _channels = nullptr;
}


void sound_generator::set_dma_node(int index, volatile uint16_t *buf, int len)
{
    _dma_chain[index].eof = 1; // always generate interrupt
    _dma_chain[index].sosf = 0;
    _dma_chain[index].owner = 1;
    _dma_chain[index].qe.stqe_next = (lldesc_t *)(_dma_chain + index + 1);
    _dma_chain[index].offset = 0;
    _dma_chain[index].size = len * sizeof(uint16_t);
    _dma_chain[index].length = len * sizeof(uint16_t);
    _dma_chain[index].buf = (uint8_t *)buf;
}


void sound_generator::dac_init()
{
    _dma_chain = (volatile lldesc_t *)heap_caps_malloc(2 * sizeof(lldesc_t), MALLOC_CAP_DMA);

    for (int i = 0; i < 2; ++i)
    {
        _sample_buffer[i] =
            (uint16_t *)heap_caps_malloc(SAMPLE_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
        for (int j = 0; j < SAMPLE_BUFFER_SIZE; ++j)
            _sample_buffer[i][j] = 0x7f00;
        set_dma_node(i, _sample_buffer[i], SAMPLE_BUFFER_SIZE);
    }
    _dma_chain[1].sosf = 1;
    _dma_chain[1].qe.stqe_next = (lldesc_t *)_dma_chain; // closes DMA chain

    esp::i2s_module_enable(0, true);

    // Initialize I2S device
    I2S0.conf.tx_reset = 1;
    I2S0.conf.tx_reset = 0;

    // Reset DMA
    I2S0.lc_conf.in_rst = 1;
    I2S0.lc_conf.in_rst = 0;

    // Reset FIFO
    I2S0.conf.rx_fifo_reset = 1;
    I2S0.conf.rx_fifo_reset = 0;

    I2S0.conf_chan.tx_chan_mod = (_gpio == GPIO_NUM_25 ? 3 : 4);

    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.fifo_conf.tx_fifo_mod = 1;
    I2S0.fifo_conf.dscr_en = 1;

    I2S0.conf.tx_mono = 1;
    I2S0.conf.tx_start = 0;
    I2S0.conf.tx_msb_right = 1;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_slave_mod = 0;
    I2S0.conf.tx_short_sync = 0;
    I2S0.conf.tx_msb_shift = 0;

    I2S0.conf2.lcd_en = 1;
    I2S0.conf2.camera_en = 0;

    int a, b, num, m;
    _sample_rate = calc_i2s_timing_params(_sample_rate, &a, &b, &num, &m);
    I2S0.clkm_conf.clka_en = 0;
    I2S0.clkm_conf.clkm_div_a = a;
    I2S0.clkm_conf.clkm_div_b = b;
    I2S0.clkm_conf.clkm_div_num = num;
    I2S0.sample_rate_conf.tx_bck_div_num = m;

    I2S0.sample_rate_conf.tx_bits_mod = 16;

    if (_isr_handle == nullptr)
    {
        esp::esp_intr_alloc_pinned_to_core(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, isr_handler,
                                           this, &_isr_handle, esp::core_usage::quiet_core());
        I2S0.int_clr.val = 0xFFFFFFFF;
        I2S0.int_ena.out_eof = 1;
    }

    I2S0.out_link.addr = (uintptr_t)_dma_chain;
    I2S0.out_link.start = 1;
    I2S0.conf.tx_start = 1;

    dac_enable_i2s_output(_gpio);
}


void sound_generator::sigmadelta_init()
{
    sdm_config_t sdm_cfg = {};
    sdm_cfg.gpio_num = _gpio;
    sdm_cfg.clk_src = SDM_CLK_SRC_DEFAULT;
    sdm_cfg.sample_rate_hz = 80000000 / 11; // same as legacy prescale = 10
    sdm_new_channel(&sdm_cfg, &_sdm_channel);
    sdm_channel_enable(_sdm_channel);

    esp_timer_create_args_t args = {};
    args.callback = timer_handler;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&args, &_timer_handle);
}


void sound_generator::init()
{
    if (!_init_done)
    {
        // handle automatic paramters
        if (_gen_method == sound_gen_method::Auto)
            _gen_method = esp::current_video_mode::get() == esp::video_mode::cvbs
                ? sound_gen_method::sigma_delta
                : sound_gen_method::dac;
        if (_gpio == GPIO_AUTO)
            _gpio = _gen_method == sound_gen_method::dac ? GPIO_NUM_25 : GPIO_NUM_23;

        // actual init
        if (_gen_method == sound_gen_method::dac)
            dac_init();
        else
            sigmadelta_init();


        _init_done = true;
    }
}


bool sound_generator::play(bool value)
{
    if (value != _play)
    {
        init();

        if (_gen_method == sound_gen_method::dac)
        {
            I2S0.conf.tx_start = value;
        }
        else
        {
            if (value)
                esp_timer_start_periodic(_timer_handle, 1000000 / _sample_rate);
            else
                esp_timer_stop(_timer_handle);
        }


        _play = value;
        return !value;
    }
    else
        return value;
}


samples_generator *sound_generator::play_samples(int8_t const *data, int length, int volume,
                                                 int duration_ms)
{
    auto sgen = new samples_generator(data, length);
    attach(sgen);
    sgen->set_auto_destroy(true);
    if (duration_ms > -1)
        sgen->set_duration(duration_ms > 0 ? (_sample_rate / 1000 * duration_ms) : length);
    sgen->set_volume(volume);
    sgen->enable(true);
    play(true);
    return sgen;
}


// does NOT take ownership of the waveform generator
void sound_generator::attach(waveform_generator *value)
{
    bool is_playing = play(false);

    value->set_sample_rate(_sample_rate);

    value->next = _channels;
    _channels = value;

    play(is_playing);
}


void sound_generator::detach(waveform_generator *value)
{
    if (!value)
        return;

    bool is_playing = play(false);
    detach_no_suspend(value);
    play(is_playing);
}


void sound_generator::detach_no_suspend(waveform_generator *value)
{
    for (waveform_generator *c = _channels, *prev = nullptr; c; prev = c, c = c->next)
    {
        if (c == value)
        {
            if (prev)
                prev->next = c->next;
            else
                _channels = c->next;
            if (value->auto_destroy())
                delete value;
            break;
        }
    }
}


int IRAM_ATTR sound_generator::sample()
{
    int sample = 0, tvol = 0;
    for (auto g = _channels; g;)
    {
        if (g->enabled())
        {
            sample += g->sample();
            tvol += g->volume();
        }
        else if (g->duration() == 0 && g->auto_detach())
        {
            auto curr = g;
            g = g->next; // setup next item before detaching this one
            detach_no_suspend(curr);
            continue; // bypass "g = g->next;"
        }
        g = g->next;
    }

    int avol = tvol ? esp::imin(127, 127 * 127 / tvol) : 127;
    sample = sample * avol / 127;
    sample = sample * volume() / 127;

    return sample;
}


// used by DAC generator
void IRAM_ATTR sound_generator::isr_handler(void *arg)
{
    if (I2S0.int_st.out_eof)
    {

        auto sound_generator = (esp::sound::sound_generator *)arg;
        auto desc = (volatile lldesc_t *)I2S0.out_eof_des_addr;

        auto buf = (uint16_t *)sound_generator->_sample_buffer[desc->sosf];

        for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
            buf[i ^ 1] = (sound_generator->sample() + 127) << 8;
    }
    I2S0.int_clr.val = I2S0.int_st.val;
}


// used by sigma-delta generator
void sound_generator::timer_handler(void *args)
{
    auto sound_generator = (esp::sound::sound_generator *)args;

    sdm_channel_set_pulse_density(sound_generator->_sdm_channel, sound_generator->sample());
}


// SoundGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*

// note (C,D,E,F,G,A,B) + [#,b] + octave (2..7) + space + tempo (99..1)
// pause (P) + space + tempo (99.1)
char const * noteToFreq(char const * note, int * freq)
{
  uint16_t NIDX2FREQ[][12] = { {   66,   70,   74,   78,   83,   88,   93,   98,  104,  110,  117,  124 }, // 2
                               {  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247 }, // 3
                               {  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494 }, // 4
                               {  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988 }, // 5
                               { 1046, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976 }, // 6
                               { 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951 }, // 7
                             };
  uint8_t NNAME2NIDX[] = {9, 11, 0, 2, 4, 5, 7};  // A, B, C, D, E, F, G
  *freq = 0;
  while (*note && *note == ' ')
    ++note;
  if (*note == 0)
    return note;
  int noteIndex = (*note >= 'A' && *note <= 'G' ? NNAME2NIDX[*note - 'A'] : -2); // -2 = pause
  ++note;
  if (*note == '#') {
    ++noteIndex;
    ++note;
  } else if (*note == 'b') {
    --noteIndex;
    ++note;
  }
  int octave = *note - '0';
  ++note;
  if (noteIndex == -1) {
    noteIndex = 11;
    --octave;
  } else if (noteIndex == 12) {
    noteIndex = 0;
    ++octave;
  }
  if (noteIndex >= 0 && noteIndex <= 11 && octave >= 2 && octave <= 7)
    *freq = NIDX2FREQ[octave - 2][noteIndex];
  return note;
}


char const * noteToDelay(char const * note, int * delayMS)
{
  *delayMS = 0;
  while (*note && *note == ' ')
    ++note;
  if (*note == 0)
    return note;
  int val = atoi(note);
  if (val > 0)
    *delayMS = 1000 / val;
  return note + (val > 9 ? 2 : 1);
}


void play_task(void*arg)
{
  const char * music = "A4 4 A4 4 A#4 4 C5 4 C5 4 A#4 4 A4 4 G4 4 F4 4 F4 4 G4 4 A4 4 A4 2 G4 16 G4 2 P 8 "
                       "A4 4 A4 4 A#4 4 C5 4 C5 4 A#4 4 A4 4 G4 4 F4 4 F4 4 G4 4 A4 4 G4 2 F4 16 F4 2 P 8";
  while (true) {
    if (Play) {
      const char * m = music;
      while (*m && Play) {
        int freq, delms;
        m = noteToFreq(m, &freq);
        m = noteToDelay(m, &delms);
        frequency = freq;
        delay(delms);
        frequency = 0;
        delay(25);
      }
      Play = false;
    } else
      delay(100);
  }
}


*/


} // end of namespace
