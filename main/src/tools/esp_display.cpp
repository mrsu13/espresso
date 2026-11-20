/**
 * @file esp_display.h
 * @brief Объявление класса дисплея esp::display.
 */

/**
 * @file esp_display.cpp
 * @brief Дисплей ST7789V платы TTGO T-Display на шине SPI.
 */

#include "esp_display.h"

#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"

#include <algorithm>


/**
 * @class esp::display
 * @brief Дисплей ST7789V платы TTGO T-Display.
 *
 * Экран 135 × 240 точек в книжной ориентации подключён по SPI и
 * управляется драйвером esp_lcd из ESP-IDF. Подсветка управляется
 * отдельно (esp::backlight, GPIO4).
 *
 * Дисплей инициализируется в конструкторе; успех проверяется методом
 * ready(). Изображение передаётся полосами по нескольку строк из одного
 * буфера DMA.
 */


namespace
{

/** @brief Тег журнала модуля. */
const char *const TAG = "display";

/** @brief Контроллер SPI дисплея; SPI2 остаётся свободным для карты памяти. */
constexpr spi_host_device_t SPI_HOST = SPI3_HOST;

/** @brief Вывод MOSI (данные к дисплею) на плате TTGO T-Display. */
constexpr int PIN_MOSI = 19;

/** @brief Вывод SCLK (тактовый сигнал SPI). */
constexpr int PIN_SCLK = 18;

/** @brief Вывод CS (выбор дисплея на шине). */
constexpr gpio_num_t PIN_CS = GPIO_NUM_5;

/** @brief Вывод DC: низкий уровень — команда, высокий — данные. */
constexpr gpio_num_t PIN_DC = GPIO_NUM_16;

/** @brief Вывод сброса контроллера. */
constexpr gpio_num_t PIN_RST = GPIO_NUM_23;

/** @brief Частота тактового сигнала SPI, Гц. */
constexpr unsigned PIXEL_CLOCK_HZ = 40 * 1000 * 1000;

/**
 * @brief Смещение видимого окна по горизонтали.
 *
 * Память контроллера рассчитана на 240 × 320 точек, а видимая область
 * 135 × 240 расположена в ней со смещением (книжная ориентация).
 */
constexpr int X_GAP = 52;

/** @brief Смещение видимого окна по вертикали; см. X_GAP. */
constexpr int Y_GAP = 40;

/** @brief Число строк в одной передаче (полосе). */
constexpr int BAND_LINES = 20;

/** @brief Число полос на весь экран. */
constexpr int BANDS = (esp::display::HEIGHT + BAND_LINES - 1) / BAND_LINES;

/** @brief Размер буфера полосы в байтах. */
constexpr size_t BAND_SIZE = esp::display::WIDTH * BAND_LINES * sizeof(std::uint16_t);

/**
 * @brief Проверяет код ошибки и при ошибке выводит сообщение.
 * @param ret Код ошибки.
 * @param what Имя вызванной функции для сообщения.
 * @return @c true, если @p ret равен @c ESP_OK.
 */
bool check(esp_err_t ret, const char *what)
{
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "%s failed: %s.", what, esp_err_to_name(ret));
    return ret == ESP_OK;
}

/**
 * @brief Обработчик окончания передачи полосы.
 *
 * Драйвер esp_lcd вызывает его из прерывания SPI. Первые два параметра
 * (интерфейс панели и данные события) не используются.
 *
 * @param user_ctx Семафор esp::display::_sent.
 * @return @c true, если пробуждена задача с более высоким приоритетом
 *         и нужно переключение контекста.
 */
bool on_color_trans_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *user_ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(user_ctx), &woken);
    return woken == pdTRUE;
}

}


// Открытые

/**
 * @brief Инициализирует дисплей.
 *
 * При ошибке занятые ресурсы освобождаются, а ready() возвращает @c false.
 */
esp::display::display()
    : _io(nullptr)
    , _panel(nullptr)
    , _sent(nullptr)
    , _band(nullptr)
    , _bus(false)
{
    if (init())
        ESP_LOGI(TAG, "ST7789 %dx%d ready.", WIDTH, HEIGHT);
    else
        release();
}

/**
 * @brief Освобождает панель, шину SPI и буфер.
 */
esp::display::~display()
{
    release();
}

/**
 * @brief Заполняет весь экран одним цветом.
 *
 * Метод возвращает управление, когда все данные переданы.
 *
 * @param color Цвет в формате RGB565.
 * @return @c true при успехе; @c false, если дисплей не инициализирован
 *         или передача не удалась.
 */
bool esp::display::fill(std::uint16_t color)
{
    if (!_panel)
        return false;

    // Контроллер принимает RGB565 старшим байтом вперёд, а процессор
    // хранит числа младшим байтом вперёд: байты меняются местами.
    const std::uint16_t value = static_cast<std::uint16_t>((color >> 8) | (color << 8));
    std::fill(_band, _band + WIDTH * BAND_LINES, value);

    // Каждый вызов ставит передачу полосы в очередь и сразу возвращает
    // управление; перед следующей полосой драйвер сам дожидается окончания
    // предыдущей, поэтому один буфер годится для всего экрана.
    for (int y = 0; y < HEIGHT; y += BAND_LINES)
    {
        if (!check(esp_lcd_panel_draw_bitmap(_panel, 0, y, WIDTH, std::min(y + BAND_LINES, HEIGHT), _band),
                   "esp_lcd_panel_draw_bitmap"))
            return false;
    }

    // Последние передачи ещё идут в фоне: дождаться окончания всех, чтобы
    // следующий вызов мог безопасно заполнить буфер заново. Тайм-аут нужен
    // на случай, когда передачи не завершаются вовсе (так ведёт себя QEMU).
    for (int i = 0; i < BANDS; ++i)
    {
        if (xSemaphoreTake(_sent, pdMS_TO_TICKS(1000)) != pdTRUE)
        {
            ESP_LOGE(TAG, "Timed out waiting for the display transfer.");
            return false;
        }
    }

    ESP_LOGD(TAG, "Screen filled with 0x%04x.", color);

    return true;
}


// Закрытые

/**
 * @brief Инициализирует шину SPI, интерфейс панели и контроллер ST7789.
 * @return @c true при успехе.
 */
bool esp::display::init()
{
    // Семафор отдаётся по одному разу на каждую полосу; буфер полосы
    // должен быть доступен DMA, то есть лежать во внутренней памяти.
    _sent = xSemaphoreCreateCounting(BANDS, 0);
    _band = static_cast<std::uint16_t *>(heap_caps_malloc(BAND_SIZE, MALLOC_CAP_DMA));
    if (!_sent || !_band)
    {
        ESP_LOGE(TAG, "Out of memory.");
        return false;
    }

    // У дисплея нет линии MISO: данные идут только к нему.
    spi_bus_config_t bus = {};
    bus.mosi_io_num = PIN_MOSI;
    bus.miso_io_num = -1;
    bus.sclk_io_num = PIN_SCLK;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.data4_io_num = -1;
    bus.data5_io_num = -1;
    bus.data6_io_num = -1;
    bus.data7_io_num = -1;
    bus.max_transfer_sz = BAND_SIZE;
    if (!check(spi_bus_initialize(SPI_HOST, &bus, SPI_DMA_CH_AUTO), "spi_bus_initialize"))
        return false;
    _bus = true;

    // Команды и параметры по 8 бит, режим SPI 0. По окончании каждой
    // передачи изображения обработчик отдаёт семафор _sent.
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = PIN_CS;
    io_config.dc_gpio_num = PIN_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = on_color_trans_done;
    io_config.user_ctx = _sent;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    if (!check(esp_lcd_new_panel_io_spi(static_cast<esp_lcd_spi_bus_handle_t>(SPI_HOST), &io_config, &_io),
               "esp_lcd_new_panel_io_spi"))
        return false;

    // Драйвер ST7789 входит в ESP-IDF; цвет — 16 бит на точку (RGB565).
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
    panel_config.bits_per_pixel = 16;
    panel_config.reset_gpio_num = PIN_RST;
    if (!check(esp_lcd_new_panel_st7789(_io, &panel_config, &_panel), "esp_lcd_new_panel_st7789"))
        return false;

    // Матрица IPS платы T-Display показывает цвета инвертированными
    // относительно команд ST7789, поэтому инверсия включена. Видимое окно
    // смещено в памяти контроллера на X_GAP и Y_GAP.
    return check(esp_lcd_panel_reset(_panel), "esp_lcd_panel_reset")
        && check(esp_lcd_panel_init(_panel), "esp_lcd_panel_init")
        && check(esp_lcd_panel_invert_color(_panel, true), "esp_lcd_panel_invert_color")
        && check(esp_lcd_panel_set_gap(_panel, X_GAP, Y_GAP), "esp_lcd_panel_set_gap")
        && check(esp_lcd_panel_disp_on_off(_panel, true), "esp_lcd_panel_disp_on_off");
}

/**
 * @brief Освобождает ресурсы в порядке, обратном захвату.
 *
 * Безопасен и после неудачной, частичной инициализации: освобождается
 * только то, что было захвачено.
 */
void esp::display::release()
{
    if (_panel)
    {
        esp_lcd_panel_del(_panel);
        _panel = nullptr;
    }

    if (_io)
    {
        esp_lcd_panel_io_del(_io);
        _io = nullptr;
    }

    if (_bus)
    {
        spi_bus_free(SPI_HOST);
        _bus = false;
    }

    heap_caps_free(_band);
    _band = nullptr;

    if (_sent)
    {
        vSemaphoreDelete(_sent);
        _sent = nullptr;
    }
}
