/**
 * @file esp_app.h
 * @brief Объявление класса приложения esp::app.
 */

/**
 * @file esp_app.cpp
 * @brief Приложение: сведения о плате при запуске и её проверка.
 */

#include "esp_app.h"

#include "esp_esp32.h"

#include <cinttypes>
#include <cstdio>
#include <string>

#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>


/**
 * @class esp::app
 * @brief Приложение прошивки для платы LilyGO TTGO VGA32 v1.4.
 *
 * При создании настраивает кнопку K1, запускает проверку платы
 * (esp::board_test: VGA, клавиатура и мышь PS/2, звук) и выводит в
 * консоль сведения о плате. Затем exec() в бесконечном цикле вызывает
 * dispatch(), которая опрашивает кнопку и передаёт проверке события
 * клавиатуры и мыши.
 */


namespace esp::p
{

/** @brief Тег журнала модуля. */
const char *const TAG = "app";

/**
 * @brief Модули, отладочные сообщения которых выводятся в сборках
 *        с включёнными проверками (без NDEBUG).
 */
const char *const DEBUG_TAGS[] = {"app", "button", "sdcard", "test"};

/**
 * @brief Кнопка K1 платы TTGO VGA32 v1.4.
 *
 * Вывод только на вход, с внешней подтяжкой к питанию (10 кОм): нажатая
 * кнопка замыкает его на землю. Вторая кнопка платы, K2, — сброс.
 */
constexpr gpio_num_t BUTTON_GPIO = GPIO_NUM_36;

/**
 * @brief Период опроса кнопки, клавиатуры и мыши, мс.
 *
 * Нажатие кнопки засчитывается, когда уровень держится несколько периодов
 * подряд (подавление дребезга, см. esp::button::pressed()).
 */
constexpr int POLL_PERIOD_MS = 10;

}


// Открытые

/**
 * @brief Создаёт приложение.
 *
 * Конструкторы членов настраивают вывод кнопки и запускают проверку платы:
 * включают VGA, PS/2 и звук и рисуют проверочный экран. Затем в консоль
 * выводятся сведения о плате (см. hello()); объём свободной памяти в них
 * учитывает память, занятую видеовыводом, PS/2 и звуком.
 */
esp::app::app()
    : _button{esp::p::BUTTON_GPIO, "K1"}
{
#ifndef NDEBUG
    // Вызовы ESP_LOGD программы компилируются всегда (LOG_LOCAL_LEVEL в
    // src/CMakeLists.txt), а выводятся только здесь, в сборках с
    // проверками; уровень журнала компонентов ESP-IDF не меняется.
    for (const char *tag : esp::p::DEBUG_TAGS)
        esp_log_level_set(tag, ESP_LOG_DEBUG);
#endif

    // Слот для карты памяти на плате есть (CS 13, MOSI 12, MISO 2, CLK 14),
    // но монтирование пока не нужно.
    // if (!_sd_card.mount())
    //     ESP_LOGW(esp::p::TAG, "No SD card.");

    hello();

    ESP_LOGD(esp::p::TAG, "Pins: button K1 GPIO%d.", static_cast<int>(esp::p::BUTTON_GPIO));
    ESP_LOGD(esp::p::TAG, "Application initialized.");
}

/**
 * @brief Завершает работу приложения.
 *
 * На практике не вызывается: exec() не возвращает управление.
 */
esp::app::~app()
{
    ESP_LOGD(esp::p::TAG, "Application finished.");
}

/**
 * @brief Выполняет один шаг основного цикла.
 *
 * Ждёт период esp::p::POLL_PERIOD_MS, опрашивает кнопку K1 и передаёт проверке
 * платы накопившиеся события клавиатуры и мыши.
 */
void esp::app::dispatch()
{
    // Задержка задаёт период опроса и отдаёт процессор другим задачам,
    // в том числе задаче простоя, за которой следит сторожевой таймер.
    vTaskDelay(pdMS_TO_TICKS(esp::p::POLL_PERIOD_MS));

    if (_button.pressed())
        _test.button_pressed();

    _test.poll();
}

/**
 * @brief Запускает основной цикл приложения.
 *
 * Не возвращает управление.
 */
void esp::app::exec()
{
    while (true)
        dispatch();
}


// Закрытые

/**
 * @brief Выводит в консоль заставку со сведениями о плате.
 *
 * Слева — чашка кофе с надписью «Espresso», справа — версия прошивки,
 * корпус и ревизия чипа, число ядер, радиоинтерфейсы, объём флеш-памяти
 * и PSRAM, общий и свободный объём памяти, карта памяти, MAC-адрес и
 * версия ESP-IDF.
 */
void esp::app::hello()
{
    // Строка формата для printf: каждая строка — кусок рисунка и одно
    // поле сведений.
    static const char *const BANNER_FMT =
        R"(                         Version ........... %s
        (  (             Chip .............. %s
         )  )            Silicon revision .. v%d.%d
        (  (             CPU cores ......... %d
     .---------.         Wireless .......... %s
     |         |--.      Flash ............. %s
     |         |  |      PSRAM ............. %s
     \         /--'      Available memory .. %zu kB
   ___`-------'___       Free memory ....... %zu kB
  `---------------'      SD card ........... %s
      Espresso           MAC address ....... %02x:%02x:%02x:%02x:%02x:%02x
                         ESP-IDF ........... %s
)";

    esp_chip_info_t info;
    esp_chip_info(&info);

    // Корпус известен только для ESP32; для других чипов выводится имя
    // цели сборки.
    const esp::chip_package package = esp::detected_chip_package();
    const char *const chip =
        package == esp::chip_package::unknown ? CONFIG_IDF_TARGET : esp::chip_package_name(package);

    // Радиоинтерфейсы — по флагам возможностей чипа.
    std::string wireless;
    {
        const struct
        {
            std::uint32_t feature;
            const char *name;
        } features[] = {
            {CHIP_FEATURE_WIFI_BGN, "Wi-Fi"},
            {CHIP_FEATURE_BT, "BT"},
            {CHIP_FEATURE_BLE, "BLE"},
            {CHIP_FEATURE_IEEE802154, "802.15.4"},
        };

        for (const auto &f : features)
        {
            if (info.features & f.feature)
            {
                if (!wireless.empty())
                    wireless += '/';
                wireless += f.name;
            }
        }

        if (wireless.empty())
            wireless = "none";
    }

    // Объём самой микросхемы флеш-памяти, а не размер из настроек проекта.
    char flash[48];
    {
        std::uint32_t size = 0;
        if (esp_flash_get_physical_size(nullptr, &size) == ESP_OK)
            std::snprintf(flash, sizeof(flash), "%" PRIu32 " MB %s", size / (1024 * 1024),
                          (info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        else
            std::snprintf(flash, sizeof(flash), "%s", "unknown");
    }

    // PSRAM, подключённая к куче; ноль, если её поддержка выключена в
    // настройках, даже когда микросхема есть.
    char psram[48];
    {
        const size_t size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        if (size)
            std::snprintf(psram, sizeof(psram), "%zu kB %s", size / 1024,
                          (info.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded" : "external");
        else
            std::snprintf(psram, sizeof(psram), "%s",
                          (info.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded, not enabled"
                                                                   : "not used");
    }

    char sdcard[48];
    {
        if (_sd_card.mounted())
            std::snprintf(sdcard, sizeof(sdcard), "%" PRIu64 " MB",
                          _sd_card.capacity() / (1024 * 1024));
        else
            std::snprintf(sdcard, sizeof(sdcard), "%s", "No SD card");
    }

    // Заводской MAC-адрес из eFuse; в QEMU он нулевой.
    std::uint8_t mac[6] = {};
    esp_efuse_mac_get_default(mac);

    const esp_app_desc_t *const desc = esp_app_get_description();

    // Ревизия в esp_chip_info_t записана как MXX: 301 означает v3.1.
    // Память — куча с 32-битным доступом, включая IRAM.
    std::printf(BANNER_FMT, desc->version, chip, info.revision / 100, info.revision % 100,
                info.cores, wireless.c_str(), flash, psram,
                heap_caps_get_total_size(MALLOC_CAP_32BIT) / 1024,
                heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024, sdcard, mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5], esp_get_idf_version());

    ESP_LOGD(esp::p::TAG, "Package %s, features 0x%08" PRIx32 ", largest free block %zu bytes.",
             esp::chip_package_name(package), info.features,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
