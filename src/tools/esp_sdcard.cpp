/**
 * @file esp_sdcard.h
 * @brief Объявление класса карты памяти esp::sd_card.
 */

/**
 * @file esp_sdcard.cpp
 * @brief Карта памяти SD на шине SPI с файловой системой FAT.
 */

#include "esp_sdcard.h"

#include "esp_esp32.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"


/**
 * @class esp::sd_card
 * @brief Карта памяти SD на шине SPI с файловой системой FAT.
 *
 * После монтирования файлы карты доступны через стандартные функции
 * ввода-вывода по пути mount_point(). Выводы шины выбираются по корпусу
 * чипа (см. mount()); на плате TTGO VGA32 (ESP32-PICO-D4) слот подключён
 * к выводам CS 13, MOSI 12, MISO 2, CLK 14.
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "sdcard";

}


// Открытые

/**
 * @brief Монтирует карту памяти.
 *
 * Выводы шины SPI зависят от корпуса чипа: для ESP32-PICO-D4 (плата
 * TTGO VGA32) — MISO 2, MOSI 12; для ESP32-D0WD — MISO 35, MOSI 12; для
 * остальных — MISO 16, MOSI 17. Тактовый сигнал всегда на выводе 14,
 * выбор карты — на выводе 13. После монтирования сведения о карте
 * выводятся в консоль.
 *
 * @return @c true, если карта смонтирована, в том числе раньше.
 */
bool esp::sd_card::mount()
{
    if (_card)
        return true;

    ESP_LOGD(TAG, "Mounting SD card ...");

    const int MAX_FILES = 4; // Сколько файлов можно держать открытыми одновременно.
    const size_t ALLOCATION_UNIT_SIZE = 16 * 1024; // Размер кластера в байтах.
    int miso; // Вывод MISO: по умолчанию 16 для WROOM-32, 2 для PICO-D4.
    int mosi; // Вывод MOSI: по умолчанию 17 для WROOM-32, 12 для PICO-D4.
    const int CLK = 14; // Вывод тактового сигнала.
    const int CS = 13; // Вывод выбора карты.

    // По корпусу чипа угадывается плата, а по плате — разводка выводов.
    switch (esp::detected_chip_package())
    {
        case chip_package::pico_d4:
            ESP_LOGD(TAG, "TTGO VGA32 detected.");
            miso = 2;
            mosi = 12;
            break;
        case chip_package::d0wdq5:
            miso = 35;
            mosi = 12;
            break;
        default:
            miso = 16;
            mosi = 17;
            break;
    }

    ESP_LOGD(TAG, "SPI pins: MISO %d, MOSI %d, CLK %d, CS %d.", miso, mosi, CLK, CS);

    // Параметры монтирования. Если включить format_if_mount_failed, карта,
    // которую не удалось смонтировать, будет заново размечена и
    // отформатирована — со всеми её данными.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = MAX_FILES;
    mount_config.allocation_unit_size = ALLOCATION_UNIT_SIZE;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    const spi_host_device_t host_id = static_cast<spi_host_device_t>(host.slot);

    // Неиспользуемые линии шины отмечаются значением -1.
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = mosi;
    bus_cfg.miso_io_num = miso;
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

    // Слот без сигналов обнаружения карты (CD) и защиты от записи (WP);
    // если они есть на плате, их задают в slot_config.gpio_cd и
    // slot_config.gpio_wp.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = esp::int2gpio(CS);
    slot_config.host_id = host_id;

    ret = esp_vfs_fat_sdspi_mount(_mount_point, &host, &slot_config, &mount_config, &_card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "Failed to mount filesystem: %s.", esp_err_to_name(ret));
        else
            ESP_LOGE(TAG, "Failed to initialize the card: %s.", esp_err_to_name(ret));

        // Шина освобождается, чтобы следующая попытка начиналась с нуля.
        _card = nullptr;
        spi_bus_free(host_id);
        return false;
    }

    _slot = host.slot;

    // Карта инициализирована: вывести её параметры.
    sdmmc_card_print_info(stdout, _card);

    ESP_LOGD(TAG, "SD card has been mounted to '%s'.", _mount_point);

    return true;
}

/**
 * @brief Отмонтирует карту и освобождает шину SPI.
 * @return @c true, если карта была смонтирована и успешно отмонтирована.
 */
bool esp::sd_card::umount()
{
    if (!_card)
        return false;

    // Отмонтировать файловую систему и отключить периферию SD/MMC; шина
    // освобождается даже при ошибке, чтобы объект вернулся в исходное
    // состояние.
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

/**
 * @brief Возвращает ёмкость карты.
 * @return Ёмкость в байтах; 0, если карта не смонтирована.
 */
std::uint64_t esp::sd_card::capacity() const
{
    // В регистре CSD ёмкость задана числом секторов.
    return _card ? (std::uint64_t)_card->csd.capacity * _card->csd.sector_size : 0;
}
