/**
 * @file esp_app.h
 * @brief Объявление класса приложения esp::app.
 */

/**
 * @file esp_app.cpp
 * @brief Приложение: терминал, сведения о плате при запуске и оболочка Форта.
 */

#include "esp_app.h"

#include "esp_esp32.h"

#include <array>
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
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>


/**
 * @class esp::app
 * @brief Приложение прошивки для платы LilyGO TTGO VGA32 v1.4.
 *
 * При создании настраивает кнопку K1, монтирует SD-карту, если она
 * вставлена, запускает терминал esp::vt::terminal (монитор VGA,
 * клавиатура PS/2, UART0), выводит на него сведения о плате и запускает
 * оболочку Форта esp::shell::forth_shell. Затем
 * exec() в бесконечном цикле вызывает dispatch(), которая опрашивает
 * кнопку: короткое нажатие K1 прерывает код Форта, долгое заново ищет
 * клавиатуру и мышь (раздел 12 документа об устройстве).
 */


namespace esp::p
{

/** @brief Тег журнала модуля. */
const char *const TAG = "app";

/**
 * @brief Модули, отладочные сообщения которых выводятся в сборках
 *        с включёнными проверками (без NDEBUG).
 */
const char *const DEBUG_TAGS[] = {"app", "button", "sdcard", "test", "vt", "shell"};

/**
 * @brief Кнопка K1 платы TTGO VGA32 v1.4.
 *
 * Вывод только на вход, с внешней подтяжкой к питанию (10 кОм): нажатая
 * кнопка замыкает его на землю. Вторая кнопка платы, K2, — сброс.
 */
constexpr gpio_num_t BUTTON_GPIO = GPIO_NUM_36;

/**
 * @brief Период опроса кнопки K1, мс.
 *
 * Нажатие кнопки засчитывается, когда уровень держится несколько периодов
 * подряд (подавление дребезга, см. esp::button::pressed()).
 */
constexpr int POLL_PERIOD_MS = 10;

/**
 * @brief Сколько держат K1 для долгого нажатия, мкс.
 *
 * Нажатие короче — короткое; предварительно, уточнит опыт (раздел 14
 * документа об устройстве).
 */
constexpr std::int64_t LONG_PRESS_US = 1000000;

}


// Открытые

/**
 * @brief Создаёт приложение.
 *
 * Конструктор кнопки настраивает её вывод. Затем монтируется SD-карта —
 * без неё оболочка держит историю строк только в памяти, — запускается
 * терминал — VGA, PS/2, UART0, — на него выводятся сведения о плате (см.
 * hello(); объём свободной памяти в них учитывает память терминала), и
 * запускается оболочка Форта.
 */
esp::app::app()
    : _button{esp::p::BUTTON_GPIO, "K1"}
    , _terminal{}
    , _shell{_terminal, _sd_card}
    , _press_start_us{0}
    , _is_press_pending{false}
{
    // Карта памяти (CS 13, MOSI 12, MISO 2, CLK 14) монтируется до
    // терминала: без карты ESP-IDF сообщает в журнал об ошибках, а до
    // запуска терминала журнал идёт только в UART0, мимо экрана.
    if (!_sd_card.mount())
        ESP_LOGW(esp::p::TAG, "No SD card.");

    _terminal.begin();
    hello();

    // Журнал теперь идёт через терминал, и на экран попадают только
    // предупреждения и ошибки (раздел 6.4 документа об устройстве).
    esp_log_level_set("*", ESP_LOG_WARN);
#ifndef NDEBUG
    // Вызовы ESP_LOGD программы компилируются всегда (LOG_LOCAL_LEVEL в
    // src/CMakeLists.txt), а выводятся только здесь, в сборках с
    // проверками, для модулей из esp::p::DEBUG_TAGS.
    for (const char *tag : esp::p::DEBUG_TAGS)
        esp_log_level_set(tag, ESP_LOG_DEBUG);
#endif

    ESP_LOGD(esp::p::TAG, "Pins: button K1 GPIO%d.", static_cast<int>(esp::p::BUTTON_GPIO));
    ESP_LOGD(esp::p::TAG, "Application initialized.");
    _shell.start();
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
 * Ждёт период esp::p::POLL_PERIOD_MS и опрашивает кнопку K1. Во время
 * проверки платы нажатие K1 сразу достаётся ей
 * (esp::shell::forth_shell::button_pressed()). В остальное время
 * нажатие, которое отпустили раньше esp::p::LONG_PRESS_US, — короткое:
 * оно прерывает выполняющийся код Форта и сбрасывает непрочитанный ввод,
 * как Ctrl+C (esp::vt::terminal::interrupt()), а если прерывать нечего,
 * отвечает звонком. Долгое нажатие, как только кнопку продержали
 * esp::p::LONG_PRESS_US, отвечает звонком, и терминал заново ищет
 * клавиатуру и мышь, подключённые после запуска
 * (esp::vt::terminal::check_devices()).
 */
void esp::app::dispatch()
{
    // Задержка задаёт период опроса и отдаёт процессор другим задачам,
    // в том числе задаче простоя, за которой следит сторожевой таймер.
    vTaskDelay(pdMS_TO_TICKS(esp::p::POLL_PERIOD_MS));

    if (_button.pressed())
    {
        _press_start_us = esp_timer_get_time();
        _is_press_pending = !_shell.button_pressed();
        return;
    }
    if (!_is_press_pending)
        return;

    if (_button.is_down())
    {
        // Кнопку ещё держат: нажатие станет долгим, когда пройдёт порог.
        if (esp_timer_get_time() - _press_start_us < esp::p::LONG_PRESS_US)
            return;
        _is_press_pending = false;
        _terminal.write("\a");
        _terminal.check_devices();
        return;
    }

    // Кнопку отпустили до порога: нажатие короткое.
    _is_press_pending = false;
    if (!_terminal.interrupt())
        _terminal.write("\a");
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
 * @brief Выводит на терминал заставку со сведениями о плате.
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
    // Память — куча с 32-битным доступом, включая IRAM. Буфер статический:
    // стек главной задачи — всего 3,5 КБ.
    static std::array<char, 1024> text;
    std::snprintf(text.data(), text.size(), BANNER_FMT, desc->version, chip, info.revision / 100,
                  info.revision % 100, info.cores, wireless.c_str(), flash, psram,
                  heap_caps_get_total_size(MALLOC_CAP_32BIT) / 1024,
                  heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024, sdcard, mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5], esp_get_idf_version());
    _terminal.write(text.data());

    ESP_LOGD(esp::p::TAG, "Package %s, features 0x%08" PRIx32 ", largest free block %zu bytes.",
             esp::chip_package_name(package), info.features,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
