// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


#include "esp_gfx_rect.h"


namespace esp::gfx
{

bool clip_line(int &x1, int &y1, int &x2, int &y2, const rect &clip_rect, bool check_only);

}
