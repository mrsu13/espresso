// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_tools.h
 * @brief Объявление общих вспомогательных функций и констант пространства esp.
 */

/**
 * @file esp_tools.cpp
 * @brief Общие вспомогательные функции: тики FreeRTOS, включение модулей
 *        I2S, выделение прерывания на заданном ядре.
 */

#include "esp_tools.h"

#include <cstdint>

#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <esp_ipc.h>
#include <esp_private/periph_ctrl.h>
#include <freertos/FreeRTOS.h>
#include <soc/dport_reg.h>


#pragma GCC optimize("O2")


namespace esp::p
{

/**
 * @brief Параметры esp_intr_alloc(), передаваемые на другое ядро.
 *
 * esp::esp_intr_alloc_pinned_to_core() заполняет структуру на своём стеке и
 * ждёт, пока esp::p::intr_alloc() выполнится на нужном ядре, поэтому
 * указатель на неё действителен всё время вызова.
 */
struct intr_alloc_args
{
    /** @brief Источник прерывания (@c ETS_*_INTR_SOURCE). */
    int source;

    /** @brief Флаги @c ESP_INTR_FLAG_*: уровень прерывания, размещение обработчика в IRAM. */
    int flags;

    /** @brief Обработчик прерывания. */
    intr_handler_t handler;

    /** @brief Аргумент обработчика. */
    void *arg;

    /** @brief Куда записать дескриптор прерывания; может быть @c nullptr. */
    intr_handle_t *ret_handle;

    /** @brief Результат esp_intr_alloc(); его записывает esp::p::intr_alloc(). */
    esp_err_t result;
};

void intr_alloc(void *arg);

}


// Открытые

/**
 * @brief Переводит миллисекунды в тики FreeRTOS для функций ожидания.
 * @param ms Время ожидания, мс; отрицательное — ждать без ограничения.
 * @return Число тиков; @c portMAX_DELAY, если @p ms отрицательно.
 */
TickType_t esp::ms_to_ticks(int ms)
{
    return ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(ms);
}

/**
 * @brief Включает или выключает модуль I2S.
 *
 * Включение подаёт на модуль тактовую частоту шины и снимает с него сброс,
 * выключение снимает тактовую частоту и переводит модуль в сброс. Модуль
 * I2S0 выводит звук на ЦАП (esp::sound::sound_generator), I2S1 — поток
 * разрядов VGA (esp::vga::gpio_stream).
 *
 * @param num Номер модуля: 0 — I2S0, 1 — I2S1.
 * @param enable @c true — включить, @c false — выключить.
 */
void esp::i2s_module_enable(int num, bool enable)
{
    // Модуля PERIPH_I2S1_MODULE для periph_module_enable() в ESP-IDF больше
    // нет, поэтому разряды тактирования и сброса меняются прямо в регистрах
    // DPORT — в критической секции, общей с драйверами ESP-IDF.
    const std::uint32_t clk = num == 0 ? DPORT_I2S0_CLK_EN : DPORT_I2S1_CLK_EN;
    const std::uint32_t rst = num == 0 ? DPORT_I2S0_RST : DPORT_I2S1_RST;
    PERIPH_RCC_ATOMIC()
    {
        if (enable)
        {
            DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, clk);
            DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, rst);
        }
        else
        {
            DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, clk);
            DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, rst);
        }
    }
}

/**
 * @brief Выделяет прерывание на заданном ядре.
 *
 * esp_intr_alloc() ставит обработчик на то ядро, на котором её вызвали,
 * поэтому функция вызывает её на ядре @p core через IPC и ждёт завершения.
 * Так обработчик прерывания VGA попадает на ядро видеовывода
 * (esp::VIDEO_TASKS_CORE), а обработчик звука — на менее загруженное ядро
 * (esp::core_usage::quiet_core()).
 *
 * @param source Источник прерывания (@c ETS_*_INTR_SOURCE).
 * @param flags Флаги @c ESP_INTR_FLAG_*: уровень прерывания, размещение
 *        обработчика в IRAM.
 * @param handler Обработчик прерывания.
 * @param arg Аргумент обработчика.
 * @param ret_handle Куда записать дескриптор прерывания для
 *        esp_intr_free(); может быть @c nullptr.
 * @param core Ядро: 0 или 1.
 * @return @c ESP_OK или код ошибки: esp_ipc_call_blocking() возвращает
 *         @c ESP_ERR_INVALID_ARG при неверном номере ядра, esp_intr_alloc()
 *         — например, @c ESP_ERR_NOT_FOUND, если свободного прерывания с
 *         такими флагами нет. При ошибке @p ret_handle не меняется.
 */
esp_err_t esp::esp_intr_alloc_pinned_to_core(int source, int flags, intr_handler_t handler,
                                             void *arg, intr_handle_t *ret_handle, int core)
{
    esp::p::intr_alloc_args args{.source = source,
                                 .flags = flags,
                                 .handler = handler,
                                 .arg = arg,
                                 .ret_handle = ret_handle,
                                 .result = ESP_FAIL};
    const esp_err_t ipc_result = esp_ipc_call_blocking(core, esp::p::intr_alloc, &args);
    return ipc_result != ESP_OK ? ipc_result : args.result;
}


// Закрытые

/**
 * @brief Выделяет прерывание на текущем ядре.
 *
 * Функция IPC для esp::esp_intr_alloc_pinned_to_core(): выполняется на
 * выбранном ядре, вызывает esp_intr_alloc() с параметрами из @p arg и
 * записывает туда же её результат.
 *
 * @param arg Параметры: указатель на esp::p::intr_alloc_args.
 */
void esp::p::intr_alloc(void *arg)
{
    esp::p::intr_alloc_args *args = static_cast<esp::p::intr_alloc_args *>(arg);
    args->result =
        esp_intr_alloc(args->source, args->flags, args->handler, args->arg, args->ret_handle);
}
