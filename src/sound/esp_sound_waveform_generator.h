// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Основа генераторов волны esp::sound::waveform_generator.
 */


#include <cstdint>
#include <limits>


namespace esp::sound
{

/**
 * @brief Основа генераторов волны — звуковых каналов генератора звука.
 *
 * Генератор волны выдаёт по отсчёту на каждый вызов sample(): целое со
 * знаком от −128 до 127, уже умноженное на громкость генератора. Генератор
 * звука esp::sound::sound_generator держит подключённые к нему генераторы
 * волны в односвязном списке (ссылка на следующий — next()) и на каждый
 * свой отсчёт смешивает отсчёты всех включённых генераторов.
 *
 * Новый генератор выключен и звучит только после enable(). Длительность
 * считается в отсчётах: когда она истекает, генератор выключается сам.
 * Выключенный генератор с истёкшей длительностью генератор звука отключает
 * от себя, если задано автоотключение (set_auto_detach()).
 *
 * Производные классы копируются: так esp::sound::sound_generator::play_sound()
 * получает собственный генератор из образца.
 */
class waveform_generator
{

public:

    virtual ~waveform_generator() = default;

    virtual void set_frequency(int value) = 0;
    virtual int sample() = 0;

    inline void enable(bool value);
    inline bool enabled() const;

    inline void set_volume(int value);
    inline int volume() const;

    inline void set_duration(std::uint32_t value);
    inline std::uint32_t duration() const;

    inline void set_auto_detach(bool value);
    inline bool auto_detach() const;

    inline void set_sample_rate(int value);
    inline std::uint16_t sample_rate() const;

    inline void set_next(waveform_generator *value);
    inline waveform_generator *next() const;

protected:

    inline waveform_generator();

    inline void dec_duration();

private:

    /**
     * @brief Следующий генератор в списке каналов генератора звука.
     *
     * Список односвязный, его голова — esp::sound::sound_generator::channels().
     * Ссылку меняет только генератор звука при подключении и отключении;
     * у последнего генератора списка и у неподключённого — @c nullptr.
     */
    waveform_generator *_next;

    /**
     * @brief Частота дискретизации, Гц.
     *
     * Её задаёт генератор звука при подключении
     * (esp::sound::sound_generator::attach()); до подключения она равна нулю.
     * От неё зависит шаг фазы, который вычисляет set_frequency(), поэтому
     * частоту волны задают после подключения.
     */
    std::uint16_t _sample_rate;

    /**
     * @brief Громкость генератора, от 0 до 127.
     *
     * Производные классы умножают на неё каждый отсчёт и делят на 127.
     * Генератор звука, кроме того, складывает громкости включённых каналов,
     * чтобы ослабить смесь, когда их сумма больше 127. По умолчанию 100.
     */
    std::int8_t _volume;

    /**
     * @brief Включён ли генератор.
     *
     * Выключенный генератор не звучит: генератор звука не вызывает его
     * sample(). Новый генератор выключен; dec_duration() выключает его,
     * когда истекает длительность.
     */
    bool _enabled;

    /**
     * @brief Сколько отсчётов осталось выдать.
     *
     * Уменьшается на единицу на каждом выданном отсчёте (dec_duration()) и
     * на нуле выключает генератор. Начальное значение — наибольшее для
     * типа: практически бесконечная длительность, при частоте
     * дискретизации 16 кГц это больше трёх суток.
     */
    std::uint32_t _duration;

    /**
     * @brief Отключить ли генератор от генератора звука, когда истечёт длительность.
     *
     * Генератор звука проверяет этот флаг у выключенных генераторов с
     * нулевой длительностью и убирает их из списка каналов, не дожидаясь
     * вызова detach().
     */
    bool _auto_detach;
};

}


// Реализация

// Открытые

/**
 * @fn void esp::sound::waveform_generator::set_frequency(int value)
 * @brief Задаёт частоту волны.
 *
 * Смысл значения определяет производный класс: у генераторов
 * периодической волны это частота в герцах, у эмулятора шума VIC-20 —
 * значение регистра частоты, а генераторы шума и отсчётов частоту не
 * используют.
 *
 * @param value Частота волны.
 */

/**
 * @fn int esp::sound::waveform_generator::sample()
 * @brief Выдаёт следующий отсчёт.
 *
 * Генератор звука вызывает этот метод на каждый отсчёт вывода, в режиме
 * ЦАП — из обработчика прерывания, поэтому метод не должен выделять
 * память, ждать или писать в журнал. На каждом выданном отсчёте генератор
 * уменьшает оставшуюся длительность вызовом dec_duration().
 *
 * @return Отсчёт от −128 до 127 с учётом громкости генератора.
 */

/**
 * @brief Включает или выключает генератор.
 *
 * Новый генератор выключен и звучит только после включения. Генератор
 * выключается и сам, когда истекает его длительность.
 *
 * @param value @c true — включить, @c false — выключить.
 */
void esp::sound::waveform_generator::enable(bool value)
{
    _enabled = value;
}

/**
 * @brief Сообщает, включён ли генератор.
 * @return @c true, если генератор включён.
 */
bool esp::sound::waveform_generator::enabled() const
{
    return _enabled;
}

/**
 * @brief Задаёт громкость генератора.
 * @param value Громкость, от 0 до 127; по умолчанию 100.
 */
void esp::sound::waveform_generator::set_volume(int value)
{
    _volume = value;
}

/**
 * @brief Возвращает громкость генератора.
 * @return Громкость, от 0 до 127.
 */
int esp::sound::waveform_generator::volume() const
{
    return _volume;
}

/**
 * @brief Задаёт длительность в отсчётах.
 *
 * Отсчёт идёт от текущего момента: столько отсчётов генератор выдаст,
 * прежде чем выключится.
 *
 * @param value Число отсчётов; −1 (наибольшее значение типа) — практически
 *        бесконечно.
 */
void esp::sound::waveform_generator::set_duration(std::uint32_t value)
{
    _duration = value;
}

/**
 * @brief Возвращает оставшуюся длительность в отсчётах.
 * @return Сколько отсчётов осталось выдать; 0 — длительность истекла.
 */
std::uint32_t esp::sound::waveform_generator::duration() const
{
    return _duration;
}

/**
 * @brief Задаёт автоотключение.
 *
 * Генератор с автоотключением генератор звука убирает из списка каналов,
 * когда истекает длительность.
 *
 * @param value @c true — отключать по окончании.
 */
void esp::sound::waveform_generator::set_auto_detach(bool value)
{
    _auto_detach = value;
}

/**
 * @brief Сообщает, задано ли автоотключение.
 * @return @c true, если генератор отключается по окончании.
 */
bool esp::sound::waveform_generator::auto_detach() const
{
    return _auto_detach;
}

/**
 * @brief Задаёт частоту дискретизации.
 *
 * Вызывает генератор звука при подключении; самому задавать её не нужно.
 *
 * @param value Частота дискретизации, Гц, не больше 65535.
 */
void esp::sound::waveform_generator::set_sample_rate(int value)
{
    _sample_rate = value;
}

/**
 * @brief Возвращает частоту дискретизации.
 * @return Частота дискретизации, Гц; 0 — генератор ещё не подключён.
 */
std::uint16_t esp::sound::waveform_generator::sample_rate() const
{
    return _sample_rate;
}

/**
 * @brief Задаёт следующий генератор в списке каналов.
 *
 * Список ведёт генератор звука; другим классам менять ссылку нельзя.
 *
 * @param value Следующий генератор или @c nullptr.
 */
void esp::sound::waveform_generator::set_next(waveform_generator *value)
{
    _next = value;
}

/**
 * @brief Возвращает следующий генератор в списке каналов.
 * @return Следующий генератор; @c nullptr у последнего.
 */
esp::sound::waveform_generator *esp::sound::waveform_generator::next() const
{
    return _next;
}

// Защищённые

/**
 * @brief Создаёт выключенный неподключённый генератор.
 *
 * Громкость 100, длительность практически бесконечная, без
 * автоотключения.
 */
esp::sound::waveform_generator::waveform_generator()
    : _next{nullptr}
    , _sample_rate{0}
    , _volume{100}
    , _enabled{false}
    , _duration{std::numeric_limits<std::uint32_t>::max()}
    , _auto_detach{false}
{
}

/**
 * @brief Уменьшает оставшуюся длительность на один отсчёт.
 *
 * Производные классы вызывают метод из sample() на каждом выданном
 * отсчёте. Когда длительность доходит до нуля, генератор выключается.
 */
void esp::sound::waveform_generator::dec_duration()
{
    --_duration;
    if (!_duration)
        _enabled = false;
}
