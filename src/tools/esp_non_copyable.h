#pragma once


/**
 * @file
 * @brief Основа некопируемых классов esp::non_copyable.
 */


namespace esp
{

/**
 * @brief Основа для классов, объекты которых нельзя копировать и перемещать.
 *
 * Так объявляются классы, владеющие оборудованием или памятью: буфером DMA,
 * обработчиком прерывания, задачей FreeRTOS. Копия такого объекта
 * освободила бы тот же ресурс второй раз. Достаточно унаследовать
 * @c esp::non_copyable вместо того, чтобы объявлять в каждом классе четыре
 * удалённые функции копирования и перемещения: у основы они закрыты и
 * удалены, поэтому и производный класс их не получает.
 *
 * Основа пустая и размер объекта не увеличивает.
 */
class non_copyable
{

protected:

    non_copyable() = default;
    ~non_copyable() = default;

private:

    non_copyable(const non_copyable &) = delete;
    non_copyable &operator=(const non_copyable &) = delete;
    non_copyable(non_copyable &&) = delete;
    non_copyable &operator=(non_copyable &&) = delete;
};

}
