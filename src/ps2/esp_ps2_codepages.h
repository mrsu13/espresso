// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains codepages declarations
 *
 */


#include "esp_ps2_utils.h"
#include "esp_tools.h"


namespace esp::ps2
{


// associates virtual key to ASCII code
struct virtual_key_ascii
{
    virtual_key vk;
    uint8_t ascii;
};


struct code_page
{
    uint16_t codepage;
    const virtual_key_ascii *conv_table; // last item has vk = VK_NONE (ending marker)
};


extern const code_page CODE_PAGE_437;
extern const code_page CODE_PAGE_1252;


struct code_pages
{
    static int count()
    {
        return 2;
    }
    static code_page const *get(uint16_t codepage, code_page const *default_value = &CODE_PAGE_437)
    {
        static const code_page *codepages[] = {&CODE_PAGE_437, &CODE_PAGE_1252};
        for (int i = 0; i < sizeof(code_page) / sizeof(code_page *); ++i)
            if (codepages[i]->codepage == codepage)
                return codepages[i];
        return default_value;
    }
};


/**
 * @brief Converts virtual key item to ASCII.
 *
 * This method converts the specified virtual key to ASCII, if possible.<br>
 * For example VK_A is converted to 'A' (ASCII 0x41), CTRL  + VK_SPACE produces ASCII NUL (0x00), CTRL + letter produces
 * ASCII control codes from SOH (0x01) to SUB (0x1A), CTRL + VK_BACKSLASH produces ASCII FS (0x1C), CTRL + VK_QUESTION produces
 * ASCII US (0x1F), CTRL + VK_LEFTBRACKET produces ASCII ESC (0x1B), CTRL + VK_RIGHTBRACKET produces ASCII GS (0x1D),
 * CTRL + VK_TILDE produces ASCII RS (0x1E) and VK_SCROLLLOCK produces XON or XOFF.
 *
 * @param item The virtual key to convert.
 * @param codepage Codepage used to convert language specific characters.
 *
 * @return The ASCII code of virtual key or -1 if virtual key cannot be translated to ASCII.
 */
int virtual_key_to_ascii(virtual_key_item const &item, code_page const *codepage);


} // end of namespace
