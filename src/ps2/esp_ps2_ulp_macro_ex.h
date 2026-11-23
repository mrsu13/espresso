// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include <cstddef>
#include <cstdint>

#include <esp_attr.h>
#include <esp_err.h>
#include <esp_log.h>
#if !__has_include(<esp32/ulp.h>)
#   error \
       "The PS/2 driver needs the ULP FSM coprocessor: set CONFIG_ULP_COPROC_ENABLED=y, CONFIG_ULP_COPROC_TYPE_FSM=y and CONFIG_ULP_COPROC_RESERVE_MEM=4096 in sdkconfig"
#endif
#include <esp32/ulp.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/sens_reg.h>
#include <soc/soc.h>


// Команды ULP для регистра STAGE, которых нет в esp32/ulp.h.

/** @brief Операция АЛУ над STAGE: увеличить. */
#define ALU_SEL_STAGE_INC 0
/** @brief Операция АЛУ над STAGE: уменьшить. */
#define ALU_SEL_STAGE_DEC 1
/** @brief Операция АЛУ над STAGE: обнулить. */
#define ALU_SEL_STAGE_RST 2

/** @brief Подкод перехода по регистру STAGE. */
#define SUB_OPCODE_STAGEB 2

/** @brief STAGE = STAGE + imm_ (imm_ — 8 разрядов). */
#define I_STAGEINCI(imm_) \
    { \
        .alu_imm = {.dreg = 0, \
                    .sreg = 0, \
                    .imm = imm_, \
                    .unused = 0, \
                    .sel = ALU_SEL_STAGE_INC, \
                    .sub_opcode = SUB_OPCODE_ALU_CNT, \
                    .opcode = OPCODE_ALU} \
    }

/** @brief STAGE = 0. */
#define I_STAGERSTI() \
    { \
        .alu_imm = {.dreg = 0, \
                    .sreg = 0, \
                    .imm = 0, \
                    .unused = 0, \
                    .sel = ALU_SEL_STAGE_RST, \
                    .sub_opcode = SUB_OPCODE_ALU_CNT, \
                    .opcode = OPCODE_ALU} \
    }

/** @brief Относительный переход на pc_offset команд, если STAGE < imm_value (8 разрядов). */
#define I_STAGEBL(pc_offset, imm_value) \
    { \
        .b = {.imm = imm_value, \
              .cmp = 0, \
              .offset = abs(pc_offset), \
              .sign = (pc_offset >= 0) ? 0 : 1, \
              .sub_opcode = SUB_OPCODE_STAGEB, \
              .opcode = OPCODE_BRANCH} \
    }

/** @brief Относительный переход на pc_offset команд, если STAGE <= imm_value (8 разрядов). */
#define I_STAGEBLE(pc_offset, imm_value) \
    { \
        .b = {.imm = imm_value, \
              .cmp = 1, \
              .offset = abs(pc_offset), \
              .sign = (pc_offset >= 0) ? 0 : 1, \
              .sub_opcode = SUB_OPCODE_STAGEB, \
              .opcode = OPCODE_BRANCH} \
    }

/** @brief Относительный переход на pc_offset команд, если STAGE >= imm_value (8 разрядов). */
#define I_STAGEBGE(pc_offset, imm_value) \
    { \
        .b = {.imm = 0x8000 | imm_value, \
              .cmp = 0, \
              .offset = abs(pc_offset), \
              .sign = (pc_offset >= 0) ? 0 : 1, \
              .sub_opcode = SUB_OPCODE_STAGEB, \
              .opcode = OPCODE_BRANCH} \
    }

// Переходы к меткам по регистру STAGE.

/** @brief Переход к метке label_num, если STAGE < imm_value. */
#define M_STAGEBL(label_num, imm_value) M_BRANCH(label_num), I_STAGEBL(0, imm_value)

/** @brief Переход к метке label_num, если STAGE >= imm_value. */
#define M_STAGEBGE(label_num, imm_value) M_BRANCH(label_num), I_STAGEBGE(0, imm_value)


namespace esp::ps2
{

esp_err_t ulp_process_macros_and_load_ex(std::uint32_t load_addr, const ulp_insn_t *program,
                                         std::size_t *psize);

}
