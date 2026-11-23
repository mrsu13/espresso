// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


/**
 * @file esp_ps2_ps2_controller.h
 * @brief Объявление контроллера портов PS/2 esp::ps2::ps2_controller.
 */

/**
 * @file esp_ps2_ps2_controller.cpp
 * @brief Контроллер портов PS/2: программа сопроцессора ULP, приём и передача байтов.
 */

#include "esp_ps2_ps2_controller.h"

#include "esp_ps2_keyboard.h"
#include "esp_ps2_mouse.h"
#include "esp_ps2_ulp_macro_ex.h"
#include "esp_ps2_utils.h"
#include "esp_tools.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include <driver/rtc_io.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>
#include <soc/sens_reg.h>
#include <strings.h>
#if __has_include(<hal/rtc_io_periph.h>)
#   include <hal/rtc_io_periph.h>
#elif __has_include(<soc/rtc_io_periph.h>)
#   include <soc/rtc_io_periph.h>
#endif


#pragma GCC optimize("O2")
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"


// Программа сопроцессора ULP
//
// Сопроцессор ULP следит за линиями CLK и DATA обоих портов, принимает
// слова устройств и передаёт им команды. С процессором он обменивается
// через переменные в медленной памяти RTC (RTCMEM_*) и будит его
// прерыванием, когда что-то принял.
//
// Номера выводов портов известны только в begin(), поэтому команды,
// которые к ним обращаются, записаны в программе заглушками;
// esp::ps2::p::replace_placeholders() заменяет их настоящими командами.


// Заглушки

/** @brief Код операции заглушки: у ULP код 12 свободен. */
#define OPCODE_PLACEHOLDER 12

/** @brief Подкод заглушки DAT_ENABLE_OUTPUT(). */
#define SUB_OPCODE_DAT_ENABLE_OUTPUT 0
/** @brief Подкод заглушки DAT_ENABLE_INPUT(). */
#define SUB_OPCODE_DAT_ENABLE_INPUT 1
/** @brief Подкод заглушки CLK_ENABLE_OUTPUT(). */
#define SUB_OPCODE_CLK_ENABLE_OUTPUT 2
/** @brief Подкод заглушки CLK_ENABLE_INPUT(). */
#define SUB_OPCODE_CLK_ENABLE_INPUT 3
/** @brief Подкод заглушки READ_CLK(). */
#define SUB_OPCODE_READ_CLK 4
/** @brief Подкод заглушки READ_DAT(). */
#define SUB_OPCODE_READ_DAT 5
/** @brief Подкод заглушки WRITE_CLK(). */
#define SUB_OPCODE_WRITE_CLK 6
/** @brief Подкод заглушки WRITE_DAT(). */
#define SUB_OPCODE_WRITE_DAT 7

/** @brief Порт 0 в заглушках. */
#define PS2_PORT0 0
/** @brief Порт 1 в заглушках. */
#define PS2_PORT1 1


/** @brief Включить (value = 1) или выключить выход DAT. */
#define DAT_ENABLE_OUTPUT(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_DAT_ENABLE_OUTPUT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief Включить (value = 1) или выключить вход DAT. */
#define DAT_ENABLE_INPUT(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_DAT_ENABLE_INPUT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief Включить (value = 1) или выключить выход CLK. */
#define CLK_ENABLE_OUTPUT(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_CLK_ENABLE_OUTPUT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief Включить (value = 1) или выключить вход CLK. */
#define CLK_ENABLE_INPUT(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_CLK_ENABLE_INPUT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief R0 = CLK, как rtc_gpio_get_level(). */
#define READ_CLK(ps2port) \
    { \
        .macro = {.label = 0, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_READ_CLK, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief R0 = DAT, как rtc_gpio_get_level(). */
#define READ_DAT(ps2port) \
    { \
        .macro = {.label = 0, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_READ_DAT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief CLK = value, как rtc_gpio_set_level(). */
#define WRITE_CLK(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_WRITE_CLK, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }

/** @brief DAT = value, как rtc_gpio_set_level(). */
#define WRITE_DAT(ps2port, value) \
    { \
        .macro = {.label = value, \
                  .unused = ps2port, \
                  .sub_opcode = SUB_OPCODE_WRITE_DAT, \
                  .opcode = OPCODE_PLACEHOLDER} \
    }


// Составные команды

/** @brief DAT — только вход, как rtc_gpio_set_direction(..., RTC_GPIO_MODE_INPUT_ONLY). */
#define CONFIGURE_DAT_INPUT(ps2port) DAT_ENABLE_OUTPUT(ps2port, 0), DAT_ENABLE_INPUT(ps2port, 1)

/** @brief DAT — только выход, как rtc_gpio_set_direction(..., RTC_GPIO_MODE_OUTPUT_ONLY). */
#define CONFIGURE_DAT_OUTPUT(ps2port) DAT_ENABLE_OUTPUT(ps2port, 1), DAT_ENABLE_INPUT(ps2port, 0)

/** @brief CLK — только вход, как rtc_gpio_set_direction(..., RTC_GPIO_MODE_INPUT_ONLY). */
#define CONFIGURE_CLK_INPUT(ps2port) CLK_ENABLE_OUTPUT(ps2port, 0), CLK_ENABLE_INPUT(ps2port, 1)

/** @brief CLK — только выход, как rtc_gpio_set_direction(..., RTC_GPIO_MODE_OUTPUT_ONLY). */
#define CONFIGURE_CLK_OUTPUT(ps2port) CLK_ENABLE_OUTPUT(ps2port, 1), CLK_ENABLE_INPUT(ps2port, 0)

/** @brief DAT = разряд 0 регистра R0, как rtc_gpio_set_level(). */
#define WRITE_DAT_R0(ps2port) I_BL(3, 1), WRITE_DAT(ps2port, 1), I_BGE(2, 1), WRITE_DAT(ps2port, 0)

/** @brief M_BGE с дальним переходом. */
#define M_LONG_BGE(label, value) I_BL(2, value), M_BX(label)

/** @brief M_BL с дальним переходом. */
#define M_LONG_BL(label, value) I_BGE(2, value), M_BX(label)

/** @brief Задержка в микросекундах: ULP работает на 8 МГц, микросекунда — 8 тактов. */
#define M_DELAY_US(us) I_DELAY(us * 8)


// Адреса в медленной памяти RTC, в 32-разрядных словах

/** @brief Начало программы. */
#define RTCMEM_PROG_START 0x000
/** @brief Начало переменных. */
#define RTCMEM_VARS_START 0x200

// Команды: ставит процессор, сбрасывает ULP.

/** @brief 1 — передать данные в порт 0. */
#define RTCMEM_PORT0_TX (RTCMEM_VARS_START + 0)
/** @brief 1 — передать данные в порт 1. */
#define RTCMEM_PORT1_TX (RTCMEM_VARS_START + 1)
/** @brief 1 — включить приём порта 0. */
#define RTCMEM_PORT0_RX_ENABLE (RTCMEM_VARS_START + 2)
/** @brief 1 — включить приём порта 1. */
#define RTCMEM_PORT1_RX_ENABLE (RTCMEM_VARS_START + 3)
/** @brief 1 — выключить приём порта 0. */
#define RTCMEM_PORT0_RX_DISABLE (RTCMEM_VARS_START + 4)
/** @brief 1 — выключить приём порта 1. */
#define RTCMEM_PORT1_RX_DISABLE (RTCMEM_VARS_START + 5)

// Параметры команд: ставит процессор.

/** @brief Слово для порта 0 (при RTCMEM_PORT0_TX = 1). */
#define RTCMEM_PORT0_DATAOUT (RTCMEM_VARS_START + 6)
/** @brief Слово для порта 1 (при RTCMEM_PORT1_TX = 1). */
#define RTCMEM_PORT1_DATAOUT (RTCMEM_VARS_START + 7)

// Флаги: ставит ULP и будит процессор, сбрасывает процессор.

/** @brief 1 — с порта 0 принято слово. */
#define RTCMEM_PORT0_RX (RTCMEM_VARS_START + 8)
/** @brief 1 — с порта 1 принято слово. */
#define RTCMEM_PORT1_RX (RTCMEM_VARS_START + 9)
/** @brief 1 — CLK порта 0 не ответил при приёме. */
#define RTCMEM_PORT0_RX_CLK_TIMEOUT (RTCMEM_VARS_START + 10)
/** @brief 1 — CLK порта 1 не ответил при приёме. */
#define RTCMEM_PORT1_RX_CLK_TIMEOUT (RTCMEM_VARS_START + 11)

// Параметры флагов: ставит ULP.

/** @brief Слово, принятое с порта 0 (при RTCMEM_PORT0_RX = 1). */
#define RTCMEM_PORT0_DATAIN (RTCMEM_VARS_START + 12)
/** @brief Слово, принятое с порта 1 (при RTCMEM_PORT1_RX = 1). */
#define RTCMEM_PORT1_DATAIN (RTCMEM_VARS_START + 13)

// Внутренние переменные ULP.

/** @brief 1 — порт 0 настроен на приём, 0 — нет. */
#define RTCMEM_PORT0_RX_ENABLED (RTCMEM_VARS_START + 14)
/** @brief 1 — порт 1 настроен на приём, 0 — нет. */
#define RTCMEM_PORT1_RX_ENABLED (RTCMEM_VARS_START + 15)

/** @brief Последняя переменная. */
#define RTCMEM_LASTVAR (RTCMEM_VARS_START + 15)


/**
 * @brief Наибольшее время между фронтами CLK при приёме, в итерациях цикла
 * ожидания (надёжный минимум — около 15).
 */
#define CLK_RX_TIMEOUT_VAL 100

/**
 * @brief Наибольшее время между фронтами CLK при передаче, в итерациях цикла
 * ожидания; подобрано под клавиатуру PERIBOARD 409P.
 */
#define CLK_TX_TIMEOUT_VAL 1200

/**
 * @brief Через сколько проходов цикла (счётчик R2) процессор будится снова, пока
 * порт выключен.
 */
#define WAKE_THRESHOLD 3000


// Программа и переменные должны поместиться в медленную память RTC.
#if RTCMEM_LASTVAR >= 0x800
#   error "ULP program too big"
#endif

/**
 * @brief Сколько байтов медленной памяти RTC нужно программе и переменным.
 *
 * С IDF 5 система хранит свои данные в медленной памяти RTC сразу за
 * областью, отведённой ULP. Программа и переменные должны оставаться в
 * этой области, иначе они затрут те данные.
 */
#define RTCMEM_REQUIRED_BYTES ((RTCMEM_LASTVAR + 1) * 4)
#if defined(CONFIG_ULP_COPROC_RESERVE_MEM) && CONFIG_ULP_COPROC_RESERVE_MEM < RTCMEM_REQUIRED_BYTES
#   error \
       "CONFIG_ULP_COPROC_RESERVE_MEM is too small for esp::ps2::ps2_controller: set it to at least 2112 (4096 suggested) in sdkconfig"
#endif


// Метки программы ULP

/** @brief Метка: главный цикл: ожидание команды. */
#define LABEL_WAIT_COMMAND 0

/** @brief Метка: проверка приёма с порта 0. */
#define LABEL_RX 1
/** @brief Метка: проверка приёма с порта 1. */
#define LABEL_RX_NEXT 2

/** @brief Метка: настройка порта 0 на приём. */
#define LABEL_PORT0_ENABLE_RX 3
/** @brief Метка: остановка приёма порта 0. */
#define LABEL_PORT0_STOP_RX 4
/** @brief Метка: передача в порт 0. */
#define LABEL_PORT0_TX 5
/** @brief Метка: передача следующего разряда в порт 0. */
#define LABEL_PORT0_TX_NEXT_BIT 6
/** @brief Метка: ожидание CLK = 1 при передаче в порт 0. */
#define LABEL_PORT0_TX_WAIT_CLK_HIGH 7
/** @brief Метка: конец передачи в порт 0. */
#define LABEL_PORT0_TX_EXIT 8
/** @brief Метка: ожидание смены CLK при приёме с порта 0. */
#define LABEL_PORT0_RX_WAIT_LOOP 9
/** @brief Метка: CLK = 1 при приёме с порта 0. */
#define LABEL_PORT0_RX_CLK_IS_HIGH 10
/** @brief Метка: CLK = 0 при приёме с порта 0: чтение DAT. */
#define LABEL_PORT0_RX_CLK_IS_LOW 11
/** @brief Метка: CLK порта 0 не ответил при приёме. */
#define LABEL_PORT0_RX_CLK_TIMEOUT 12
/** @brief Метка: проверка CLK порта 0: не начался ли приём. */
#define LABEL_PORT0_RX_CHECK_CLK 13

/** @brief Метка: настройка порта 1 на приём. */
#define LABEL_PORT1_ENABLE_RX 14
/** @brief Метка: остановка приёма порта 1. */
#define LABEL_PORT1_STOP_RX 15
/** @brief Метка: передача в порт 1. */
#define LABEL_PORT1_TX 16
/** @brief Метка: передача следующего разряда в порт 1. */
#define LABEL_PORT1_TX_NEXT_BIT 17
/** @brief Метка: ожидание CLK = 1 при передаче в порт 1. */
#define LABEL_PORT1_TX_WAIT_CLK_HIGH 18
/** @brief Метка: конец передачи в порт 1. */
#define LABEL_PORT1_TX_EXIT 19
/** @brief Метка: ожидание смены CLK при приёме с порта 1. */
#define LABEL_PORT1_RX_WAIT_LOOP 20
/** @brief Метка: CLK = 1 при приёме с порта 1. */
#define LABEL_PORT1_RX_CLK_IS_HIGH 21
/** @brief Метка: CLK = 0 при приёме с порта 1: чтение DAT. */
#define LABEL_PORT1_RX_CLK_IS_LOW 22
/** @brief Метка: CLK порта 1 не ответил при приёме. */
#define LABEL_PORT1_RX_CLK_TIMEOUT 23
/** @brief Метка: проверка CLK порта 1: не начался ли приём. */
#define LABEL_PORT1_RX_CHECK_CLK 24


// Вспомогательные команды

/** @brief Временно выключить порт 0, если он был включён; портит R0. */
#define TEMP_PORT0_DISABLE() \
    I_LD(R0, R3, RTCMEM_PORT0_RX_ENABLED), I_BL(4, 1), CONFIGURE_CLK_OUTPUT(PS2_PORT0), \
        WRITE_CLK(PS2_PORT0, 0)

/** @brief Снова включить порт 0, если он был включён; портит R0. */
#define TEMP_PORT0_ENABLE() \
    I_LD(R0, R3, RTCMEM_PORT0_RX_ENABLED), I_BL(3, 1), CONFIGURE_CLK_INPUT(PS2_PORT0)

/** @brief Временно выключить порт 1, если он был включён; портит R0. */
#define TEMP_PORT1_DISABLE() \
    I_LD(R0, R3, RTCMEM_PORT1_RX_ENABLED), I_BL(4, 1), CONFIGURE_CLK_OUTPUT(PS2_PORT1), \
        WRITE_CLK(PS2_PORT1, 0)

/** @brief Снова включить порт 1, если он был включён; портит R0. */
#define TEMP_PORT1_ENABLE() \
    I_LD(R0, R3, RTCMEM_PORT1_RX_ENABLED), I_BL(3, 1), CONFIGURE_CLK_INPUT(PS2_PORT1)

/** @brief Выключить порт 0 (CLK прижимается к нулю); портит R0. */
#define PERM_PORT0_DISABLE() \
    I_MOVI(R0, 0), I_ST(R0, R3, RTCMEM_PORT0_RX_ENABLED), CONFIGURE_CLK_OUTPUT(PS2_PORT0), \
        WRITE_CLK(PS2_PORT0, 0)

/** @brief Выключить порт 1 (CLK прижимается к нулю); портит R0. */
#define PERM_PORT1_DISABLE() \
    I_MOVI(R0, 0), I_ST(R0, R3, RTCMEM_PORT1_RX_ENABLED), CONFIGURE_CLK_OUTPUT(PS2_PORT1), \
        WRITE_CLK(PS2_PORT1, 0)


// Регистры ULP
//
// R0 — временный накопитель.
// R1 — временный регистр.
// R2 — в LABEL_PORT0_TX и LABEL_PORT1_TX передаваемое слово (на выходе
//      обнуляется); в LABEL_RX счётчик повторного пробуждения процессора,
//      пока порт выключен, а во время приёма — последнее значение CLK
//      (обнуляется в конце приёма или при тайм-ауте).
// R3 — база адресов переменных (0x0000); в LABEL_PORT0_TX и LABEL_PORT1_TX
//      временно счётчик ожидания CLK = 0, в LABEL_RX — счётчик ожидания
//      смены CLK.
// STAGE — счётчик разрядов при приёме и передаче.

namespace esp::ps2::p
{

/** @brief Тег журнала модуля. */
constexpr const char *TAG = "ps2";

/**
 * @brief Программа сопроцессора ULP; команды работы с выводами в ней — заглушки.
 *
 * esp::ps2::ps2_controller::begin() загружает её в медленную память RTC
 * и заменяет заглушки (replace_placeholders()).
 */
const ulp_insn_t ULP_CODE[] = {

    // Останавливаем таймер ULP; программа не кончается, так что это не обязательно.
    I_END(),

    // R3 всё время равен 0x0000 — это база адресов переменных: переменную
    // читает одна команда I_LD(R#, R3, RTCMEM_XX).
    I_MOVI(R3, 0x0000),


    // LABEL_WAIT_COMMAND — главный цикл: ожидание команды

    M_LABEL(LABEL_WAIT_COMMAND),

    // Передать в порт 0?
    I_LD(R0, R3, RTCMEM_PORT0_TX),
    M_BGE(LABEL_PORT0_TX, 1), // да — к LABEL_PORT0_TX

    // Включить приём порта 0?
    I_LD(R0, R3, RTCMEM_PORT0_RX_ENABLE),
    M_BGE(LABEL_PORT0_ENABLE_RX, 1), // да — к LABEL_PORT0_ENABLE_RX

    // Выключить приём порта 0?
    I_LD(R0, R3, RTCMEM_PORT0_RX_DISABLE),
    M_BGE(LABEL_PORT0_STOP_RX, 1), // да — к LABEL_PORT0_STOP_RX

    // Передать в порт 1?
    I_LD(R0, R3, RTCMEM_PORT1_TX),
    M_BGE(LABEL_PORT1_TX, 1), // да — к LABEL_PORT1_TX

    // Включить приём порта 1?
    I_LD(R0, R3, RTCMEM_PORT1_RX_ENABLE),
    M_BGE(LABEL_PORT1_ENABLE_RX, 1), // да — к LABEL_PORT1_ENABLE_RX

    // Выключить приём порта 1?
    I_LD(R0, R3, RTCMEM_PORT1_RX_DISABLE),
    M_BGE(LABEL_PORT1_STOP_RX, 1), // да — к LABEL_PORT1_STOP_RX

    // Проверяем приём с портов 0 и 1
    M_BX(LABEL_RX),


    // LABEL_PORT0_ENABLE_RX — настройка порта 0 на приём

    M_LABEL(LABEL_PORT0_ENABLE_RX),

    // Отмечаем, что порт 0 принимает
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT0_RX_ENABLED), // [RTCMEM_PORT0_RX_ENABLED] = 1

    // CLK и DAT — входы
    CONFIGURE_CLK_INPUT(PS2_PORT0),
    CONFIGURE_DAT_INPUT(PS2_PORT0),

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT0_RX_ENABLE), // [RTCMEM_PORT0_RX_ENABLE] = 0

    M_BX(LABEL_RX),


    // LABEL_PORT1_ENABLE_RX — настройка порта 1 на приём

    M_LABEL(LABEL_PORT1_ENABLE_RX),

    // Отмечаем, что порт 1 принимает
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT1_RX_ENABLED), // [RTCMEM_PORT1_RX_ENABLED] = 1

    // CLK и DAT — входы
    CONFIGURE_CLK_INPUT(PS2_PORT1),
    CONFIGURE_DAT_INPUT(PS2_PORT1),

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT1_RX_ENABLE), // [RTCMEM_PORT1_RX_ENABLE] = 0

    M_BX(LABEL_RX),


    // LABEL_PORT0_STOP_RX — остановка приёма порта 0 (CLK прижимается к нулю)

    M_LABEL(LABEL_PORT0_STOP_RX),

    PERM_PORT0_DISABLE(),

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT0_RX_DISABLE), // [RTCMEM_PORT0_RX_DISABLE] = 0

    M_BX(LABEL_RX),


    // LABEL_PORT1_STOP_RX — остановка приёма порта 1 (CLK прижимается к нулю)

    M_LABEL(LABEL_PORT1_STOP_RX),

    PERM_PORT1_DISABLE(),

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT1_RX_DISABLE), // [RTCMEM_PORT1_RX_DISABLE] = 0

    M_BX(LABEL_RX),


    // LABEL_PORT0_TX — передача в порт 0
    // В конце порт 0 сам включается на приём.

    M_LABEL(LABEL_PORT0_TX),

    // Передаём слово из RTCMEM_PORT0_DATAOUT

    // R2 — слово для передачи (10 разрядов: данные, чётность и стоповый разряд)
    I_LD(R2, R3, RTCMEM_PORT0_DATAOUT), // R2 = [RTCMEM_PORT0_DATAOUT]

    // Обнуляем счётчик разрядов (STAGE: 0...7 — данные, 8 — чётность, 9 — стоповый разряд)
    I_STAGERSTI(), // STAGE = 0

    // Временно выключаем порт 1, если он включён
    TEMP_PORT1_DISABLE(),

    // Держим CLK и DAT в нуле 200 мкс
    CONFIGURE_CLK_OUTPUT(PS2_PORT0),
    WRITE_CLK(PS2_PORT0, 0),
    M_DELAY_US(200),
    CONFIGURE_DAT_OUTPUT(PS2_PORT0),
    WRITE_DAT(PS2_PORT0, 0),

    // CLK — вход
    CONFIGURE_CLK_INPUT(PS2_PORT0),

    M_LABEL(LABEL_PORT0_TX_NEXT_BIT),

    // Ждём CLK = 0

    // R3 (он равен 0) временно служит счётчиком ожидания
    I_ADDI(R3, R3, 1),
    I_MOVR(R0, R3),
    M_BGE(LABEL_PORT0_TX_EXIT, CLK_TX_TIMEOUT_VAL), // CLK не ответил — к LABEL_PORT0_TX_EXIT

    // Читаем CLK
    READ_CLK(PS2_PORT0), // R0 = CLK

    // Повторяем, пока CLK = 1
    M_BGE(LABEL_PORT0_TX_NEXT_BIT, 1), // к LABEL_PORT0_TX_NEXT_BIT, если R0 >= 1

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Разряд 10 — подтверждение от устройства: ничего не выставляем
    M_STAGEBGE(LABEL_PORT0_TX_WAIT_CLK_HIGH,
               10), // к LABEL_PORT0_TX_WAIT_CLK_HIGH, если STAGE >= 10

    // CLK = 0: выставляем разряд (младший разряд R0)
    I_ANDI(R0, R2, 1), // R0 = R2 & 1
    WRITE_DAT_R0(PS2_PORT0), // DAT = младший разряд R0

    M_LABEL(LABEL_PORT0_TX_WAIT_CLK_HIGH),

    // Ждём CLK = 1

    // Читаем CLK
    READ_CLK(PS2_PORT0), // R0 = CLK

    // Повторяем, пока CLK = 0
    M_BL(LABEL_PORT0_TX_WAIT_CLK_HIGH, 1), // к LABEL_PORT0_TX_WAIT_CLK_HIGH, если R0 < 1

    // Сдвигаем слово вправо: следующий разряд для передачи
    I_RSHI(R2, R2, 1), // R2 = R2 >> 1

    // Счётчик разрядов + 1
    I_STAGEINCI(1), // STAGE = STAGE + 1

    // Слово кончилось? Если нет — передаём следующий разряд
    M_STAGEBL(LABEL_PORT0_TX_NEXT_BIT, 11), // к LABEL_PORT0_TX_NEXT_BIT, если STAGE < 11

    M_LABEL(LABEL_PORT0_TX_EXIT),

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Снова приём
    CONFIGURE_DAT_INPUT(PS2_PORT0),
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT0_RX_ENABLED), // [RTCMEM_PORT0_RX_ENABLED] = 1

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT0_TX), // [RTCMEM_PORT0_TX] = 0

    // Снова включаем порт 1, если он был включён
    TEMP_PORT1_ENABLE(),

    // Дальше R2 — счётчик повторного пробуждения процессора
    I_MOVI(R2, 0),

    // В цикл команд
    M_BX(LABEL_RX),


    // LABEL_PORT1_TX — передача в порт 1
    // В конце порт 1 сам включается на приём.

    M_LABEL(LABEL_PORT1_TX),

    // Передаём слово из RTCMEM_PORT1_DATAOUT

    // R2 — слово для передачи (10 разрядов: данные, чётность и стоповый разряд)
    I_LD(R2, R3, RTCMEM_PORT1_DATAOUT), // R2 = [RTCMEM_PORT1_DATAOUT]

    // Обнуляем счётчик разрядов (STAGE: 0...7 — данные, 8 — чётность, 9 — стоповый разряд)
    I_STAGERSTI(), // STAGE = 0

    // Временно выключаем порт 0, если он включён
    TEMP_PORT0_DISABLE(),

    // Держим CLK и DAT в нуле 200 мкс
    CONFIGURE_CLK_OUTPUT(PS2_PORT1),
    WRITE_CLK(PS2_PORT1, 0),
    M_DELAY_US(200),
    CONFIGURE_DAT_OUTPUT(PS2_PORT1),
    WRITE_DAT(PS2_PORT1, 0),

    // CLK — вход
    CONFIGURE_CLK_INPUT(PS2_PORT1),

    M_LABEL(LABEL_PORT1_TX_NEXT_BIT),

    // Ждём CLK = 0

    // R3 (он равен 0) временно служит счётчиком ожидания
    I_ADDI(R3, R3, 1),
    I_MOVR(R0, R3),
    M_BGE(LABEL_PORT1_TX_EXIT, CLK_TX_TIMEOUT_VAL), // CLK не ответил — к LABEL_PORT1_TX_EXIT

    // Читаем CLK
    READ_CLK(PS2_PORT1), // R0 = CLK

    // Повторяем, пока CLK = 1
    M_BGE(LABEL_PORT1_TX_NEXT_BIT, 1), // к LABEL_PORT1_TX_NEXT_BIT, если R0 >= 1

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Разряд 10 — подтверждение от устройства: ничего не выставляем
    M_STAGEBGE(LABEL_PORT1_TX_WAIT_CLK_HIGH,
               10), // к LABEL_PORT1_TX_WAIT_CLK_HIGH, если STAGE >= 10

    // CLK = 0: выставляем разряд (младший разряд R0)
    I_ANDI(R0, R2, 1), // R0 = R2 & 1
    WRITE_DAT_R0(PS2_PORT1), // DAT = младший разряд R0

    M_LABEL(LABEL_PORT1_TX_WAIT_CLK_HIGH),

    // Ждём CLK = 1

    // Читаем CLK
    READ_CLK(PS2_PORT1), // R0 = CLK

    // Повторяем, пока CLK = 0
    M_BL(LABEL_PORT1_TX_WAIT_CLK_HIGH, 1), // к LABEL_PORT1_TX_WAIT_CLK_HIGH, если R0 < 1

    // Сдвигаем слово вправо: следующий разряд для передачи
    I_RSHI(R2, R2, 1), // R2 = R2 >> 1

    // Счётчик разрядов + 1
    I_STAGEINCI(1), // STAGE = STAGE + 1

    // Слово кончилось? Если нет — передаём следующий разряд
    M_STAGEBL(LABEL_PORT1_TX_NEXT_BIT, 11), // к LABEL_PORT1_TX_NEXT_BIT, если STAGE < 11

    M_LABEL(LABEL_PORT1_TX_EXIT),

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Снова приём
    CONFIGURE_DAT_INPUT(PS2_PORT1),
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT1_RX_ENABLED), // [RTCMEM_PORT1_RX_ENABLED] = 1

    // Сбрасываем команду
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT1_TX), // [RTCMEM_PORT1_TX] = 0

    // Снова включаем порт 0, если он был включён
    TEMP_PORT0_ENABLE(),

    // Дальше R2 — счётчик повторного пробуждения процессора
    I_MOVI(R2, 0),

    // В цикл команд
    M_BX(LABEL_RX),


    // LABEL_PORT0_RX_CLK_TIMEOUT

    M_LABEL(LABEL_PORT0_RX_CLK_TIMEOUT),

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Выключаем порт 0
    PERM_PORT0_DISABLE(),

    // Снова включаем порт 1, если он был включён
    TEMP_PORT1_ENABLE(),

    // Флаг: CLK порта 0 не ответил
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT0_RX_CLK_TIMEOUT),

    I_WAKE(),

    // Дальше R2 — счётчик повторного пробуждения процессора
    I_MOVI(R2, 0),

    M_BX(LABEL_RX),


    // LABEL_PORT1_RX_CLK_TIMEOUT

    M_LABEL(LABEL_PORT1_RX_CLK_TIMEOUT),

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Выключаем порт 1
    PERM_PORT1_DISABLE(),

    // Снова включаем порт 0, если он был включён
    TEMP_PORT0_ENABLE(),

    // Флаг: CLK порта 1 не ответил
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT1_RX_CLK_TIMEOUT),

    I_WAKE(),

    // Дальше R2 — счётчик повторного пробуждения процессора.
    I_MOVI(R2, 0),

    // Дальше сразу LABEL_RX.


    // LABEL_RX — проверка новых данных с порта 0
    // Во время приёма порт 1 выключен, в конце он включается снова.
    // После приёма порт 0 остаётся выключенным, пока процессор не заберёт слово.

    M_LABEL(LABEL_RX),

    // Порт 0 принимает?
    I_LD(R0, R3, RTCMEM_PORT0_RX_ENABLED),
    M_BGE(LABEL_PORT0_RX_CHECK_CLK, 1), // да — к LABEL_PORT0_RX_CHECK_CLK

    // Нет: порт 0 выключен — видимо, слово ждёт процессора; считаем проходы в R2
    I_ADDI(R2, R2, 1), // R2 = R2 + 1
    I_MOVR(R0, R2),
    M_BL(LABEL_RX_NEXT, WAKE_THRESHOLD), // Меньше WAKE_THRESHOLD — проверяем другой порт
    I_WAKE(), // Снова будим процессор
    I_MOVI(R2, 0), // Обнуляем счётчик
    M_BX(LABEL_RX_NEXT), // Проверяем другой порт

    M_LABEL(LABEL_PORT0_RX_CHECK_CLK),

    // Читаем CLK
    READ_CLK(PS2_PORT0), // R0 = CLK

    // CLK = 0?
    M_BGE(LABEL_RX_NEXT, 1), // нет — к LABEL_RX_NEXT

    // Да: принимаем слово с порта 0

    // Сбрасываем состояние
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT0_DATAIN), // [RTCMEM_PORT0_DATAIN] = 0
    I_STAGERSTI(), // STAGE = 0 (STAGE — счётчик разрядов)
    I_MOVI(R2, 0), // R2 = 0 (R2 — последнее прочитанное значение CLK)

    // Временно выключаем порт 1, если он включён
    TEMP_PORT1_DISABLE(),

    // CLK уже 0: сразу читаем DAT
    M_BX(LABEL_PORT0_RX_CLK_IS_LOW),

    M_LABEL(LABEL_PORT0_RX_WAIT_LOOP),

    // R3 (он равен 0) временно служит счётчиком ожидания
    I_ADDI(R3, R3, 1),
    I_MOVR(R0, R3),
    M_BGE(LABEL_PORT0_RX_CLK_TIMEOUT,
          CLK_RX_TIMEOUT_VAL), // CLK не ответил — к LABEL_PORT0_RX_CLK_TIMEOUT

    // Читаем CLK
    READ_CLK(PS2_PORT0), // R0 = CLK

    // CLK изменился?
    I_SUBR(R1, R2, R0), // R1 = R2 - R0
    M_BXZ(LABEL_PORT0_RX_WAIT_LOOP), // нет — снова к LABEL_PORT0_RX_WAIT_LOOP

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Запоминаем новое значение CLK
    I_MOVR(R2, R0), // R2 = R0

    // CLK = 1?
    M_BGE(LABEL_PORT0_RX_CLK_IS_HIGH, 1),

    M_LABEL(LABEL_PORT0_RX_CLK_IS_LOW),

    // CLK = 0: читаем DAT
    READ_DAT(PS2_PORT0), // R0 = DAT

    // Добавляем разряд к слову и сдвигаем слово вправо
    I_LSHI(R0, R0, 11), // R0 = R0 << 11
    I_LD(R1, R3, RTCMEM_PORT0_DATAIN), // R1 = [RTCMEM_PORT0_DATAIN]
    I_ORR(R1, R1, R0), // R1 = R1 | R0
    I_RSHI(R1, R1, 1), // R1 = R1 >> 1
    I_ST(R1, R3, RTCMEM_PORT0_DATAIN), // [RTCMEM_PORT0_DATAIN] = R1

    // Повторяем
    M_BX(LABEL_PORT0_RX_WAIT_LOOP),

    M_LABEL(LABEL_PORT0_RX_CLK_IS_HIGH),

    // Счётчик разрядов + 1
    I_STAGEINCI(1), // STAGE = STAGE + 1

    // Слово кончилось? Если нет — принимаем следующий разряд
    M_STAGEBL(LABEL_PORT0_RX_WAIT_LOOP, 11), // к LABEL_PORT0_RX_WAIT_LOOP, если STAGE < 11

    // Слово принято: выключаем порт 0
    PERM_PORT0_DISABLE(),

    // Флаг: слово принято
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT0_RX), // [RTCMEM_PORT0_RX] = 1

    // Снова включаем порт 1, если он был включён
    TEMP_PORT1_ENABLE(),

    // RTC_CNTL_RDY_FOR_WAKEUP_S в RTC_CNTL_LOW_POWER_ST_REG не проверяем: он не
    // устанавливается, потому что ULP не выполняет HALT, а процессор всегда работает.
    I_WAKE(),

    // Дальше R2 — счётчик повторного пробуждения процессора
    I_MOVI(R2, 0),


    // LABEL_RX_NEXT — проверка новых данных с порта 1
    // Во время приёма порт 0 выключен, в конце он включается снова.
    // После приёма порт 1 остаётся выключенным, пока процессор не заберёт слово.

    M_LABEL(LABEL_RX_NEXT),

    // Порт 1 принимает?
    I_LD(R0, R3, RTCMEM_PORT1_RX_ENABLED),
    M_BGE(LABEL_PORT1_RX_CHECK_CLK, 1), // да — к LABEL_PORT1_RX_CHECK_CLK

    // Нет: порт 1 выключен — видимо, слово ждёт процессора; считаем проходы в R2
    I_ADDI(R2, R2, 1), // R2 = R2 + 1
    I_MOVR(R0, R2),
    M_LONG_BL(LABEL_WAIT_COMMAND,
              WAKE_THRESHOLD), // Меньше WAKE_THRESHOLD — в главный цикл
    I_WAKE(), // Снова будим процессор
    I_MOVI(R2, 0), // Обнуляем счётчик
    M_BX(LABEL_WAIT_COMMAND), // В главный цикл

    M_LABEL(LABEL_PORT1_RX_CHECK_CLK),

    // Читаем CLK
    READ_CLK(PS2_PORT1), // R0 = CLK

    // CLK = 0?
    M_LONG_BGE(LABEL_WAIT_COMMAND, 1), // нет — к LABEL_WAIT_COMMAND

    // Да: принимаем слово с порта 1

    // Сбрасываем состояние
    I_MOVI(R0, 0),
    I_ST(R0, R3, RTCMEM_PORT1_DATAIN), // [RTCMEM_PORT1_DATAIN] = 0
    I_STAGERSTI(), // STAGE = 0 (STAGE — счётчик разрядов)
    I_MOVI(R2, 0), // R2 = 0 (R2 — последнее прочитанное значение CLK)

    // Временно выключаем порт 0, если он включён
    TEMP_PORT0_DISABLE(),

    // CLK уже 0: сразу читаем DAT
    M_BX(LABEL_PORT1_RX_CLK_IS_LOW),

    M_LABEL(LABEL_PORT1_RX_WAIT_LOOP),

    // R3 (он равен 0) временно служит счётчиком ожидания
    I_ADDI(R3, R3, 1),
    I_MOVR(R0, R3),
    M_BGE(LABEL_PORT1_RX_CLK_TIMEOUT,
          CLK_RX_TIMEOUT_VAL), // CLK не ответил — к LABEL_PORT1_RX_CLK_TIMEOUT

    // Читаем CLK
    READ_CLK(PS2_PORT1), // R0 = CLK

    // CLK изменился?
    I_SUBR(R1, R2, R0), // R1 = R2 - R0
    M_BXZ(LABEL_PORT1_RX_WAIT_LOOP), // нет — снова к LABEL_PORT1_RX_WAIT_LOOP

    // R3 снова база переменных, а не счётчик
    I_MOVI(R3, 0),

    // Запоминаем новое значение CLK
    I_MOVR(R2, R0), // R2 = R0

    // CLK = 1?
    M_BGE(LABEL_PORT1_RX_CLK_IS_HIGH, 1),

    M_LABEL(LABEL_PORT1_RX_CLK_IS_LOW),

    // CLK = 0: читаем DAT
    READ_DAT(PS2_PORT1), // R0 = DAT

    // Добавляем разряд к слову и сдвигаем слово вправо
    I_LSHI(R0, R0, 11), // R0 = R0 << 11
    I_LD(R1, R3, RTCMEM_PORT1_DATAIN), // R1 = [RTCMEM_PORT1_DATAIN]
    I_ORR(R1, R1, R0), // R1 = R1 | R0
    I_RSHI(R1, R1, 1), // R1 = R1 >> 1
    I_ST(R1, R3, RTCMEM_PORT1_DATAIN), // [RTCMEM_PORT1_DATAIN] = R1

    // Повторяем
    M_BX(LABEL_PORT1_RX_WAIT_LOOP),

    M_LABEL(LABEL_PORT1_RX_CLK_IS_HIGH),

    // Счётчик разрядов + 1
    I_STAGEINCI(1), // STAGE = STAGE + 1

    // Слово кончилось? Если нет — принимаем следующий разряд
    M_STAGEBL(LABEL_PORT1_RX_WAIT_LOOP, 11), // к LABEL_PORT1_RX_WAIT_LOOP, если STAGE < 11

    // Слово принято: выключаем порт 1
    PERM_PORT1_DISABLE(),

    // Флаг: слово принято
    I_MOVI(R0, 1),
    I_ST(R0, R3, RTCMEM_PORT1_RX), // [RTCMEM_PORT1_RX] = 1

    // Снова включаем порт 0, если он был включён
    TEMP_PORT0_ENABLE(),

    // RTC_CNTL_RDY_FOR_WAKEUP_S в RTC_CNTL_LOW_POWER_ST_REG не проверяем: он не
    // устанавливается, потому что ULP не выполняет HALT, а процессор всегда работает.
    I_WAKE(),

    // Дальше R2 — счётчик повторного пробуждения процессора
    I_MOVI(R2, 0),

    // В главный цикл
    M_BX(LABEL_WAIT_COMMAND),
};

void replace_placeholders(std::uint32_t prg_start, int size, bool port_0_enabled,
                          gpio_num_t port_0_clk_gpio, gpio_num_t port_0_dat_gpio,
                          bool port_1_enabled, gpio_num_t port_1_clk_gpio,
                          gpio_num_t port_1_dat_gpio);

}


/**
 * @class esp::ps2::ps2_controller
 * @brief Контроллер портов PS/2 на сопроцессоре ULP.
 *
 * Сопроцессор ULP непрерывно следит за линиями CLK и DATA двух портов
 * PS/2, принимает слова устройств и передаёт им команды; принятое слово
 * он оставляет в медленной памяти RTC и будит процессор прерыванием.
 * Слово порта — 11 разрядов: стартовый, 8 разрядов данных, чётность и
 * стоповый.
 *
 * Все методы контроллера статические, объект контроллера создать нельзя.
 * begin() по набору устройств заодно создаёт клавиатуру и мышь:
 * @code
 * esp::ps2::ps2_controller::begin(esp::ps2::ps2_preset::keyboard_port_0_mouse_port_1);
 * esp::ps2::keyboard *keyboard = esp::ps2::ps2_controller::keyboard();
 * @endcode
 */


esp::ps2::keyboard *esp::ps2::ps2_controller::_keyboard = nullptr;
esp::ps2::mouse *esp::ps2::ps2_controller::_mouse = nullptr;
std::unique_ptr<esp::ps2::keyboard> esp::ps2::ps2_controller::_own_keyboard;
std::unique_ptr<esp::ps2::mouse> esp::ps2::ps2_controller::_own_mouse;
bool esp::ps2::ps2_controller::_port_enabled[2];
intr_handle_t esp::ps2::ps2_controller::_ulp_wake_isr_handle;
bool esp::ps2::ps2_controller::_parity_error[2];
bool esp::ps2::ps2_controller::_sync_error[2];
bool esp::ps2::ps2_controller::_clk_time_out_error[2];
QueueHandle_t esp::ps2::ps2_controller::_data_in[2];
SemaphoreHandle_t esp::ps2::ps2_controller::_port_lock[2];
bool esp::ps2::ps2_controller::_init_done = false;


// Открытые

/**
 * @brief Запускает контроллер на заданных выводах.
 *
 * Загружает и запускает программу ULP и ставит обработчик её прерывания
 * на ядро 1. Сопроцессор работает и после end(); повторный вызов только
 * сбрасывает состояние портов. Годятся только выводы, доступные
 * сопроцессору ULP: GPIO2, 4, 12 (с ограничениями), 13, 14, 15, 25, 26,
 * 27, 32 и 33.
 *
 * @param port_0_clk_gpio Вывод CLK порта 0.
 * @param port_0_dat_gpio Вывод DATA порта 0.
 * @param port_1_clk_gpio Вывод CLK порта 1; @c GPIO_UNUSED — порта 1 нет.
 * @param port_1_dat_gpio Вывод DATA порта 1; @c GPIO_UNUSED — порта 1 нет.
 */
void esp::ps2::ps2_controller::begin(gpio_num_t port_0_clk_gpio, gpio_num_t port_0_dat_gpio,
                                     gpio_num_t port_1_clk_gpio, gpio_num_t port_1_dat_gpio)
{
    // Сопроцессор ULP запускается один раз и работает и после end().
    if (!_init_done)
    {
        _port_enabled[0] = (port_0_clk_gpio != GPIO_UNUSED && port_0_dat_gpio != GPIO_UNUSED);
        _port_enabled[1] = (port_1_clk_gpio != GPIO_UNUSED && port_1_dat_gpio != GPIO_UNUSED);

        if (_port_enabled[0])
        {
            if (!rtc_gpio_is_valid_gpio(port_0_clk_gpio)
                || !rtc_gpio_is_valid_gpio(port_0_dat_gpio))
            {
                ESP_LOGE(esp::ps2::p::TAG, "Invalid PS/2 port 0 pins.");
                _port_enabled[0] = false;
            }
            else
            {
                rtc_gpio_init(port_0_clk_gpio);
                rtc_gpio_init(port_0_dat_gpio);
            }
        }

        if (_port_enabled[1])
        {
            if (!rtc_gpio_is_valid_gpio(port_1_clk_gpio)
                || !rtc_gpio_is_valid_gpio(port_1_dat_gpio))
            {
                ESP_LOGE(esp::ps2::p::TAG, "Invalid PS/2 port 1 pins.");
                _port_enabled[1] = false;
            }
            else
            {
                rtc_gpio_init(port_1_clk_gpio);
                rtc_gpio_init(port_1_dat_gpio);
            }
        }

        // Очищаем память ULP: без этого после программного сброса ULP может
        // не запуститься.
        for (int i = RTCMEM_PROG_START; i < RTCMEM_LASTVAR; ++i)
            RTC_SLOW_MEM[i] = 0x0000;

        // Собираем программу ULP в медленной памяти RTC и подставляем выводы
        // вместо заглушек.
        std::size_t size = sizeof(esp::ps2::p::ULP_CODE) / sizeof(ulp_insn_t);
        ulp_process_macros_and_load_ex(RTCMEM_PROG_START, esp::ps2::p::ULP_CODE, &size);
        esp::ps2::p::replace_placeholders(RTCMEM_PROG_START, size, _port_enabled[0],
                                          port_0_clk_gpio, port_0_dat_gpio, _port_enabled[1],
                                          port_1_clk_gpio, port_1_dat_gpio);
        assert(size < RTCMEM_VARS_START && "ULP Program too long, increase RTCMEM_VARS_START");

        // Точка входа; ULP запускается по команде процессора (FORCE START).
        REG_SET_FIELD(SENS_SAR_START_FORCE_REG, SENS_PC_INIT, RTCMEM_PROG_START);
        SET_PERI_REG_MASK(SENS_SAR_START_FORCE_REG, SENS_ULP_CP_FORCE_START_TOP);

        for (int p = 0; p < 2; ++p)
        {
            RTC_SLOW_MEM[RTCMEM_PORT0_TX + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLE + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_DISABLE + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLED + p] = 0;
            _parity_error[p] = _sync_error[p] = false;
            _data_in[p] = (_port_enabled[p] ? xQueueCreate(1, sizeof(std::uint16_t)) : nullptr);
            _port_lock[p] = (_port_enabled[p] ? xSemaphoreCreateRecursiveMutex() : nullptr);
            enable_rx(p);
        }

        // Запускаем ULP.
        SET_PERI_REG_MASK(SENS_SAR_START_FORCE_REG, SENS_ULP_CP_START_TOP);

        // Обработчик прерывания RTC — его вызывает команда WAKE сопроцессора.
        // Уровень 2 нужен, чтобы надёжно работать рядом с частыми
        // прерываниями VGA на том же ядре. На некоторых платах читать
        // медленную память RTC и получать прерывания может только ядро 1,
        // поэтому обработчик ставится на него.
        esp::esp_intr_alloc_pinned_to_core(ETS_RTC_CORE_INTR_SOURCE, ESP_INTR_FLAG_LEVEL2,
                                           ulp_wake_isr, nullptr, &_ulp_wake_isr_handle, 1);
        SET_PERI_REG_MASK(RTC_CNTL_INT_ENA_REG, RTC_CNTL_ULP_CP_INT_ENA);

        _init_done = true;
    }
    else
    {
        // ULP уже работает: сбрасываем состояние портов.
        for (int p = 0; p < 2; ++p)
        {
            RTC_SLOW_MEM[RTCMEM_PORT0_TX + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLE + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_DISABLE + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX + p] = 0;
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLED + p] = 0;
            _parity_error[p] = _sync_error[p] = false;
            if (_port_enabled[p])
            {
                xQueueReset(_data_in[p]);
                xSemaphoreGiveRecursive(_port_lock[p]);
                enable_rx(p);
            }
        }
    }
}

/**
 * @brief Запускает контроллер на выводах платы и создаёт клавиатуру и мышь.
 *
 * Порт 0 — GPIO33 (CLK) и GPIO32 (DATA), порт 1 — GPIO26 (CLK) и GPIO27
 * (DATA), как на TTGO VGA32. Сначала вызывается end().
 *
 * @param preset Что подключено к портам.
 * @param keyboard_mode Получать ли виртуальные клавиши и ставить ли их в очередь.
 */
void esp::ps2::ps2_controller::begin(ps2_preset preset, kbd_mode keyboard_mode)
{
    end();

    const bool generate_virtual_keys = (keyboard_mode == kbd_mode::generate_virtual_keys
                                        || keyboard_mode == kbd_mode::create_virtual_keys_queue);
    const bool create_vk_queue = (keyboard_mode == kbd_mode::create_virtual_keys_queue);
    switch (preset)
    {
        case ps2_preset::keyboard_port_0_mouse_port_1:
            begin(GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_26, GPIO_NUM_27);
            _own_keyboard = std::make_unique<esp::ps2::keyboard>();
            set_keyboard(_own_keyboard.get());
            _own_keyboard->begin(generate_virtual_keys, create_vk_queue, 0);
            _own_mouse = std::make_unique<esp::ps2::mouse>();
            set_mouse(_own_mouse.get());
            _own_mouse->begin(1);
            break;
        case ps2_preset::keyboard_port_1_mouse_port_0:
            begin(GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_26, GPIO_NUM_27);
            _own_mouse = std::make_unique<esp::ps2::mouse>();
            set_mouse(_own_mouse.get());
            _own_mouse->begin(0);
            _own_keyboard = std::make_unique<esp::ps2::keyboard>();
            set_keyboard(_own_keyboard.get());
            _own_keyboard->begin(generate_virtual_keys, create_vk_queue, 1);
            break;
        case ps2_preset::keyboard_port_0:
            // keyboard::begin() сам запустит контроллер на одном порту и
            // вызовет set_keyboard(); так же и в остальных случаях.
            _own_keyboard = std::make_unique<esp::ps2::keyboard>();
            _own_keyboard->begin(GPIO_NUM_33, GPIO_NUM_32, generate_virtual_keys, create_vk_queue);
            break;
        case ps2_preset::keyboard_port_1:
            _own_keyboard = std::make_unique<esp::ps2::keyboard>();
            _own_keyboard->begin(GPIO_NUM_26, GPIO_NUM_27, generate_virtual_keys, create_vk_queue);
            break;
        case ps2_preset::mouse_port_0:
            _own_mouse = std::make_unique<esp::ps2::mouse>();
            _own_mouse->begin(GPIO_NUM_33, GPIO_NUM_32);
            break;
        case ps2_preset::mouse_port_1:
            _own_mouse = std::make_unique<esp::ps2::mouse>();
            _own_mouse->begin(GPIO_NUM_26, GPIO_NUM_27);
            break;
    }
}

/**
 * @brief Удаляет созданные контроллером клавиатуру и мышь и выключает приём портов.
 *
 * Клавиатура и мышь, заданные set_keyboard() и set_mouse(), не удаляются,
 * только забываются. Сопроцессор ULP продолжает работать.
 */
void esp::ps2::ps2_controller::end()
{
    if (_init_done)
    {
        _own_keyboard.reset();
        _keyboard = nullptr;

        _own_mouse.reset();
        _mouse = nullptr;

        for (int p = 0; p < 2; ++p)
            disable_rx(p);
    }
}

/**
 * @brief Сообщает, есть ли непрочитанное слово порта.
 * @param ps2_port Порт: 0 или 1.
 * @return @c true, если слово есть.
 */
bool esp::ps2::ps2_controller::data_available(int ps2_port)
{
    return uxQueueMessagesWaiting(_data_in[ps2_port]);
}

/**
 * @brief Получает байт с порта.
 *
 * Проверяет стартовый и стоповый разряды и чётность и запоминает ошибки
 * (parity_error(), sync_error(), clk_time_out_error()). Приняв слово,
 * сопроцессор выключает приём порта; здесь приём включается снова.
 *
 * @param ps2_port Порт: 0 или 1.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return Байт или −1, если время вышло или слово пришло с ошибкой.
 */
int esp::ps2::ps2_controller::get_data(int ps2_port, int time_out_ms)
{
    int r = -1;

    std::uint16_t w;
    if (xQueueReceive(_data_in[ps2_port], &w, esp::ms_to_ticks(time_out_ms)))
    {
        // Разряд 0 — стартовый, 1–8 — данные, 9 — чётность, 10 — стоповый;
        // 0xFFFF — CLK не ответил.
        _clk_time_out_error[ps2_port] = (w == 0xffff);
        if (!_clk_time_out_error[ps2_port])
        {
            const std::uint8_t start_bit = w & 1;
            const std::uint8_t stop_bit = (w >> 10) & 1;
            const std::uint8_t parity = (w >> 9) & 1;
            r = (w >> 1) & 0xff;
            _parity_error[ps2_port] = (parity != !calc_parity(r));
            _sync_error[ps2_port] = (start_bit != 0 || stop_bit != 1);
            if (_parity_error[ps2_port] || _sync_error[ps2_port])
                r = -1;
        }

        // Приняв слово или не дождавшись CLK, ULP выключает приём: включаем
        // его снова.
        RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLE + ps2_port] = 1;
    }

    return r;
}

/**
 * @brief Передаёт байт в порт.
 *
 * Добавляет к байту разряд чётности и стоповый разряд и поручает передачу
 * сопроцессору; ответа не ждёт. В ненастроенный порт ничего не передаётся.
 *
 * @param data Байт.
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::ps2_controller::send_data(std::uint8_t data, int ps2_port)
{
    if (_port_enabled[ps2_port])
    {
        // 0x200 — стоповый разряд; стартовый ULP выставляет сам.
        RTC_SLOW_MEM[RTCMEM_PORT0_DATAOUT + ps2_port] =
            0x200 | ((!calc_parity(data) & 1) << 8) | data;
        RTC_SLOW_MEM[RTCMEM_PORT0_TX + ps2_port] = 1;
    }
}

/**
 * @brief Выключает приём порта: сопроцессор прижимает CLK к нулю.
 *
 * Пока CLK в нуле, устройство ничего не передаёт. Включает приём enable_rx().
 *
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::ps2_controller::disable_rx(int ps2_port)
{
    if (_port_enabled[ps2_port])
        RTC_SLOW_MEM[RTCMEM_PORT0_RX_DISABLE + ps2_port] = 1;
}

/**
 * @brief Включает приём порта, отпуская CLK.
 *
 * Если принятое слово ещё не забрано, приём не включается: это сделает
 * get_data().
 *
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::ps2_controller::enable_rx(int ps2_port)
{
    if (_port_enabled[ps2_port])
    {
        // Пока принятое слово не забрано, приём не включаем.
        if (!data_available(ps2_port))
            RTC_SLOW_MEM[RTCMEM_PORT0_RX_ENABLE + ps2_port] = 1;
    }
}

/**
 * @brief Захватывает порт для монопольного доступа.
 *
 * Захват рекурсивный: задача, уже владеющая портом, может захватить его
 * снова и должна столько же раз вызвать unlock().
 *
 * @param ps2_port Порт: 0 или 1.
 * @param time_out_ms Сколько ждать, мс; −1 — сколько потребуется.
 * @return @c true, если порт захвачен; ненастроенный порт «захватывается» всегда.
 */
bool esp::ps2::ps2_controller::lock(int ps2_port, int time_out_ms)
{
    return _port_enabled[ps2_port]
        ? xSemaphoreTakeRecursive(_port_lock[ps2_port], esp::ms_to_ticks(time_out_ms))
        : true;
}

/**
 * @brief Освобождает порт, захваченный lock().
 * @param ps2_port Порт: 0 или 1.
 */
void esp::ps2::ps2_controller::unlock(int ps2_port)
{
    if (_port_enabled[ps2_port])
        xSemaphoreGiveRecursive(_port_lock[ps2_port]);
}

// Закрытые

/**
 * @brief Прерывание сопроцессора ULP: переносит принятые слова в очереди портов.
 *
 * Тайм-аут CLK передаётся словом 0xFFFF.
 *
 * @param arg Не используется.
 */
void IRAM_ATTR esp::ps2::ps2_controller::ulp_wake_isr(void *arg)
{
    const std::uint32_t rtc_intr = READ_PERI_REG(RTC_CNTL_INT_ST_REG);

    if (rtc_intr & RTC_CNTL_SAR_INT_ST)
    {
        for (int p = 0; p < 2; ++p)
        {
            if (RTC_SLOW_MEM[RTCMEM_PORT0_RX + p] & 0xffff)
            {
                // Принято слово.
                const std::uint16_t d = RTC_SLOW_MEM[RTCMEM_PORT0_DATAIN + p] & 0xffff;
                xQueueOverwriteFromISR(_data_in[p], &d, nullptr);
                RTC_SLOW_MEM[RTCMEM_PORT0_RX + p] = 0;
            }
            else if (RTC_SLOW_MEM[RTCMEM_PORT0_RX_CLK_TIMEOUT + p] & 0xffff)
            {
                // CLK не ответил.
                const std::uint16_t d = 0xffff;
                xQueueOverwriteFromISR(_data_in[p], &d, nullptr);
                RTC_SLOW_MEM[RTCMEM_PORT0_RX_CLK_TIMEOUT + p] = 0;
            }
        }
    }

    WRITE_PERI_REG(RTC_CNTL_INT_CLR_REG, rtc_intr);
}

/**
 * @brief Заменяет заглушки в загруженной программе ULP настоящими командами.
 *
 * Заглушки (код операции OPCODE_PLACEHOLDER) обращаются к выводам портов;
 * номера выводов известны только в esp::ps2::ps2_controller::begin().
 * Заглушки ненастроенного порта остаются как есть.
 *
 * @param prg_start Начало программы в медленной памяти RTC, в
 *        32-разрядных словах.
 * @param size Размер программы в 32-разрядных словах.
 * @param port_0_enabled Настроен ли порт 0.
 * @param port_0_clk_gpio Вывод CLK порта 0.
 * @param port_0_dat_gpio Вывод DATA порта 0.
 * @param port_1_enabled Настроен ли порт 1.
 * @param port_1_clk_gpio Вывод CLK порта 1.
 * @param port_1_dat_gpio Вывод DATA порта 1.
 */
void esp::ps2::p::replace_placeholders(std::uint32_t prg_start, int size, bool port_0_enabled,
                                       gpio_num_t port_0_clk_gpio, gpio_num_t port_0_dat_gpio,
                                       bool port_1_enabled, gpio_num_t port_1_clk_gpio,
                                       gpio_num_t port_1_dat_gpio)
{
    std::uint32_t clk_rtc_gpio_num[2], clk_rtc_gpio_reg[2], clk_rtc_gpio_ie_s[2];
    std::uint32_t dat_rtc_gpio_num[2], dat_rtc_gpio_reg[2], dat_rtc_gpio_ie_s[2];

    if (port_0_enabled)
    {
        const int port_0_clk_io = rtc_io_number_get(port_0_clk_gpio);
        clk_rtc_gpio_num[0] = port_0_clk_io;
        clk_rtc_gpio_reg[0] = rtc_io_desc[port_0_clk_io].reg;
        clk_rtc_gpio_ie_s[0] = static_cast<std::uint32_t>(ffs(rtc_io_desc[port_0_clk_io].ie)) - 1;
        const int port_0_dat_io = rtc_io_number_get(port_0_dat_gpio);
        dat_rtc_gpio_num[0] = port_0_dat_io;
        dat_rtc_gpio_reg[0] = rtc_io_desc[port_0_dat_io].reg;
        dat_rtc_gpio_ie_s[0] = static_cast<std::uint32_t>(ffs(rtc_io_desc[port_0_dat_io].ie)) - 1;
    }

    if (port_1_enabled)
    {
        const int port_1_clk_io = rtc_io_number_get(port_1_clk_gpio);
        clk_rtc_gpio_num[1] = port_1_clk_io;
        clk_rtc_gpio_reg[1] = rtc_io_desc[port_1_clk_io].reg;
        clk_rtc_gpio_ie_s[1] = static_cast<std::uint32_t>(ffs(rtc_io_desc[port_1_clk_io].ie)) - 1;
        const int port_1_dat_io = rtc_io_number_get(port_1_dat_gpio);
        dat_rtc_gpio_num[1] = port_1_dat_io;
        dat_rtc_gpio_reg[1] = rtc_io_desc[port_1_dat_io].reg;
        dat_rtc_gpio_ie_s[1] = static_cast<std::uint32_t>(ffs(rtc_io_desc[port_1_dat_io].ie)) - 1;
    }

    for (int i = 0; i < size; ++i)
    {
        ulp_insn_t *const ins = reinterpret_cast<ulp_insn_t *>(RTC_SLOW_MEM) + prg_start + i;
        if (ins->macro.opcode == OPCODE_PLACEHOLDER)
        {
            // Номер порта заглушки хранится в неиспользуемом поле команды.
            const int ps2port = ins->macro.unused;
            if ((port_0_enabled && ps2port == 0) || (port_1_enabled && ps2port == 1))
            {
                ins->macro.unused = 0;
                // Макрос команды ULP даёт список инициализации; тип перед ним
                // превращает список в значение.
                switch (ins->macro.sub_opcode)
                {
                    case SUB_OPCODE_DAT_ENABLE_OUTPUT:
                        if (ins->macro.label)
                            *ins = ulp_insn_t I_WR_REG_BIT(
                                RTC_GPIO_ENABLE_W1TS_REG,
                                dat_rtc_gpio_num[ps2port] + RTC_GPIO_ENABLE_W1TS_S, 1);
                        else
                            *ins = ulp_insn_t I_WR_REG_BIT(
                                RTC_GPIO_ENABLE_W1TC_REG,
                                dat_rtc_gpio_num[ps2port] + RTC_GPIO_ENABLE_W1TC_S, 1);
                        break;
                    case SUB_OPCODE_DAT_ENABLE_INPUT:
                        *ins =
                            ulp_insn_t I_WR_REG_BIT(dat_rtc_gpio_reg[ps2port],
                                                    dat_rtc_gpio_ie_s[ps2port], ins->macro.label);
                        break;
                    case SUB_OPCODE_CLK_ENABLE_OUTPUT:
                        if (ins->macro.label)
                            *ins = ulp_insn_t I_WR_REG_BIT(
                                RTC_GPIO_ENABLE_W1TS_REG,
                                clk_rtc_gpio_num[ps2port] + RTC_GPIO_ENABLE_W1TS_S, 1);
                        else
                            *ins = ulp_insn_t I_WR_REG_BIT(
                                RTC_GPIO_ENABLE_W1TC_REG,
                                clk_rtc_gpio_num[ps2port] + RTC_GPIO_ENABLE_W1TC_S, 1);
                        break;
                    case SUB_OPCODE_CLK_ENABLE_INPUT:
                        *ins =
                            ulp_insn_t I_WR_REG_BIT(clk_rtc_gpio_reg[ps2port],
                                                    clk_rtc_gpio_ie_s[ps2port], ins->macro.label);
                        break;
                    case SUB_OPCODE_READ_CLK:
                        *ins = ulp_insn_t I_RD_REG(RTC_GPIO_IN_REG,
                                                   clk_rtc_gpio_num[ps2port] + RTC_GPIO_IN_NEXT_S,
                                                   clk_rtc_gpio_num[ps2port] + RTC_GPIO_IN_NEXT_S);
                        break;
                    case SUB_OPCODE_READ_DAT:
                        *ins = ulp_insn_t I_RD_REG(RTC_GPIO_IN_REG,
                                                   dat_rtc_gpio_num[ps2port] + RTC_GPIO_IN_NEXT_S,
                                                   dat_rtc_gpio_num[ps2port] + RTC_GPIO_IN_NEXT_S);
                        break;
                    case SUB_OPCODE_WRITE_CLK:
                        *ins = ulp_insn_t I_WR_REG_BIT(
                            RTC_GPIO_OUT_REG, clk_rtc_gpio_num[ps2port] + RTC_GPIO_OUT_DATA_S,
                            ins->macro.label);
                        break;
                    case SUB_OPCODE_WRITE_DAT:
                        *ins = ulp_insn_t I_WR_REG_BIT(
                            RTC_GPIO_OUT_REG, dat_rtc_gpio_num[ps2port] + RTC_GPIO_OUT_DATA_S,
                            ins->macro.label);
                        break;
                }
            }
        }
    }
}
