#include "esp_sdcard.h"

#include "esp_esp32.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"


namespace
{

const char *const TAG = "sdcard";

}


// Public

bool esp::sd_card::mount()
{
    if (_card)
        return true;

    ESP_LOGD(TAG, "Mounting SD card ...");

    const int MAX_FILES = 4; // Number of files that can be open at the time.
    const size_t ALLOCATION_UNIT_SIZE = 16 * 1024; // Allocation unit size in bytes.
    int MISO; // MISO Pin for MISO signal (default 16 for WROOM-32, 2 for PICO-D4).
    int MOSI; // MOSI Pin for MOSI signal (default 17 for WROOM-32, 12 for PICO-D4).
    const int CLK = 14; // CLK Pin for CLK signal (default 14).
    const int CS = 13; // CS Pin for CS signal (default 13).

    switch (esp::chip_package())
    {
        case ChipPackage::PicoD4:
            ESP_LOGD(TAG, "TTGO VGA32 detected.");
            MISO = 2;
            MOSI = 12;
            break;
        case ChipPackage::D0WDQ5:
            MISO = 35;
            MOSI = 12;
            break;
        default:
            MISO = 16;
            MOSI = 17;
            break;
    }

    ESP_LOGD(TAG, "SPI pins: MISO %d, MOSI %d, CLK %d, CS %d.", MISO, MOSI, CLK, CS);

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = MAX_FILES;
    mount_config.allocation_unit_size = ALLOCATION_UNIT_SIZE;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    const spi_host_device_t host_id = static_cast<spi_host_device_t>(host.slot);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = MOSI;
    bus_cfg.miso_io_num = MISO;
    bus_cfg.sclk_io_num = CLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.data4_io_num = -1;
    bus_cfg.data5_io_num = -1;
    bus_cfg.data6_io_num = -1;
    bus_cfg.data7_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;

    esp_err_t ret = spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize SD card SPI bus: %s.", esp_err_to_name(ret));
        return false;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = esp::int2gpio(CS);
    slot_config.host_id = host_id;

    ret = esp_vfs_fat_sdspi_mount(_mount_point, &host,
                                  &slot_config, &mount_config, &_card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "Failed to mount filesystem: %s.", esp_err_to_name(ret));
        else
            ESP_LOGE(TAG, "Failed to initialize the card: %s.", esp_err_to_name(ret));

        _card = nullptr;
        spi_bus_free(host_id);
        return false;
    }

    _slot = host.slot;

    // Card has been initialized, print its properties.
    sdmmc_card_print_info(stdout, _card);

    ESP_LOGD(TAG, "SD card has been mounted to '%s'.", _mount_point);

    return true;
}

bool esp::sd_card::umount()
{
    if (!_card)
        return false;

    // All done, unmount partition and disable SDMMC peripheral.
    const esp_err_t ret = esp_vfs_fat_sdcard_unmount(_mount_point, _card);
    _card = nullptr;
    spi_bus_free(static_cast<spi_host_device_t>(_slot));
    _slot = -1;

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s.", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGD(TAG, "SD card has been unmounted.");

    return true;
}

std::uint64_t esp::sd_card::capacity() const
{
    return _card
        ? (std::uint64_t)_card->csd.capacity * _card->csd.sector_size
        : 0;
}
