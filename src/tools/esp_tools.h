// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>


namespace esp
{

/**
 * @brief Ядро видеовывода.
 *
 * Контроллеры VGA ставят на это ядро обработчик прерывания I2S1, который
 * готовит строки развёртки, и отмечают его в esp::core_usage как самое
 * загруженное. Это ядро, на котором не работает задача Wi-Fi: она
 * задерживала бы обработчик прерывания, и изображение мерцало бы.
 *
 * Ядро задачи Wi-Fi выбирают в настройках ESP-IDF параметром
 * @c CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1 (по умолчанию задача Wi-Fi
 * работает на ядре 0). Из этого же параметра номер ядра берёт заголовок
 * esp_wifi.h, поэтому компонент esp_wifi программе не нужен.
 */
#if CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1
constexpr int VIDEO_TASKS_CORE = 0;
#else
constexpr int VIDEO_TASKS_CORE = 1;
#endif

/**
 * @brief Номер вывода со смыслом «вывод не используется».
 *
 * Равен @c GPIO_NUM_MAX и поэтому не совпадает ни с одним настоящим
 * выводом. Его передают вместо вывода, который не нужен: второго порта
 * esp::ps2::ps2_controller или выхода делителя частоты
 * esp::vga::gpio_stream; его же возвращает esp::int2gpio() для номера -1.
 */
constexpr gpio_num_t GPIO_UNUSED = GPIO_NUM_MAX;

/**
 * @brief Номер вывода со смыслом «выбрать вывод автоматически».
 *
 * Следует за esp::GPIO_UNUSED и тоже не совпадает ни с одним настоящим
 * выводом. Генератор звука esp::sound::sound_generator при первом запуске
 * заменяет его выводом по умолчанию для своего способа вывода звука.
 */
constexpr gpio_num_t GPIO_AUTO = static_cast<gpio_num_t>(GPIO_NUM_MAX + 1);

TickType_t ms_to_ticks(int ms);

void i2s_module_enable(int num, bool enable);

[[nodiscard]] esp_err_t esp_intr_alloc_pinned_to_core(int source, int flags, intr_handler_t handler,
                                                      void *arg, intr_handle_t *ret_handle,
                                                      int core);

}
