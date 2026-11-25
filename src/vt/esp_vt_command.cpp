/**
 * @file esp_vt_command.h
 * @brief Объявление разобранной последовательности esp::vt::command.
 */

/**
 * @file esp_vt_command.cpp
 * @brief Разобранная последовательность CSI или ESC esp::vt::command.
 */

#include "esp_vt_command.h"

#include <cstddef>


/**
 * @struct esp::vt::command
 * @brief Параметры, промежуточные байты и последний байт последовательности CSI или ESC.
 *
 * Запись заполняет разборщик esp::vt::parser по мере прихода байтов и
 * передаёт исполнителю esp::vt::interpreter, когда последовательность
 * завершена. Запись действительна только до возврата из вызова исполнителя:
 * нужное он копирует себе.
 */


// Открытые

/**
 * @brief Очищает запись перед новой последовательностью.
 */
void esp::vt::command::clear()
{
    param_count = 0;
    intermediate_count = 0;
    private_marker = 0;
    final_byte = 0;
}

/**
 * @brief Возвращает параметр как есть.
 * @param index Номер параметра, с 0.
 * @return Значение параметра или @c -1, если параметр пропущен или его нет.
 */
int esp::vt::command::param(int index) const
{
    if (index < 0 || index >= param_count)
        return -1;
    return params[static_cast<std::size_t>(index)];
}

/**
 * @brief Возвращает параметр, считая пропущенный нулём.
 *
 * Так читают номера режимов и способы стирания (@c CSI J, @c CSI K), у
 * которых пропущенный параметр означает 0.
 *
 * @param index Номер параметра, с 0.
 * @return Значение параметра; 0, если параметр пропущен или его нет.
 */
int esp::vt::command::param_or_0(int index) const
{
    const int value = param(index);
    return value < 0 ? 0 : value;
}

/**
 * @brief Возвращает параметр, считая пропущенный или нулевой единицей.
 *
 * Так читают счётчики и координаты: @c CSI A без параметра и @c CSI 0A
 * сдвигают курсор на одну строку, @c CSI H ставит его в строку 1.
 *
 * @param index Номер параметра, с 0.
 * @return Значение параметра; 1, если параметр пропущен, равен нулю или его нет.
 */
int esp::vt::command::param_or_1(int index) const
{
    const int value = param(index);
    return value <= 0 ? 1 : value;
}
