#pragma once


/**
 * @file
 * @brief Проверки на ПК: счёт проверок esp::test::context и макросы ESP_CHECK, ESP_CHECK_EQUAL.
 */


#include <cstdio>
#include <string>


namespace esp::test
{

/**
 * @brief Ведёт счёт проверкам и печатает неудачные.
 *
 * Набор проверок получает контекст, называет каждый случай set_case() и
 * проверяет условия макросами @c ESP_CHECK и @c ESP_CHECK_EQUAL. Неудача
 * не прерывает набор: печатаются файл, строка, случай и проверка, и
 * проверки идут дальше.
 */
class context
{

public:

    inline void set_case(const char *name);
    inline void check(bool is_true, const char *expression, const char *file, int line);
    inline void check_equal(long long actual, long long expected, const char *expression,
                            const char *file, int line);
    inline void check_equal(const std::string &actual, const std::string &expected,
                            const char *expression, const char *file, int line);
    inline int checks() const;
    inline int failures() const;

private:

    /** @brief Имя текущего случая для сообщений о неудаче. */
    const char *_case = "";

    /** @brief Сколько всего было проверок. */
    int _checks = 0;

    /** @brief Сколько проверок не прошло. */
    int _failures = 0;
};

}

/** @brief Проверяет условие @p expression в контексте @p context. */
#define ESP_CHECK(context, expression) \
    (context).check((expression), #expression, __FILE__, __LINE__)

/** @brief Проверяет, что @p actual равно @p expected; при неудаче печатает оба значения. */
#define ESP_CHECK_EQUAL(context, actual, expected) \
    (context).check_equal((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)


// Реализация

// Открытые

/**
 * @brief Называет текущий случай.
 * @param name Имя случая; строка должна жить до следующего вызова.
 */
void esp::test::context::set_case(const char *name)
{
    _case = name;
}

/**
 * @brief Проверяет условие.
 * @param is_true Значение условия.
 * @param expression Текст условия для сообщения.
 * @param file Файл проверки.
 * @param line Строка проверки.
 */
void esp::test::context::check(bool is_true, const char *expression, const char *file, int line)
{
    ++_checks;
    if (is_true)
        return;
    ++_failures;
    std::printf("FAIL %s:%d [%s] %s\n", file, line, _case, expression);
}

/**
 * @brief Проверяет равенство чисел.
 * @param actual Полученное значение.
 * @param expected Ожидаемое значение.
 * @param expression Текст сравнения для сообщения.
 * @param file Файл проверки.
 * @param line Строка проверки.
 */
void esp::test::context::check_equal(long long actual, long long expected, const char *expression,
                                     const char *file, int line)
{
    ++_checks;
    if (actual == expected)
        return;
    ++_failures;
    std::printf("FAIL %s:%d [%s] %s: got %lld, expected %lld\n", file, line, _case, expression,
                actual, expected);
}

/**
 * @brief Проверяет равенство строк.
 * @param actual Полученная строка.
 * @param expected Ожидаемая строка.
 * @param expression Текст сравнения для сообщения.
 * @param file Файл проверки.
 * @param line Строка проверки.
 */
void esp::test::context::check_equal(const std::string &actual, const std::string &expected,
                                     const char *expression, const char *file, int line)
{
    ++_checks;
    if (actual == expected)
        return;
    ++_failures;
    std::printf("FAIL %s:%d [%s] %s: got \"%s\", expected \"%s\"\n", file, line, _case, expression,
                actual.c_str(), expected.c_str());
}

/**
 * @brief Возвращает число проверок.
 * @return Сколько всего было проверок.
 */
int esp::test::context::checks() const
{
    return _checks;
}

/**
 * @brief Возвращает число неудачных проверок.
 * @return Сколько проверок не прошло.
 */
int esp::test::context::failures() const
{
    return _failures;
}
