// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_non_copyable.h"
#include "esp_sound_config.h"
#include "esp_sound_samples_generator.h"
#include "esp_sound_sound_gen_method.h"
#include "esp_sound_waveform_generator.h"
#include "esp_tools.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <driver/gpio.h>
#include <driver/sdm.h>
#include <esp32/rom/lldesc.h>
#include <esp_intr_alloc.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>


namespace esp::sound
{

class sound_generator : esp::non_copyable
{

public:

    explicit sound_generator(int sample_rate = DEFAULT_SAMPLE_RATE,
                             gpio_num_t gpio = esp::GPIO_AUTO,
                             sound_gen_method gen_method = sound_gen_method::automatic);
    ~sound_generator();

    void init();
    void clear();

    bool play(bool value);
    inline bool playing() const;

    samples_generator *play_samples(const std::int8_t *data, int length, int volume = 100,
                                    int duration_ms = 0);

    template<std::derived_from<esp::sound::waveform_generator> T>
    inline bool play_sound(const T &waveform, int frequency, int duration_ms, int volume = 100);

    void attach(waveform_generator *value);
    void detach(waveform_generator *value);
    inline waveform_generator *channels() const;

    inline void set_volume(int value);
    inline int volume() const;

private:

    static void isr_handler(void *arg);
    static void timer_handler(void *arg);

    void dac_init();
    void sigmadelta_init();
    void set_dma_node(int index, volatile std::uint16_t *buf, int len);
    int available_voice();
    void start_voice(int index, waveform_generator *generator);
    bool has_channel(const waveform_generator *value) const;
    void remove_channel(waveform_generator *value);
    int sample();

    /**
     * @brief Голова списка каналов — подключённых генераторов волны.
     *
     * Список односвязный, ссылки хранят сами генераторы
     * (esp::sound::waveform_generator::next()). Новый генератор встаёт в
     * начало. Список обходит sample() — в режиме ЦАП из обработчика
     * прерывания, — поэтому и читают, и меняют его только под
     * спин-блокировкой @c _lock.
     */
    waveform_generator *_channels;

    /**
     * @brief Два буфера отсчётов для DMA в режиме ЦАП.
     *
     * По @c esp::sound::SAMPLE_BUFFER_SIZE 16-разрядных отсчётов в памяти,
     * доступной DMA. Пока I2S0 выводит один буфер, обработчик прерывания
     * заполняет другой. В режиме сигма-дельта не выделяются.
     */
    std::uint16_t *_sample_buffer[2];

    /**
     * @brief Общая громкость, от 0 до 127.
     *
     * Умножается на смесь всех каналов после того, как она ослаблена по
     * сумме громкостей каналов. По умолчанию 100.
     */
    std::int8_t _volume;

    /**
     * @brief Частота дискретизации, Гц.
     *
     * Сначала — заданная в конструкторе. В режиме ЦАП dac_init() заменяет
     * её действительной частотой, которую дают подобранные делители
     * тактовой частоты I2S. Подключаемые генераторы волны получают это
     * значение.
     */
    std::uint16_t _sample_rate;

    /**
     * @brief Выводится ли звук.
     *
     * Меняется только в play(): @c true между play(true) и play(false).
     */
    bool _play;

    /**
     * @brief Вывод звука.
     *
     * До первого запуска может быть @c esp::GPIO_AUTO; init() заменяет его на
     * GPIO25 для ЦАП или GPIO23 для сигма-дельта-модулятора.
     */
    gpio_num_t _gpio;

    /**
     * @brief Прерывание I2S0 в режиме ЦАП.
     *
     * Выделяется в dac_init() и освобождается деструктором; ненулевое
     * значение и означает для деструктора, что работал режим ЦАП.
     */
    intr_handle_t _isr_handle;

    /**
     * @brief Два дескриптора DMA, замкнутые в кольцо, — по одному на буфер отсчётов.
     *
     * Лежат в памяти, доступной DMA; их читает и меняет оборудование,
     * поэтому указатель на @c volatile. Поле @c sosf второго дескриптора
     * равно единице: по нему обработчик прерывания узнаёт, какой буфер
     * выведен.
     */
    volatile lldesc_t *_dma_chain;

    /**
     * @brief Способ вывода звука.
     *
     * До первого запуска может быть @c sound_gen_method::automatic; init()
     * заменяет его на ЦАП или сигма-дельта-модулятор по текущему
     * видеовыходу.
     */
    sound_gen_method _gen_method;

    /**
     * @brief Настроено ли оборудование.
     *
     * init() настраивает его один раз, при первом запуске звука, и ставит
     * этот флаг.
     */
    bool _init_done;

    /**
     * @brief Таймер esp_timer в режиме сигма-дельта.
     *
     * Срабатывает с частотой дискретизации, и timer_handler() подаёт
     * модулятору очередной отсчёт. Создаётся в sigmadelta_init(),
     * удаляется деструктором.
     */
    esp_timer_handle_t _timer_handle;

    /**
     * @brief Канал сигма-дельта-модулятора в режиме сигма-дельта.
     *
     * Создаётся в sigmadelta_init() на выводе @c _gpio и освобождается
     * деструктором; в режиме ЦАП остаётся @c nullptr.
     */
    sdm_channel_handle_t _sdm_channel;

    /**
     * @brief Спин-блокировка списка каналов и голосов.
     *
     * Обработчик прерывания держит её, пока заполняет буфер, обработчик
     * таймера — пока получает отсчёт, а методы, меняющие список, — пока
     * меняют его. Обработчик может работать на другом ядре, и без
     * блокировки задача могла бы изменить список или занять голос под
     * новый звук, пока sample() их обходит.
     */
    portMUX_TYPE _lock;

    /**
     * @brief Генераторы волны в голосах; @c nullptr — голос пуст.
     *
     * Генератор голоса создают play_sound() и play_samples() в
     * @c _voice_storage. Голос занят, пока его генератор в списке каналов;
     * отзвучавший генератор убирается из списка автоотключением, и его голос
     * достаётся следующему звуку (available_voice()).
     */
    std::array<waveform_generator *, VOICE_COUNT> _voices;

    /**
     * @brief Место под генераторы голосов, по @c esp::sound::VOICE_SIZE байтов на голос.
     *
     * Выровнено для любого типа. Генератор голоса создаётся здесь
     * @c std::construct_at() и разрушается @c std::destroy_at(), когда голос
     * занимает новый звук или clear() освобождает все голоса, — без
     * динамической памяти.
     */
    alignas(std::max_align_t) std::byte _voice_storage[VOICE_COUNT][VOICE_SIZE];
};

}


// Реализация

// Открытые

/**
 * @brief Сообщает, выводится ли звук.
 * @return @c true между play(true) и play(false).
 */
bool esp::sound::sound_generator::playing() const
{
    return _play;
}

/**
 * @brief Воспроизводит волну заданной частоты и длительности.
 *
 * Создаёт в свободном голосе копию @p waveform, подключает её и запускает
 * звук, не дожидаясь его конца; play() вызывать не нужно. Когда
 * длительность истекает, генератор отключается, и его голос достаётся
 * следующему звуку. Звуки, запущенные один за другим, звучат
 * одновременно — так строятся аккорды; одновременно звучат не больше
 * @c esp::sound::VOICE_COUNT таких звуков.
 *
 * @code
 * // Синусоида 500 Гц, 200 мс.
 * sound.play_sound(esp::sound::sine_waveform_generator(), 500, 200);
 *
 * // До-мажорное трезвучие на секунду.
 * sound.play_sound(esp::sound::sine_waveform_generator(), 262, 1000); // До.
 * sound.play_sound(esp::sound::sine_waveform_generator(), 330, 1000); // Ми.
 * sound.play_sound(esp::sound::sine_waveform_generator(), 392, 1000); // Соль.
 * @endcode
 *
 * @tparam T Класс генератора волны; его объект должен помещаться в голос
 *         (@c esp::sound::VOICE_SIZE байтов).
 * @param waveform Образец генератора; копируется.
 * @param frequency Частота волны, Гц.
 * @param duration_ms Длительность, мс.
 * @param volume Громкость, от 0 до 127.
 * @return @c false, если свободных голосов нет и звук не запущен.
 */
template<std::derived_from<esp::sound::waveform_generator> T>
bool esp::sound::sound_generator::play_sound(const T &waveform, int frequency, int duration_ms,
                                             int volume)
{
    static_assert(sizeof(T) <= VOICE_SIZE,
                  "the waveform generator does not fit into a voice: raise esp::sound::VOICE_SIZE");
    static_assert(alignof(T) <= alignof(std::max_align_t),
                  "the waveform generator needs a stricter alignment than a voice provides");

    const int index = available_voice();
    if (index < 0)
        return false;

    T *const generator = std::construct_at(reinterpret_cast<T *>(_voice_storage[index]), waveform);
    // Генератор настраивается до подключения, чтобы обработчик прерывания
    // не застал его настроенным наполовину. Частота дискретизации нужна
    // set_frequency() для приращения фазы.
    generator->set_sample_rate(_sample_rate);
    generator->set_frequency(frequency);
    generator->set_volume(volume);
    generator->set_duration(_sample_rate / 1000 * duration_ms);
    start_voice(index, generator);
    return true;
}

/**
 * @brief Возвращает голову списка каналов.
 *
 * Следующие генераторы списка дают esp::sound::waveform_generator::next().
 * Список меняет и обработчик прерывания, поэтому обходить его можно, только
 * пока звук остановлен.
 *
 * @return Последний подключённый генератор волны; @c nullptr, если
 *         подключённых нет.
 */
esp::sound::waveform_generator *esp::sound::sound_generator::channels() const
{
    return _channels;
}

/**
 * @brief Задаёт общую громкость.
 *
 * Общая громкость умножается на смесь всех каналов; громкость каждого
 * канала задаёт его генератор (esp::sound::waveform_generator::set_volume()).
 *
 * @param value Громкость, от 0 до 127; по умолчанию 100.
 */
void esp::sound::sound_generator::set_volume(int value)
{
    _volume = value;
}

/**
 * @brief Возвращает общую громкость.
 * @return Громкость, от 0 до 127.
 */
int esp::sound::sound_generator::volume() const
{
    return _volume;
}
