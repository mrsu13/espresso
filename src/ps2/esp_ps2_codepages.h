// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_ps2_code_page.h"
#include "esp_ps2_virtual_key_item.h"


namespace esp::ps2
{

extern const code_page CODE_PAGE_437;
extern const code_page CODE_PAGE_1252;

int virtual_key_to_ascii(const virtual_key_item &item, const code_page *codepage);

}
