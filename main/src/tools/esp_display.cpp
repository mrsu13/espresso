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


namespace
{

const char *const TAG = "display";

// TTGO T-Display wiring. SPI2 stays free for an SD card.
constexpr spi_host_device_t SPI_HOST = SPI3_HOST;
constexpr int PIN_MOSI = 19;
constexpr int PIN_SCLK = 18;
constexpr gpio_num_t PIN_CS = GPIO_NUM_5;
constexpr gpio_num_t PIN_DC = GPIO_NUM_16;
constexpr gpio_num_t PIN_RST = GPIO_NUM_23;
constexpr unsigned PIXEL_CLOCK_HZ = 40 * 1000 * 1000;

// Where the visible 135 x 240 window starts in the controller's
// 240 x 320 memory (portrait orientation).
constexpr int X_GAP = 52;
constexpr int Y_GAP = 40;

// Lines sent in one transfer, and the number of transfers per screen.
constexpr int BAND_LINES = 20;
constexpr int BANDS = (esp::display::HEIGHT + BAND_LINES - 1) / BAND_LINES;
constexpr size_t BAND_SIZE = esp::display::WIDTH * BAND_LINES * sizeof(std::uint16_t);

bool check(esp_err_t ret, const char *what)
{
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "%s failed: %s.", what, esp_err_to_name(ret));
    return ret == ESP_OK;
}

// Called from the SPI interrupt when a band has been sent.
bool on_color_trans_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *user_ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(user_ctx), &woken);
    return woken == pdTRUE;
}

}


// Public

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

esp::display::~display()
{
    release();
}

bool esp::display::fill(std::uint16_t color)
{
    if (!_panel)
        return false;

    // The controller takes RGB565 most significant byte first.
    const std::uint16_t value = static_cast<std::uint16_t>((color >> 8) | (color << 8));
    std::fill(_band, _band + WIDTH * BAND_LINES, value);

    for (int y = 0; y < HEIGHT; y += BAND_LINES)
    {
        if (!check(esp_lcd_panel_draw_bitmap(_panel, 0, y, WIDTH, std::min(y + BAND_LINES, HEIGHT), _band),
                   "esp_lcd_panel_draw_bitmap"))
            return false;
    }

    // The transfers run in the background; wait for all of them, so that
    // the band buffer can be refilled safely.
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


// Private

bool esp::display::init()
{
    _sent = xSemaphoreCreateCounting(BANDS, 0);
    _band = static_cast<std::uint16_t *>(heap_caps_malloc(BAND_SIZE, MALLOC_CAP_DMA));
    if (!_sent || !_band)
    {
        ESP_LOGE(TAG, "Out of memory.");
        return false;
    }

    // The display has no MISO line.
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

    // 8-bit commands and parameters, SPI mode 0.
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

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
    panel_config.bits_per_pixel = 16;
    panel_config.reset_gpio_num = PIN_RST;
    if (!check(esp_lcd_new_panel_st7789(_io, &panel_config, &_panel), "esp_lcd_new_panel_st7789"))
        return false;

    // The T-Display's IPS panel shows colours inverted relative to the
    // ST7789 commands, hence the inversion.
    return check(esp_lcd_panel_reset(_panel), "esp_lcd_panel_reset")
        && check(esp_lcd_panel_init(_panel), "esp_lcd_panel_init")
        && check(esp_lcd_panel_invert_color(_panel, true), "esp_lcd_panel_invert_color")
        && check(esp_lcd_panel_set_gap(_panel, X_GAP, Y_GAP), "esp_lcd_panel_set_gap")
        && check(esp_lcd_panel_disp_on_off(_panel, true), "esp_lcd_panel_disp_on_off");
}

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
