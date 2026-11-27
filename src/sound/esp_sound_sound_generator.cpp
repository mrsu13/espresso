// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_sound_sound_generator.h
 * @brief Объявление генератора звука esp::sound::sound_generator.
 */

/**
 * @file esp_sound_sound_generator.cpp
 * @brief Генератор звука: смешивание генераторов волны и вывод на ЦАП или
 *        сигма-дельта-модулятор.
 */

/**
 * @namespace esp::sound
 * @brief Звук: генераторы волны и генератор звука, который смешивает их и
 *        выводит на встроенный ЦАП или сигма-дельта-модулятор.
 */

#include "esp_sound_sound_generator.h"

#include "esp_core_usage.h"
#include "esp_current_video_mode.h"
#include "esp_sound_config.h"
#include "esp_tools.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <driver/rtc_io.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <soc/i2s_struct.h>
#include <soc/rtc_io_struct.h>
#include <soc/sens_struct.h>
#include <soc/soc.h>


#pragma GCC optimize("O2")


namespace esp::sound::p
{

void dac_enable_i2s_output(gpio_num_t gpio);
int calc_i2s_timing_params(int sample_rate, int *out_a, int *out_b, int *out_n, int *out_m);

}


static_assert(sizeof(esp::sound::samples_generator) <= esp::sound::VOICE_SIZE,
              "the samples generator does not fit into a voice: raise esp::sound::VOICE_SIZE");
static_assert(esp::sound::VOICE_SIZE % alignof(std::max_align_t) == 0,
              "esp::sound::VOICE_SIZE must keep every voice aligned");


/**
 * @class esp::sound::sound_generator
 * @brief Генератор звука: смешивает генераторы волны и выводит смесь на ЦАП
 *        или сигма-дельта-модулятор.
 *
 * К генератору звука подключаются генераторы волны:
 * esp::sound::sine_waveform_generator, esp::sound::square_waveform_generator,
 * esp::sound::triangle_waveform_generator,
 * esp::sound::sawtooth_waveform_generator,
 * esp::sound::noise_waveform_generator, esp::sound::vic_noise_generator,
 * esp::sound::samples_generator или свои, производные от
 * esp::sound::waveform_generator. На каждый отсчёт вывода генератор звука
 * смешивает отсчёты всех включённых генераторов волны.
 *
 * Смесь выводится на встроенный ЦАП через модуль I2S0 и DMA или на
 * сигма-дельта-модулятор, которому таймер подаёт отсчёт за отсчётом (см.
 * esp::sound::sound_gen_method). Оборудование настраивается при первом
 * запуске звука, а не в конструкторе.
 *
 * Простые звуки удобно подавать методами play_sound() и play_samples(): они
 * сами создают генератор волны, подключают его и запускают вывод.
 * Генераторы этих звуков живут в голосах — местах внутри объекта, по
 * @c esp::sound::VOICE_SIZE байтов на голос, — а не в динамической памяти;
 * голосов @c esp::sound::VOICE_COUNT, и голос отзвучавшего звука достаётся
 * следующему. Генераторы, подключённые через attach(), принадлежат
 * вызывающему.
 *
 * Список каналов обходит обработчик прерывания или таймера, возможно на
 * другом ядре, поэтому список и голоса защищены спин-блокировкой. Сами
 * методы генератора звука вызываются из одной задачи.
 *
 * Генератор звука может быть только один: он занимает модуль I2S0 и его
 * прерывание. Копировать его нельзя: копия освободила бы то же
 * оборудование второй раз.
 */


// Открытые

/**
 * @brief Создаёт генератор звука; оборудование настраивается при первом запуске.
 *
 * Конструктор только запоминает параметры, поэтому объект можно создавать
 * статически и раньше видеоконтроллера: способ
 * @c sound_gen_method::automatic выбирается по видеовыходу при первом
 * вызове play().
 *
 * @code
 * // Способ и вывод выбираются по видеовыходу.
 * esp::sound::sound_generator sound;
 *
 * // 16 кГц, ЦАП на GPIO25.
 * esp::sound::sound_generator sound(16000, GPIO_NUM_25, esp::sound::sound_gen_method::dac);
 *
 * // 16 кГц, сигма-дельта-модулятор на GPIO23.
 * esp::sound::sound_generator sound(16000, GPIO_NUM_23, esp::sound::sound_gen_method::sigma_delta);
 * @endcode
 *
 * @param sample_rate Частота дискретизации, Гц. В режиме ЦАП
 *        действительная частота немного отличается: делители тактовой
 *        частоты I2S подбираются к заданной как можно точнее.
 * @param gpio Вывод звука: для ЦАП — GPIO25 или GPIO26, для
 *        сигма-дельта-модулятора — почти любой. @c esp::GPIO_AUTO выбирает
 *        GPIO25 для ЦАП и GPIO23 для модулятора.
 * @param gen_method Способ вывода звука.
 */
esp::sound::sound_generator::sound_generator(int sample_rate, gpio_num_t gpio,
                                             sound_gen_method gen_method)
    : _channels{nullptr}
    , _sample_buffer{}
    , _volume{100}
    , _sample_rate{static_cast<std::uint16_t>(sample_rate)}
    , _play{false}
    , _gpio{gpio}
    , _isr_handle{nullptr}
    , _dma_chain{nullptr}
    , _gen_method{gen_method}
    , _init_done{false}
    , _timer_handle{nullptr}
    , _sdm_channel{nullptr}
    , _voices{}
{
    portMUX_INITIALIZE(&_lock);
}

/**
 * @brief Останавливает звук и освобождает оборудование.
 *
 * Генераторы волны отключаются, голоса освобождаются (см. clear()). В
 * режиме ЦАП выключается модуль I2S0 и освобождаются прерывание, буферы
 * отсчётов и дескрипторы DMA; в режиме сигма-дельта останавливается и
 * удаляется таймер и освобождается канал модулятора.
 */
esp::sound::sound_generator::~sound_generator()
{
    clear();

    if (_isr_handle)
    {
        // Режим ЦАП.
        esp::i2s_module_enable(0, false);
        esp_intr_free(_isr_handle);
        for (int i = 0; i < 2; ++i)
            heap_caps_free(_sample_buffer[i]);
        heap_caps_free(const_cast<lldesc_t *>(_dma_chain));
    }

    if (_timer_handle)
    {
        // Режим сигма-дельта.
        esp_timer_stop(_timer_handle);
        esp_timer_delete(_timer_handle);
        _timer_handle = nullptr;
        sdm_channel_disable(_sdm_channel);
        sdm_del_channel(_sdm_channel);
        _sdm_channel = nullptr;
    }
}

/**
 * @brief Останавливает звук, отключает все генераторы волны и освобождает голоса.
 *
 * Генераторы голосов разрушаются; генераторы, подключённые через attach(),
 * только отключаются — ими владеет вызывающий.
 */
void esp::sound::sound_generator::clear()
{
    play(false);

    portENTER_CRITICAL(&_lock);
    _channels = nullptr;
    for (waveform_generator *&generator : _voices)
    {
        if (generator)
        {
            std::destroy_at(generator);
            generator = nullptr;
        }
    }
    portEXIT_CRITICAL(&_lock);
}

/**
 * @brief Запускает или останавливает вывод звука.
 *
 * Первый запуск настраивает оборудование (init()). В режиме ЦАП
 * запускается или останавливается передача I2S0, в режиме сигма-дельта —
 * периодический таймер. Период таймера — целое число микросекунд,
 * отброшенная дробная часть чуть повышает частоту дискретизации.
 *
 * @param value @c true — запустить, @c false — остановить.
 * @return Прежнее состояние: @c true, если звук выводился.
 */
bool esp::sound::sound_generator::play(bool value)
{
    if (value != _play)
    {
        init();

        if (_gen_method == sound_gen_method::dac)
            I2S0.conf.tx_start = value;
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
    return value;
}

/**
 * @brief Воспроизводит готовые отсчёты из памяти.
 *
 * Создаёт в свободном голосе генератор отсчётов
 * esp::sound::samples_generator, подключает его и запускает звук, не
 * дожидаясь его конца; play() вызывать не нужно. Отсчёты должны быть
 * записаны с частотой дискретизации генератора звука.
 *
 * @param data Отсчёты, 8-разрядные целые со знаком; не копируются и должны
 *        существовать, пока звучат.
 * @param length Число отсчётов.
 * @param volume Громкость, от 0 до 127.
 * @param duration_ms Длительность, мс; 0 — один проход по отсчётам,
 *        отрицательное значение — по кругу, пока генератор не отключат
 *        (detach()) или не вызовут clear().
 * @return Созданный генератор отсчётов или @c nullptr, если свободных
 *         голосов нет. Указатель годен, пока голос не достался новому
 *         звуку: это может случиться после того, как генератор отзвучал или
 *         отключён.
 */
esp::sound::samples_generator *esp::sound::sound_generator::play_samples(const std::int8_t *data,
                                                                         int length, int volume,
                                                                         int duration_ms)
{
    const int index = available_voice();
    if (index < 0)
        return nullptr;

    samples_generator *const generator = std::construct_at(
        reinterpret_cast<samples_generator *>(_voice_storage[index]), data, length);
    if (duration_ms > -1)
        generator->set_duration(duration_ms > 0 ? (_sample_rate / 1000 * duration_ms) : length);
    generator->set_volume(volume);
    start_voice(index, generator);
    return generator;
}

/**
 * @brief Подключает генератор волны.
 *
 * Генератор встаёт в начало списка каналов и получает частоту
 * дискретизации генератора звука. Звучать он начинает, когда включён
 * (esp::sound::waveform_generator::enable()) и запущен звук (play()).
 * Владение не передаётся. Повторное подключение того же генератора ничего
 * не делает.
 *
 * @code
 * esp::sound::sine_waveform_generator sine;
 * sound.attach(&sine);
 * sine.set_frequency(500); // 500 Гц.
 * sine.enable(true);
 * sound.play(true);
 * @endcode
 *
 * @param value Генератор волны; должен существовать, пока подключён.
 */
void esp::sound::sound_generator::attach(waveform_generator *value)
{
    value->set_sample_rate(_sample_rate);

    portENTER_CRITICAL(&_lock);
    // Второе подключение того же генератора замкнуло бы список в кольцо.
    if (!has_channel(value))
    {
        value->set_next(_channels);
        _channels = value;
    }
    portEXIT_CRITICAL(&_lock);
}

/**
 * @brief Отключает генератор волны.
 *
 * Генератор голоса (созданный play_sound() или play_samples()) после
 * отключения считается отзвучавшим, и его голос достаётся следующему
 * звуку.
 *
 * @param value Генератор волны; @c nullptr и неподключённый генератор
 *        пропускаются.
 */
void esp::sound::sound_generator::detach(waveform_generator *value)
{
    portENTER_CRITICAL(&_lock);
    remove_channel(value);
    portEXIT_CRITICAL(&_lock);
}

// Закрытые

/**
 * @brief Обработчик прерывания I2S0 в режиме ЦАП: заполняет выведенный буфер.
 *
 * Прерывание приходит, когда DMA вывел буфер. Адрес его дескриптора I2S0
 * сообщает в @c out_eof_des_addr, а поле @c sosf дескриптора — номер
 * буфера. Пока DMA выводит второй буфер, этот заполняется новыми
 * отсчётами под спин-блокировкой.
 *
 * Обработчик лежит в IRAM ради скорости. Прерывание выделено без флага
 * @c ESP_INTR_FLAG_IRAM, поэтому на время записи во флеш-память оно
 * откладывается, и обработчик может вызывать методы генераторов волны,
 * лежащие во флеш-памяти.
 *
 * @param arg Генератор звука.
 */
void IRAM_ATTR esp::sound::sound_generator::isr_handler(void *arg)
{
    if (I2S0.int_st.out_eof)
    {
        sound_generator *const generator = static_cast<sound_generator *>(arg);
        const volatile lldesc_t *const desc =
            reinterpret_cast<const volatile lldesc_t *>(I2S0.out_eof_des_addr);

        std::uint16_t *const buf = generator->_sample_buffer[desc->sosf];

        // ЦАП берёт старший байт 16-разрядного слова как число без знака,
        // поэтому отсчёт сдвигается на 128: −128 даёт 0, тишина — 128,
        // 127 — 255. Соседние отсчёты меняются местами (i ^ 1), потому что
        // I2S выводит половины 32-разрядного слова начиная со старшей.
        portENTER_CRITICAL_ISR(&generator->_lock);
        for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
            buf[i ^ 1] = (generator->sample() + 128) << 8;
        portEXIT_CRITICAL_ISR(&generator->_lock);
    }
    I2S0.int_clr.val = I2S0.int_st.val;
}

/**
 * @brief Обработчик таймера в режиме сигма-дельта: подаёт модулятору следующий отсчёт.
 *
 * Таймер срабатывает с частотой дискретизации; обработчик выполняется в
 * задаче esp_timer. Отсчёт смешивается под спин-блокировкой, а
 * модулятору передаётся уже после неё.
 *
 * @param arg Генератор звука.
 */
void esp::sound::sound_generator::timer_handler(void *arg)
{
    sound_generator *const generator = static_cast<sound_generator *>(arg);

    portENTER_CRITICAL(&generator->_lock);
    const int value = generator->sample();
    portEXIT_CRITICAL(&generator->_lock);

    sdm_channel_set_pulse_density(generator->_sdm_channel, value);
}

/**
 * @brief Настраивает оборудование; без вызова — при первом запуске звука.
 *
 * Сначала разрешает автоматические параметры: способ
 * @c sound_gen_method::automatic становится сигма-дельта-модулятором при
 * композитном видеовыходе и ЦАП в остальных случаях, вывод @c esp::GPIO_AUTO —
 * GPIO25 для ЦАП и GPIO23 для модулятора. Затем выделяет буферы и
 * прерывание; звук не запускает. Вызвать заранее стоит программе, которой
 * важно, где в куче лягут буферы. Повторные вызовы ничего не делают.
 */
void esp::sound::sound_generator::init()
{
    if (!_init_done)
    {
        // Автоматические параметры.
        if (_gen_method == sound_gen_method::automatic)
            _gen_method = esp::current_video_mode::get() == esp::video_mode::cvbs
                ? sound_gen_method::sigma_delta
                : sound_gen_method::dac;
        if (_gpio == esp::GPIO_AUTO)
            _gpio = _gen_method == sound_gen_method::dac ? GPIO_NUM_25 : GPIO_NUM_23;

        if (_gen_method == sound_gen_method::dac)
            dac_init();
        else
            sigmadelta_init();

        _init_done = true;
    }
}

/**
 * @brief Настраивает вывод на ЦАП: модуль I2S0, два буфера DMA и прерывание.
 *
 * Буферы отсчётов и их дескрипторы выделяются в памяти, доступной DMA, и
 * заполняются тишиной; дескрипторы замыкаются в кольцо, и I2S0 выводит
 * буферы по очереди, вызывая прерывание по концу каждого. I2S0 работает в
 * режиме LCD, в котором его выход подаётся на встроенный ЦАП. Делители
 * тактовой частоты подбираются к заданной частоте дискретизации, и в
 * @c _sample_rate записывается действительная. Прерывание закрепляется за
 * менее загруженным ядром (esp::core_usage::quiet_core()) — по умолчанию
 * не тем, на котором работают задачи видео. Если выделить прерывание не
 * удалось, программа останавливается с сообщением об ошибке.
 */
void esp::sound::sound_generator::dac_init()
{
    _dma_chain =
        static_cast<volatile lldesc_t *>(heap_caps_malloc(2 * sizeof(lldesc_t), MALLOC_CAP_DMA));

    for (int i = 0; i < 2; ++i)
    {
        _sample_buffer[i] = static_cast<std::uint16_t *>(
            heap_caps_malloc(SAMPLE_BUFFER_SIZE * sizeof(std::uint16_t), MALLOC_CAP_DMA));
        // Тишина: середина диапазона ЦАП в старшем байте.
        for (int j = 0; j < SAMPLE_BUFFER_SIZE; ++j)
            _sample_buffer[i][j] = 0x8000;
        set_dma_node(i, _sample_buffer[i], SAMPLE_BUFFER_SIZE);
    }
    // По полю sosf обработчик прерывания узнаёт второй буфер;
    // второй дескриптор замыкает кольцо.
    _dma_chain[1].sosf = 1;
    _dma_chain[1].qe.stqe_next = const_cast<lldesc_t *>(_dma_chain);

    esp::i2s_module_enable(0, true);

    // Сброс передатчика.
    I2S0.conf.tx_reset = 1;
    I2S0.conf.tx_reset = 0;

    // Сброс входного DMA.
    I2S0.lc_conf.in_rst = 1;
    I2S0.lc_conf.in_rst = 0;

    // Сброс FIFO приёма.
    I2S0.conf.rx_fifo_reset = 1;
    I2S0.conf.rx_fifo_reset = 0;

    // Один канал: режим 3 выводит отсчёты на ЦАП 1 (GPIO25), режим 4 — на ЦАП 2 (GPIO26).
    I2S0.conf_chan.tx_chan_mod = _gpio == GPIO_NUM_25 ? 3 : 4;

    // FIFO: 16-разрядные отсчёты одного канала, данные из DMA.
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

    // Режим LCD: в нём выход I2S0 подаётся на встроенный ЦАП.
    I2S0.conf2.lcd_en = 1;
    I2S0.conf2.camera_en = 0;

    // Делители тактовой частоты; частота дискретизации становится действительной.
    int a = 0;
    int b = 0;
    int num = 0;
    int m = 0;
    _sample_rate = esp::sound::p::calc_i2s_timing_params(_sample_rate, &a, &b, &num, &m);
    I2S0.clkm_conf.clka_en = 0;
    I2S0.clkm_conf.clkm_div_a = a;
    I2S0.clkm_conf.clkm_div_b = b;
    I2S0.clkm_conf.clkm_div_num = num;
    I2S0.sample_rate_conf.tx_bck_div_num = m;

    I2S0.sample_rate_conf.tx_bits_mod = 16;

    if (!_isr_handle)
    {
        ESP_ERROR_CHECK(esp::esp_intr_alloc_pinned_to_core(
            ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, isr_handler, this, &_isr_handle,
            esp::core_usage::quiet_core()));
        I2S0.int_clr.val = 0xFFFFFFFF;
        I2S0.int_ena.out_eof = 1;
    }

    // Запуск DMA и передачи.
    I2S0.out_link.addr = reinterpret_cast<std::uintptr_t>(_dma_chain);
    I2S0.out_link.start = 1;
    I2S0.conf.tx_start = 1;

    esp::sound::p::dac_enable_i2s_output(_gpio);
}

/**
 * @brief Настраивает вывод на сигма-дельта-модулятор с таймером.
 *
 * Канал модулятора на выводе @c _gpio тактируется частотой 80 МГц / 11,
 * как с предделителем 10 в прежнем драйвере. Таймер esp_timer создаётся
 * остановленным: его запускает play() с периодом дискретизации, и на
 * каждом срабатывании timer_handler() подаёт модулятору отсчёт.
 */
void esp::sound::sound_generator::sigmadelta_init()
{
    sdm_config_t sdm_cfg = {};
    sdm_cfg.gpio_num = _gpio;
    sdm_cfg.clk_src = SDM_CLK_SRC_DEFAULT;
    sdm_cfg.sample_rate_hz = 80000000 / 11;
    sdm_new_channel(&sdm_cfg, &_sdm_channel);
    sdm_channel_enable(_sdm_channel);

    esp_timer_create_args_t args = {};
    args.callback = timer_handler;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&args, &_timer_handle);
}

/**
 * @brief Заполняет дескриптор DMA для буфера отсчётов.
 *
 * Дескриптор отдаётся DMA (@c owner), помечается концом кадра (@c eof),
 * чтобы по выводу буфера приходило прерывание, и ссылается на следующий
 * дескриптор массива; кольцо замыкает dac_init().
 *
 * @param index Номер дескриптора в @c _dma_chain.
 * @param buf Буфер отсчётов.
 * @param len Число 16-разрядных отсчётов в буфере.
 */
void esp::sound::sound_generator::set_dma_node(int index, volatile std::uint16_t *buf, int len)
{
    _dma_chain[index].eof = 1;
    _dma_chain[index].sosf = 0;
    _dma_chain[index].owner = 1;
    _dma_chain[index].qe.stqe_next = const_cast<lldesc_t *>(_dma_chain + index + 1);
    _dma_chain[index].offset = 0;
    _dma_chain[index].size = len * sizeof(std::uint16_t);
    _dma_chain[index].length = len * sizeof(std::uint16_t);
    _dma_chain[index].buf = reinterpret_cast<const volatile std::uint8_t *>(buf);
}

/**
 * @brief Находит голос для нового звука.
 *
 * Пустой голос годится сразу. Занятый голос свободен, когда его генератора
 * больше нет в списке каналов: звук отзвучал и автоотключение убрало
 * генератор из списка или генератор отключили (detach()). Отзвучавший
 * генератор, которого обработчик ещё не успел убрать из списка, убирается
 * здесь. Генератор свободного голоса разрушается.
 *
 * @return Номер свободного голоса или −1, если все голоса звучат.
 */
int esp::sound::sound_generator::available_voice()
{
    int index = -1;

    portENTER_CRITICAL(&_lock);
    for (int i = 0; i < VOICE_COUNT && index < 0; ++i)
    {
        waveform_generator *const generator = _voices[i];
        if (!generator)
            index = i;
        else
        {
            // Отзвучавший генератор обработчик убрал бы из списка на
            // следующем отсчёте; голос нужен сейчас.
            const bool finished = !generator->enabled() && !generator->duration();
            if (finished)
                remove_channel(generator);
            if (finished || !has_channel(generator))
            {
                std::destroy_at(generator);
                _voices[i] = nullptr;
                index = i;
            }
        }
    }
    portEXIT_CRITICAL(&_lock);

    return index;
}

/**
 * @brief Запускает звук голоса.
 *
 * Записывает настроенный генератор в голос, включает ему автоотключение,
 * чтобы по окончании он ушёл из списка каналов и освободил голос, включает
 * его, подключает и запускает вывод звука.
 *
 * @param index Номер голоса, полученный от available_voice().
 * @param generator Генератор, созданный в месте этого голоса.
 */
void esp::sound::sound_generator::start_voice(int index, waveform_generator *generator)
{
    _voices[index] = generator;
    generator->set_auto_detach(true);
    generator->enable(true);
    attach(generator);
    play(true);
}

/**
 * @brief Сообщает, есть ли генератор волны в списке каналов.
 *
 * Вызывается под спин-блокировкой @c _lock.
 *
 * @param value Генератор волны.
 * @return @c true, если генератор подключён.
 */
bool esp::sound::sound_generator::has_channel(const waveform_generator *value) const
{
    for (const waveform_generator *channel = _channels; channel; channel = channel->next())
    {
        if (channel == value)
            return true;
    }
    return false;
}

/**
 * @brief Убирает генератор волны из списка каналов.
 *
 * Вызывается под спин-блокировкой @c _lock — из detach(), из
 * available_voice() и из sample(), то есть и в обработчике прерывания.
 * Генератор не разрушается и не освобождается.
 *
 * @param value Генератор волны; если его нет в списке, ничего не
 *        происходит.
 */
void esp::sound::sound_generator::remove_channel(waveform_generator *value)
{
    for (waveform_generator *channel = _channels, *previous = nullptr; channel;
         previous = channel, channel = channel->next())
    {
        if (channel == value)
        {
            if (previous)
                previous->set_next(channel->next());
            else
                _channels = channel->next();
            return;
        }
    }
}

/**
 * @brief Смешивает очередной отсчёт всех каналов.
 *
 * Складывает отсчёты включённых генераторов волны и их громкости. Если
 * сумма громкостей больше 127, смесь ослабляется в отношении
 * 127 / сумма, чтобы не выйти за пределы отсчёта; затем применяется общая
 * громкость. Выключенные генераторы с истёкшей длительностью и
 * автоотключением убираются из списка каналов здесь же; память при этом
 * не освобождается.
 *
 * Вызывается под спин-блокировкой @c _lock. Метод лежит в IRAM: в режиме
 * ЦАП его вызывает обработчик прерывания.
 *
 * @return Отсчёт от −128 до 127.
 */
int IRAM_ATTR esp::sound::sound_generator::sample()
{
    int value = 0;
    int total_volume = 0;
    for (waveform_generator *channel = _channels; channel;)
    {
        if (channel->enabled())
        {
            value += channel->sample();
            total_volume += channel->volume();
        }
        else if (!channel->duration() && channel->auto_detach())
        {
            // Следующий канал запоминается до того, как отзвучавший
            // уйдёт из списка.
            waveform_generator *const finished = channel;
            channel = channel->next();
            remove_channel(finished);
            continue;
        }
        channel = channel->next();
    }

    // Смесь ослабляется, если сумма громкостей каналов больше 127,
    // затем применяется общая громкость.
    const int mix_volume = total_volume ? std::min(127, 127 * 127 / total_volume) : 127;
    value = value * mix_volume / 127;
    value = value * volume() / 127;

    return value;
}

/**
 * @brief Подаёт выход I2S0 на встроенный ЦАП вывода @p gpio.
 *
 * Делает то же, что пара функций @c dac_i2s_enable() и
 * @c dac_output_enable() прежнего драйвера ЦАП: новый драйвер ЦАП здесь не
 * подходит, потому что сам занимает модуль I2S0. Вывод становится
 * плавающей площадкой RTC без подтяжек, канал ЦАП включается
 * принудительно, а конечный автомат АЦП1 перестаёт управлять питанием
 * ЦАП.
 *
 * @param gpio GPIO25 (ЦАП 1) или GPIO26 (ЦАП 2).
 */
void esp::sound::p::dac_enable_i2s_output(gpio_num_t gpio)
{
    // Тактовый сигнал и данные ЦАП — от I2S0.
    SENS.sar_dac_ctrl1.dac_dig_force = 1;
    SENS.sar_dac_ctrl1.dac_clk_inv = 1;

    // Вывод ЦАП — плавающая площадка RTC.
    rtc_gpio_init(gpio);
    rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_DISABLED);
    rtc_gpio_pullup_dis(gpio);
    rtc_gpio_pulldown_dis(gpio);

    // Питание канала ЦАП.
    const int channel = gpio == GPIO_NUM_25 ? 0 : 1;
    RTCIO.pad_dac[channel].dac_xpd_force = 1;
    RTCIO.pad_dac[channel].xpd_dac = 1;

    // Выборки АЦП1 не должны управлять питанием ЦАП.
    SENS.sar_meas_ctrl2.sar1_dac_xpd_fsm = 0;
}

/**
 * @brief Подбирает делители тактовой частоты I2S для частоты дискретизации.
 *
 * Частота дискретизации получается как
 * @c APB_CLK_FREQ / (N + B / A) / M, где @c APB_CLK_FREQ = 80 МГц. Сначала
 * M растёт, пока N не станет меньше 255, затем перебором A от 1 до 63
 * подбирается дробная часть B / A, ближайшая к нужной. Способ взят из
 * статьи
 * https://linuxos.sk/blog/mirecove-dristy/detail/esp32-dynamicka-zmena-vzorkovacej-frekvencie-/
 *
 * @param sample_rate Нужная частота дискретизации, Гц.
 * @param out_a Знаменатель A дробной части делителя, от 1 до 63.
 * @param out_b Числитель B дробной части делителя, от 0 до 63.
 * @param out_n Целая часть N делителя, от 2 до 254.
 * @param out_m Делитель M частоты BCK, от 1 до 63.
 * @return Действительная частота дискретизации, Гц.
 */
int esp::sound::p::calc_i2s_timing_params(int sample_rate, int *out_a, int *out_b, int *out_n,
                                          int *out_m)
{
    *out_m = 1;

    double n = static_cast<double>(APB_CLK_FREQ) / sample_rate;
    while (n >= 255)
    {
        ++(*out_m);
        n = static_cast<double>(APB_CLK_FREQ) / sample_rate / *out_m;
    }

    double min_error = 1.0;
    *out_n = n;
    *out_b = 0;
    *out_a = 1;

    for (int a = 1; a < 64; ++a)
    {
        // Два ближайших к нужной дробной части числителя: с округлением
        // вниз и вверх.
        int b = (n - static_cast<double>(*out_n)) * static_cast<double>(a);
        if (b > 63)
            continue;

        double divisor =
            static_cast<double>(*out_n) + static_cast<double>(b) / static_cast<double>(a);
        double error = divisor > n ? divisor - n : n - divisor;
        if (error < min_error)
        {
            min_error = error;
            *out_a = a;
            *out_b = b;
        }

        ++b;
        if (b > 63)
            continue;
        divisor = static_cast<double>(*out_n) + static_cast<double>(b) / static_cast<double>(a);
        error = divisor > n ? divisor - n : n - divisor;
        if (error < min_error)
        {
            min_error = error;
            *out_a = a;
            *out_b = b;
        }
    }

    return APB_CLK_FREQ / (static_cast<double>(*out_n) + static_cast<double>(*out_b) / (*out_a))
        / *out_m;
}
