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

    /** @brief Точка монтирования файловой системы в VFS. */
    const char *_mount_point;

    /** @brief Описание карты от драйвера SD/MMC; @c nullptr, если карта не смонтирована. */
    sdmmc_card_t *_card;

    /** @brief Контроллер SPI, на котором смонтирована карта; -1, если карта не смонтирована. */
    int _slot;
};

}


// Реализация

// Открытые

/**
 * @brief Создаёт объект; карта при этом не монтируется.
 */
esp::sd_card::sd_card()
    : _mount_point("/sdcard")
    , _card(nullptr)
    , _slot(-1)
{
}

/**
 * @brief Отмонтирует карту, если она смонтирована.
 */
esp::sd_card::~sd_card()
{
    umount();
}

/**
 * @brief Проверяет, смонтирована ли карта.
 * @return @c true, если карта смонтирована.
 */
bool esp::sd_card::mounted() const
{
    return _card;
}

/**
 * @brief Возвращает точку монтирования.
 * @return Путь, по которому файловая система карты видна в VFS: «/sdcard».
 */
const char *esp::sd_card::mount_point() const
{
    return _mount_point;
}
