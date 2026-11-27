#pragma once

#include <driver/gpio.h>


namespace esp
{

class button
{

public:

    button(gpio_num_t gpio, const char *name);

    bool pressed();

    inline bool is_down() const;
    inline gpio_num_t gpio() const;
    inline const char *name() const;

private:

    /** @brief Сколько опросов подряд должно держаться новое состояние, чтобы его принять. */
    static constexpr int DEBOUNCE_SAMPLES = 3;

    /** @brief Вывод кнопки. */
    gpio_num_t _gpio;

    /** @brief Имя кнопки для сообщений в консоли. */
    const char *_name;

    /** @brief Принятое состояние кнопки: @c true, пока она нажата. */
    bool _down;

    /** @brief Сколько опросов подряд уровень на выводе отличается от принятого состояния. */
    int _count;
};

}


// Реализация

/**
 * @brief Сообщает, нажата ли кнопка.
 *
 * Состояние — принятое после подавления дребезга, его обновляет pressed():
 * по нему видно, сколько кнопку держат.
 *
 * @return @c true, пока кнопка нажата.
 */
bool esp::button::is_down() const
{
    return _down;
}

/**
 * @brief Возвращает вывод кнопки.
 * @return Номер вывода.
 */
gpio_num_t esp::button::gpio() const
{
    return _gpio;
}

/**
 * @brief Возвращает имя кнопки.
 * @return Имя кнопки для сообщений в консоли, например «Left».
 */
const char *esp::button::name() const
{
    return _name;
}
