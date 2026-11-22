// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains all classes related to FabGL Sound System
 */


#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#if __has_include("esp32/rom/lldesc.h")
#   include "esp32/rom/lldesc.h"
#else
#   include "rom/lldesc.h"
#endif
#include "esp_timer.h"
#include "soc/i2s_struct.h"
#include "soc/sens_struct.h"

#include "esp_tools.h"


#include "driver/sdm.h"


namespace esp::sound
{


/** @brief Частота дискретизации по умолчанию, Гц. */
constexpr int DEFAULT_SAMPLE_RATE = 16384;


// DAC mode specific:
// setting 64 at 16KHz, generates 16000/64=250 interrupts per second
/** @brief Отсчётов в буфере DMA (ЦАП): 64 при 16 кГц — 250 прерываний в секунду. */
constexpr int SAMPLE_BUFFER_SIZE = 64;


/** @brief Base abstract class for waveform generators. A waveform generator can be seen as an audio channel that will be mixed by SoundGenerator. */
class waveform_generator
{
public:

    waveform_generator()
        : next(nullptr)
        , _sample_rate(0)
        , _volume(100)
        , _enabled(false)
        , _duration(-1)
        , _auto_destroy(false)
        , _auto_detach(false)
    {
    }

    virtual ~waveform_generator() {}

    /**
   * @brief Sets output frequency
   *
   * @param value Frequency in Hertz
   */
    virtual void set_frequency(int value) = 0;

    /**
   * @brief Sets number of samples to play
   *
   * @param value Number of samples to play. -1 = infinite.
   */
    void set_duration(uint32_t value)
    {
        _duration = value;
    }

    /**
   * @brief Returns number of remaining samples to play
   *
   * @return Number of remaining samples to play. -1 = infinite.
   */
    uint32_t duration()
    {
        return _duration;
    }

    /**
   * @brief Sets autodetach mode
   *
   * @param value if true: this object needs to be detached from the sound generator when there are no more samples to play.
   */
    void set_auto_detach(bool value)
    {
        _auto_detach = value;
    }

    bool auto_detach()
    {
        return _auto_detach;
    }

    /**
   * @brief Sets autodestroy mode
   *
   * @param value if true: this object needs to be destroyed by the sound generat or when there are no more samples to play. This will set also setAutoDetach(true).
   */
    void set_auto_destroy(bool value)
    {
        _auto_destroy = value;
        _auto_detach |= value;
    }

    bool auto_destroy()
    {
        return _auto_destroy;
    }

    /**
   * @brief Gets next sample
   *
   * @return Sample value as signed 8 bit (-128..127 range)
   */
    virtual int sample() = 0;

    /**
   * @brief Sets volume of this generator
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
    void set_volume(int value)
    {
        _volume = value;
    }

    /**
   * @brief Determines current volume
   *
   * @return Current volume of this generator (0 = minimum, 127 = maximum)
   */
    int volume()
    {
        return _volume;
    }

    /**
   * @brief Determines whether this generator is enabled or disabled
   *
   * @return True if this generator is enabled
   */
    bool enabled()
    {
        return _enabled;
    }

    /**
   * @brief Enables or disabled this generator
   *
   * A generator is disabled for default and must be enabled in order to play sound
   *
   * @param value True to enable the generator, False to disable
   */
    void enable(bool value)
    {
        _enabled = value;
    }

    /**
   * @brief Sets the sample rate
   *
   * Default sample rate is 160000 Hertz.
   *
   * @param value Sample rate in Hertz
   */
    void set_sample_rate(int value)
    {
        _sample_rate = value;
    }

    /**
   * @brief Determines the sample rate
   *
   * @return Current sample rate in Hertz
   */
    uint16_t sample_rate()
    {
        return _sample_rate;
    }

    waveform_generator *next;

protected:

    void dec_duration()
    {
        --_duration;
        if (_duration == 0)
            _enabled = false;
    }

private:

    uint16_t _sample_rate;
    int8_t _volume;
    int8_t _enabled; // 0 = disabled, 1 = enabled
    uint32_t _duration; // number of samples to play (-1 = infinite)
    bool
        _auto_destroy; // if true: this object needs to be destroyed by the sound generator when there are no more samples to play
    bool
        _auto_detach; // if true: this object needs to be autodetached from the sound generator when there are no more samples to play
};


/** @brief Sine waveform generator */
class sine_waveform_generator : public waveform_generator
{
public:

    sine_waveform_generator();

    void set_frequency(int value);

    int sample();

private:

    uint32_t _phase_inc;
    uint32_t _phase_acc;
    uint16_t _frequency;
    int16_t _last_sample;
};


/** @brief Square waveform generator */
class square_waveform_generator : public waveform_generator
{
public:

    square_waveform_generator();

    void set_frequency(int value);

    /**
   * @brief Sets square wave duty cycle
   *
   * @param dutyCycle Duty cycle in 0..255 range. 255 = 100%
   */
    void set_duty_cycle(int duty_cycle);

    int sample();

private:

    uint32_t _phase_inc;
    uint32_t _phase_acc;
    uint16_t _frequency;
    int8_t _last_sample;
    uint8_t _duty_cycle;
};


/** @brief Triangle waveform generator */
class triangle_waveform_generator : public waveform_generator
{
public:

    triangle_waveform_generator();

    void set_frequency(int value);

    int sample();

private:

    uint32_t _phase_inc;
    uint32_t _phase_acc;
    uint16_t _frequency;
    int8_t _last_sample;
};


/** @brief Sawtooth waveform generator */
class sawtooth_waveform_generator : public waveform_generator
{
public:

    sawtooth_waveform_generator();

    void set_frequency(int value);

    int sample();

private:

    uint32_t _phase_inc;
    uint32_t _phase_acc;
    uint16_t _frequency;
    int8_t _last_sample;
};


/** @brief Noise generator */
class noise_waveform_generator : public waveform_generator
{
public:

    noise_waveform_generator();

    void set_frequency(int value);

    int sample();

private:

    uint16_t _noise;
};


/////////////////////////////////////////////////////////////////////////////////////////////
// "tries" to emulate VIC6561 noise generator
// derived from a reverse enginnered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210

/**
 * @brief Emulates VIC6561 (VIC20) noise generator
 *
 * Inspired from a reverse enginnered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210
 */
class vic_noise_generator : public waveform_generator
{
public:

    vic_noise_generator();

    void set_frequency(int value);
    uint16_t frequency()
    {
        return _frequency;
    }

    int sample();

private:

    static const uint16_t LFSRINIT = 0x0202;
    static const int CLK = 4433618;

    uint16_t _frequency;
    uint16_t _counter;
    uint16_t _lfsr;
    uint16_t _out_sr;
};


/**
 * @brief Samples generator
 *
 * Sample data should be sampled at the same samplerate of the sound generator.
 * Only 8 bit (signed - not compressed) depth is supported.
 */
class samples_generator : public waveform_generator
{
public:

    samples_generator(int8_t const *data, int length);

    void set_frequency(int value);

    int sample();

private:

    int8_t const *_data;
    int _length;
    int _index;
};


/** \ingroup Enumerations
 * @brief Specifies sound generation method
 */
enum class sound_gen_method
{
    dac, /**< Use DAC. Available on gpio 25 or 26. Very low resources occupation. */
    sigma_delta, /**< Use Sigma-Delta. Available on almost all GPIO pins. Requires a lot of CPU power for high sample rates. */
    Auto, /**< Use DAC when video output is VGA, use SigmaDelta when video output is Composite. */
};


/**
 * @brief SoundGenerator handles audio output
 *
 * SoundGenerator generates audio samples using DAC or Sigma-Delta modulation. Use constructor to specify GPIO and generation method.
 * Applications attach waveform generators (like SineWaveformGenerator, SquareWaveformGenerator, etc...) and call SoundGenerator.play() to start audio generation.
 *
 * Here is a list of supported sound generators:
 * - SineWaveformGenerator
 * - SquareWaveformGenerator
 * - TriangleWaveformGenerator
 * - SawtoothWaveformGenerator
 * - NoiseWaveformGenerator
 * - VICNoiseGenerator
 * - SamplesGenerator
 */
class sound_generator
{

public:

    /**
   * @brief Creates an instance of the sound generator. Only one instance is allowed
   *
   * @param sampleRate Sample rate in Hertz.
   * @param gpio GPIO to use. DAC mode can be set on 25 or 26. Value GPIO_AUTO will set GPIO 25 when genMethod is DAC and GPIO 23 when genMethod is SigmaDelta.
   * @param genMethod Sound generation method. Can be DAC, sigma-delta or automatic. If automatic then DAC is selected when VGA is used, otherwise SigmaDelta is selected when Composite is used.
   *
   * Example:
   *
   *     // creates sound generator with automatic values (depends by selected video output, VGA or CVBS)
   *     SoundGenerator soundGenerator;
   *
   *     // creates sound generator using 16Khz sample rate, GPIO 25 in DAC mode
   *     SoundGenerator soundGenerator(16000, GPIO_NUM_25, SoundGenMethod::DAC);
   *
   *     // creates sound generator using 16Khz sample rate, GPIO 23 in sigma-delta mode
   *     SoundGenerator soundGenerator(16000, GPIO_NUM_23, SoundGenMethod::SigmaDelta);
   */
    sound_generator(int sample_rate = DEFAULT_SAMPLE_RATE, gpio_num_t gpio = GPIO_AUTO,
                    sound_gen_method gen_method = sound_gen_method::Auto);

    ~sound_generator();

    /**
   * @brief Stops playing and removes all attached waveform generators
   */
    void clear();

    /**
   * @brief Starts or stops playing
   *
   * @param value True = starts playing, False = stops playing
   *
   * @return Returns previous playing state
   *
   * Example:
   *
   *     soundGenerator.play(true);
   */
    bool play(bool value);

    /**
   * @brief Plays the specified samples
   *
   * Starts immediately to play the specified samples. It is not required to call play().
   * This method returns without wait the end of sound.
   *
   * @param data Samples to play.
   * @param length Number of samples to play.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   * @param durationMS Duration in milliseconds. 0 = untile end of samples, -1 = infinite loop.
   *
   * @return Pointer to SamplesGenerator object. Lifetime of this object is limited to the duration.
   */
    samples_generator *play_samples(int8_t const *data, int length, int volume = 100,
                                    int duration_ms = 0);

    /**
   * @brief Plays the specified waveform
   *
   * Starts immediately to play the specified waveform. It is not required to call play().
   * This method returns without wait the end of sound.
   *
   * @param waveform Waveform to play.
   * @param frequency Frequency in Hertz.
   * @param durationMS Duration in milliseconds.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   *
   * Example:
   *
   *     // plays a sinewave at 500Hz for 200 milliseconds
   *     soundGen.playSound(SineWaveformGenerator(), 500, 200);
   *
   *     // plays a C Major chord for 1 second
   *     soundGen.playSound(SineWaveformGenerator(), 262, 1000);  // C
   *     soundGen.playSound(SineWaveformGenerator(), 330, 1000);  // E
   *     soundGen.playSound(SineWaveformGenerator(), 392, 1000);  // G
   */
    template<typename T>
    void play_sound(T const &waveform, int frequency, int duration_ms, int volume = 100)
    {
        auto wf = new T(waveform);
        attach(wf);
        wf->set_frequency(frequency);
        wf->set_volume(volume);
        wf->set_auto_destroy(true);
        wf->set_duration(_sample_rate / 1000 * duration_ms);
        wf->enable(true);
        play(true);
    }

    /**
   * @brief Determines whether sound generator is playing
   *
   * @return True when playing, False otherwise
   */
    bool playing()
    {
        return _play;
    }

    waveform_generator *channels()
    {
        return _channels;
    }

    /**
   * @brief Attaches a waveform generator
   *
   * @param value Pointer of the waveform generator to attach
   *
   * Example:
   *
   *     SineWaveformGenerator sine;
   *     soundGenerator.attach(&sine);
   *     sine.enable(true);
   *     sine.setFrequency(500);  // 500 Hz
   */
    void attach(waveform_generator *value);

    /**
   * @brief Detaches a waveform generator
   *
   * @param value Pointer of the waveform generator to detach
   */
    void detach(waveform_generator *value);

    /**
   * @brief Sets the overall volume
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
    void set_volume(int value)
    {
        _volume = value;
    }

    /**
   * @brief Determines current overall volume
   *
   * @return Current overall volume (0 = minimum, 127 = maximum)
   */
    int volume()
    {
        return _volume;
    }


private:

    static void isr_handler(void *arg); // used in DAC mode
    static void timer_handler(void *args); // used in sigma-delta mode

    void dac_init();
    void sigmadelta_init();
    void detach_no_suspend(waveform_generator *value);
    void set_dma_node(int index, volatile uint16_t *buf, int len);
    void init();

    int sample();


    waveform_generator *_channels;

    uint16_t *_sample_buffer[2];

    int8_t _volume;

    uint16_t _sample_rate;

    bool _play;

    gpio_num_t _gpio;

    intr_handle_t _isr_handle;

    lldesc_t volatile *_dma_chain;

    sound_gen_method _gen_method;

    bool _init_done;

    esp_timer_handle_t _timer_handle;

    sdm_channel_handle_t _sdm_channel = nullptr;
};


} // end of namespace
