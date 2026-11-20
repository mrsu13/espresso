/**
 * @file esp_app.h
 * @brief Объявление класса приложения esp::app.
 */

/**
 * @file esp_app.cpp
 * @brief Приложение: сведения о плате при запуске и управление экраном кнопками.
 */

#include "esp_app.h"

#include "esp_esp32.h"

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <cinttypes>
#include <cstdio>
#include <iterator>
#include <string>
#include <type_traits>


/**
 * @class esp::app
 * @brief Приложение прошивки для платы LilyGO TTGO T-Display.
 *
 * При создании настраивает подсветку, дисплей и кнопки, гасит экран и
 * выводит в консоль сведения о плате. Затем exec() в бесконечном цикле
 * вызывает dispatch(), которая опрашивает кнопки: каждое нажатие любой
 * из них включает или выключает экран (см. toggle_screen()).
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "app";

/**
 * @brief Модули, отладочные сообщения которых выводятся в сборках
 *        с включёнными проверками (без NDEBUG).
 */
const char *const DEBUG_TAGS[] = {"app", "backlight", "button", "display", "sdcard"};

/** @brief Вывод подсветки дисплея T-Display: высокий уровень включает подсветку. */
constexpr gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_4;

/** @brief Левая кнопка T-Display (BOOT), если разъём USB смотрит вниз. */
constexpr gpio_num_t BUTTON_LEFT_GPIO = GPIO_NUM_0;

/** @brief Правая кнопка T-Display, если разъём USB смотрит вниз. */
constexpr gpio_num_t BUTTON_RIGHT_GPIO = GPIO_NUM_35;

/**
 * @brief Период опроса кнопок, мс.
 *
 * Нажатие засчитывается, когда уровень держится несколько периодов подряд
 * (подавление дребезга, см. esp::button::pressed()).
 */
constexpr int POLL_PERIOD_MS = 10;

/**
 * @brief Включает временный режим поиска вывода подсветки.
 *
 * В этом режиме левая кнопка переключает GPIO4, правая — GPIO14 (вначале
 * на обоих низкий уровень), а каждое изменение уровня на выводах кнопок
 * выводится в журнал. При @c false кнопки включают и выключают экран.
 */
constexpr bool PIN_PROBE = false;

/** @brief Проверяемые выводы, по одному на кнопку: для левой и для правой. */
constexpr gpio_num_t PROBE_GPIOS[] = {GPIO_NUM_4, GPIO_NUM_14};

/** @brief Состояние режима поиска вывода. */
struct probe_state
{
    /** @brief Последний прочитанный уровень на выводе каждой кнопки. */
    int raw[std::size(PROBE_GPIOS)];

    /** @brief Текущий уровень на каждом проверяемом выводе. */
    int out[std::size(PROBE_GPIOS)];
};

/** @brief Состояние режима поиска вывода; приложение существует в одном экземпляре. */
probe_state probe = {};

}


// Открытые

/**
 * @brief Создаёт приложение.
 *
 * Конструкторы членов настраивают выводы подсветки и кнопок и
 * инициализируют дисплей. Затем экран заливается чёрным при выключенной
 * подсветке, и в консоль выводятся сведения о плате (см. hello()).
 */
esp::app::app()
    : _backlight(BACKLIGHT_GPIO)
    , _buttons{button(BUTTON_LEFT_GPIO, "Left"), button(BUTTON_RIGHT_GPIO, "Right")}
{
#ifndef NDEBUG
    // Вызовы ESP_LOGD модуля main компилируются всегда (LOG_LOCAL_LEVEL в
    // main/CMakeLists.txt), а выводятся только здесь, в сборках с
    // проверками; уровень журнала компонентов ESP-IDF не меняется.
    for (const char *tag : DEBUG_TAGS)
        esp_log_level_set(tag, ESP_LOG_DEBUG);
#endif

    // На T-Display нет слота для карты памяти, а попытка монтирования
    // задействует GPIO14 и GPIO16 (линию данных и команд дисплея).
    // if (!_sd_card.mount())
    //     ESP_LOGW(TAG, "No SD card.");

    // При запуске экран выключен: залит чёрным, подсветка погашена.
    if (!_display.fill(display::BLACK))
        ESP_LOGW(TAG, "No display.");

    hello();

    ESP_LOGD(TAG, "Pins: backlight GPIO%d, buttons GPIO%d and GPIO%d.",
             (int)BACKLIGHT_GPIO, (int)BUTTON_LEFT_GPIO, (int)BUTTON_RIGHT_GPIO);
    if (PIN_PROBE)
    {
        static_assert(std::extent_v<decltype(_buttons)> == std::size(PROBE_GPIOS),
                      "one probed pin per button");

        // Оба проверяемых вывода — выходы с низким уровнем.
        for (const gpio_num_t gpio : PROBE_GPIOS)
        {
            gpio_set_level(gpio, 0);
            if (gpio == BACKLIGHT_GPIO)
                continue; // Уже настроен как выход объектом _backlight.

            gpio_config_t config = {};
            config.pin_bit_mask = 1ULL << gpio;
            config.mode = GPIO_MODE_OUTPUT;
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_DISABLE;
            ESP_ERROR_CHECK(gpio_config(&config));
        }

        for (size_t i = 0; i < std::size(_buttons); ++i)
            probe.raw[i] = gpio_get_level(_buttons[i].gpio());

        ESP_LOGI(TAG, "Pin probe: the left button toggles GPIO%d, the right one GPIO%d; both are low now.",
                 (int)PROBE_GPIOS[0], (int)PROBE_GPIOS[1]);
        ESP_LOGI(TAG, "Raw levels: GPIO%d = %d, GPIO%d = %d.",
                 (int)_buttons[0].gpio(), probe.raw[0], (int)_buttons[1].gpio(), probe.raw[1]);
    }

    ESP_LOGD(TAG, "Application initialized.");
}

/**
 * @brief Завершает работу приложения.
 *
 * На практике не вызывается: exec() не возвращает управление.
 */
esp::app::~app()
{
    ESP_LOGD(TAG, "Application finished.");
}

/**
 * @brief Выполняет один шаг основного цикла.
 *
 * Ждёт период POLL_PERIOD_MS и опрашивает кнопки. Нажатие любой кнопки
 * включает или выключает экран, а в режиме поиска вывода (PIN_PROBE)
 * переключает уровень на проверяемом выводе этой кнопки.
 */
void esp::app::dispatch()
{
    // Задержка задаёт период опроса и отдаёт процессор другим задачам,
    // в том числе задаче простоя, за которой следит сторожевой таймер.
    vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));

    for (size_t i = 0; i < std::size(_buttons); ++i)
    {
        button &b = _buttons[i];

        if (PIN_PROBE)
        {
            // Уровень читается до подавления дребезга: в журнале виден
            // и сам дребезг.
            const int raw = gpio_get_level(b.gpio());
            if (raw != probe.raw[i])
            {
                probe.raw[i] = raw;
                ESP_LOGI(TAG, "Raw: GPIO%d = %d.", (int)b.gpio(), raw);
            }
        }

        if (!b.pressed())
            continue;

        if (PIN_PROBE)
        {
            probe.out[i] = !probe.out[i];
            gpio_set_level(PROBE_GPIOS[i], probe.out[i]);
            ESP_LOGI(TAG, "%s button: GPIO%d is now %s; watch the screen.",
                     b.name(), (int)PROBE_GPIOS[i], probe.out[i] ? "high" : "low");
        }
        else
        {
            toggle_screen();
        }
    }
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
 * @brief Включает или выключает экран.
 *
 * Включение: экран заливается белым, затем зажигается подсветка.
 * Выключение: подсветка гаснет, затем экран заливается чёрным.
 */
void esp::app::toggle_screen()
{
    // Заливка всегда идёт при погашенной подсветке, поэтому смена
    // изображения не видна. Без подсветки ЖК-матрица всё равно слабо
    // показывает содержимое памяти, поэтому выключенный экран — чёрный.
    if (_backlight.is_on())
    {
        _backlight.off();
        _display.fill(display::BLACK);
    }
    else
    {
        _display.fill(display::WHITE);
        _backlight.on();
    }
}

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
    const esp::ChipPackage package = esp::chip_package();
    const char *const chip = package == esp::ChipPackage::Unknown
        ? CONFIG_IDF_TARGET
        : esp::chip_package_name(package);

    // Радиоинтерфейсы — по флагам возможностей чипа.
    std::string wireless;
    {
        const struct
        {
            std::uint32_t feature;
            const char *name;
        } features[] =
        {
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
            std::snprintf(flash, sizeof(flash), "%" PRIu32 " MB %s",
                          size / (1024 * 1024),
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
                          (info.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded, not enabled" : "not used");
    }

    char sdcard[48];
    {
        if (_sd_card.mounted())
            std::snprintf(sdcard, sizeof(sdcard), "%" PRIu64 " MB", _sd_card.capacity() / (1024 * 1024));
        else
            std::snprintf(sdcard, sizeof(sdcard), "%s", "No SD card");
    }

    // Заводской MAC-адрес из eFuse; в QEMU он нулевой.
    std::uint8_t mac[6] = {};
    esp_efuse_mac_get_default(mac);

    const esp_app_desc_t *const desc = esp_app_get_description();

    // Ревизия в esp_chip_info_t записана как MXX: 301 означает v3.1.
    // Память — куча с 32-битным доступом, включая IRAM.
    std::printf(BANNER_FMT,
                desc->version,
                chip,
                info.revision / 100, info.revision % 100,
                info.cores,
                wireless.c_str(),
                flash,
                psram,
                heap_caps_get_total_size(MALLOC_CAP_32BIT) / 1024,
                heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024,
                sdcard,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                esp_get_idf_version());

    ESP_LOGD(TAG, "Package %s, features 0x%08" PRIx32 ", largest free block %zu bytes.",
             esp::chip_package_name(package), info.features,
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
