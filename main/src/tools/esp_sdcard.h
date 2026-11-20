#pragma once

#include "sd_protocol_types.h"

#include <cstdint>


namespace esp
{

class sd_card
{

public:

    inline sd_card();
    inline ~sd_card();

    bool mount();
    bool umount();
    inline bool mounted() const;
    inline const char *mount_point() const;
    std::uint64_t capacity() const;

private:

    const char *_mount_point;
    sdmmc_card_t *_card;
    int _slot;
};

}


// Implementation

// Public

esp::sd_card::sd_card()
    : _mount_point("/sdcard")
    , _card(nullptr)
    , _slot(-1)
{
}

esp::sd_card::~sd_card()
{
    umount();
}

bool esp::sd_card::mounted() const
{
    return _card;
}

const char *esp::sd_card::mount_point() const
{
    return _mount_point;
}
