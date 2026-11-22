// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#include <alloca.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/rtc.h"

#include "esp_tools.h"
#include "esp_vga_gpio_stream.h"
#include "esp_vga_utils.h"
#include "esp_vga_vga_base_controller.h"


#pragma GCC optimize("O2")


namespace esp::vga
{


#if ESP_VGA_PERFORMANCE_CHECK
volatile uint64_t s_vgapalctrlcycles = 0;
#endif


vga_base_controller::vga_base_controller() {}


void vga_base_controller::init()
{
    esp::current_video_mode::set(esp::video_mode::vga);

    _dma_buffers = nullptr;
    _dma_buffers_count = 0;
    _dma_buffers_head = nullptr;
    _dma_buffers_visible = nullptr;
    _primitive_processing_suspended = 1; // >0 suspended
    _isr_handle = nullptr;
    _double_buffer_over_dma = false;
    _view_ports[0] = _view_ports[1] = nullptr;
    _view_port = nullptr;
    _view_port_visible = nullptr;
    _view_port_memory_pool = nullptr;

    _gpio_stream.begin();
}


// initializer for 8 colors configuration
void vga_base_controller::begin(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
                                gpio_num_t h_sync_gpio, gpio_num_t v_sync_gpio)
{
    init();

    // GPIO configuration for bit 0
    setup_gpio(red_gpio, VGA_RED_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(green_gpio, VGA_GREEN_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(blue_gpio, VGA_BLUE_BIT, GPIO_MODE_OUTPUT);

    // GPIO configuration for VSync and HSync
    setup_gpio(h_sync_gpio, VGA_HSYNC_BIT, GPIO_MODE_OUTPUT);
    setup_gpio(v_sync_gpio, VGA_VSYNC_BIT, GPIO_MODE_OUTPUT);

    esp::gfx::rgb222::low_bit_only = true;
    _bits_per_channel = 1;
}


// initializer for 64 colors configuration
void vga_base_controller::begin(gpio_num_t red_1gpio, gpio_num_t red_0gpio, gpio_num_t green_1gpio,
                                gpio_num_t green_0gpio, gpio_num_t blue_1gpio,
                                gpio_num_t blue_0gpio, gpio_num_t h_sync_gpio,
                                gpio_num_t v_sync_gpio)
{
    begin(red_0gpio, green_0gpio, blue_0gpio, h_sync_gpio, v_sync_gpio);

    // GPIO configuration for bit 1
    setup_gpio(red_1gpio, VGA_RED_BIT + 1, GPIO_MODE_OUTPUT);
    setup_gpio(green_1gpio, VGA_GREEN_BIT + 1, GPIO_MODE_OUTPUT);
    setup_gpio(blue_1gpio, VGA_BLUE_BIT + 1, GPIO_MODE_OUTPUT);

    esp::gfx::rgb222::low_bit_only = false;
    _bits_per_channel = 2;
}


// initializer for default configuration
void vga_base_controller::begin()
{
    begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23,
          GPIO_NUM_15);
}


void vga_base_controller::end()
{
    if (_dma_buffers)
    {
        suspend_background_primitive_execution();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        _gpio_stream.stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (_isr_handle)
        {
            esp_intr_free(_isr_handle);
            _isr_handle = nullptr;
        }
        free_buffers();
    }
}


void vga_base_controller::setup_gpio(gpio_num_t gpio, int bit, gpio_mode_t mode)
{
    configure_gpio(gpio, mode);
    esp_rom_gpio_connect_out_signal(gpio, I2S1O_DATA_OUT0_IDX + bit, false, false);
}


void vga_base_controller::free_buffers()
{
    if (_dma_buffers_count > 0)
    {
        heap_caps_free((void *)_h_blank_line_with_v_sync);
        heap_caps_free((void *)_h_blank_line);

        free_view_port();

        set_dma_buffers_count(0);
    }
}


void vga_base_controller::free_view_port()
{
    if (_view_port_memory_pool)
    {
        for (auto pool_ptr = _view_port_memory_pool; *pool_ptr; ++pool_ptr)
            heap_caps_free((void *)*pool_ptr);
        heap_caps_free(_view_port_memory_pool);
        _view_port_memory_pool = nullptr;
    }
    for (int i = 0; i < 2; ++i)
        if (_view_ports[i])
        {
            heap_caps_free(_view_ports[i]);
            _view_ports[i] = nullptr;
        }
    _view_port = _view_port_visible = nullptr;
}


// Can be used to change buffers count, maintainig already set pointers.
// If m_doubleBufferOverDMA = true, uses m_DMABuffersHead and m_DMABuffersVisible to implement
// double buffer on DMA level.
bool vga_base_controller::set_dma_buffers_count(int buffers_count)
{
    if (buffers_count == 0)
    {
        if (_dma_buffers_visible && _dma_buffers_visible != _dma_buffers)
            heap_caps_free((void *)_dma_buffers_visible);
        heap_caps_free((void *)_dma_buffers);
        _dma_buffers = nullptr;
        _dma_buffers_visible = nullptr;
        _dma_buffers_count = 0;
        return true;
    }

    if (buffers_count != _dma_buffers_count)
    {

        // buffers head
        if (_double_buffer_over_dma && _dma_buffers_head == nullptr)
        {
            _dma_buffers_head = (lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
            _dma_buffers_head->eof = 0;
            _dma_buffers_head->sosf = 0;
            _dma_buffers_head->offset = 0;
            _dma_buffers_head->owner = 1;
            _dma_buffers_head->size = 0;
            _dma_buffers_head->length = 0;
            _dma_buffers_head->buf =
                _h_blank_line; // dummy valid address. Setting nullptr crashes DMA!
            _dma_buffers_head->qe.stqe_next = nullptr; // this will be set before the first frame
        }

        // (re)allocate and initialize DMA descs
        _dma_buffers = (lldesc_t *)heap_caps_realloc(
            (void *)_dma_buffers, buffers_count * sizeof(lldesc_t), MALLOC_CAP_DMA);
        if (_double_buffer_over_dma && is_double_buffered())
            _dma_buffers_visible = (lldesc_t *)heap_caps_realloc(
                (void *)_dma_buffers_visible, buffers_count * sizeof(lldesc_t), MALLOC_CAP_DMA);
        else
            _dma_buffers_visible = _dma_buffers;
        if (!_dma_buffers || !_dma_buffers_visible)
            return false;

        auto buffers_head = _dma_buffers_head ? _dma_buffers_head : &_dma_buffers[0];

        for (int i = 0; i < buffers_count; ++i)
        {
            _dma_buffers[i].eof = 0;
            _dma_buffers[i].sosf = 0;
            _dma_buffers[i].offset = 0;
            _dma_buffers[i].owner = 1;
            _dma_buffers[i].qe.stqe_next =
                (lldesc_t *)(i == buffers_count - 1 ? buffers_head : &_dma_buffers[i + 1]);
            if (_double_buffer_over_dma && is_double_buffered())
            {
                _dma_buffers_visible[i].eof = 0;
                _dma_buffers_visible[i].sosf = 0;
                _dma_buffers_visible[i].offset = 0;
                _dma_buffers_visible[i].owner = 1;
                _dma_buffers_visible[i].qe.stqe_next =
                    (lldesc_t *)(i == buffers_count - 1 ? buffers_head
                                                        : &_dma_buffers_visible[i + 1]);
            }
        }

        _dma_buffers_count = buffers_count;
    }

    return true;
}


// modeline syntax:
//   "label" clock_mhz hdisp hsyncstart hsyncend htotal vdisp vsyncstart vsyncend vtotal [(+HSync | -HSync) (+VSync | -VSync)] [DoubleScan | QuadScan] [FrontPorchBegins | SyncBegins | BackPorchBegins | VisibleBegins] [MultiScanBlank]
bool vga_base_controller::convert_modeline_to_timings(char const *modeline, vga_timings *timings)
{
    float freq;
    int hdisp, hsyncstart, hsyncend, htotal, vdisp, vsyncstart, vsyncend, vtotal;
    char h_sync_pol = 0, v_sync_pol = 0;
    int pos = 0;

    int count =
        sscanf(modeline, "\"%[^\"]\" %g %d %d %d %d %d %d %d %d %n", timings->label, &freq, &hdisp,
               &hsyncstart, &hsyncend, &htotal, &vdisp, &vsyncstart, &vsyncend, &vtotal, &pos);

    if (count == 10 && pos > 0)
    {

        timings->frequency = freq * 1000000;
        timings->h_visible_area = hdisp;
        timings->h_front_porch = hsyncstart - hdisp;
        timings->h_sync_pulse = hsyncend - hsyncstart;
        timings->h_back_porch = htotal - hsyncend;
        timings->v_visible_area = vdisp;
        timings->v_front_porch = vsyncstart - vdisp;
        timings->v_sync_pulse = vsyncend - vsyncstart;
        timings->v_back_porch = vtotal - vsyncend;
        timings->h_sync_logic = '-';
        timings->v_sync_logic = '-';
        timings->scan_count = 1;
        timings->multi_scan_black = 0;
        timings->h_starting_block = vga_scan_start::visible_area;

        // actually this checks just the first character
        auto pc = modeline + pos;
        auto end = pc + strlen(modeline);
        while (*pc && pc < end)
        {
            switch (*pc)
            {
                // parse [(+HSync | -HSync) (+VSync | -VSync)]
                case '+':
                case '-':
                    if (!h_sync_pol)
                        timings->h_sync_logic = h_sync_pol = *pc;
                    else if (!v_sync_pol)
                        timings->v_sync_logic = v_sync_pol = *pc;
                    pc += 6;
                    break;
                // parse [DoubleScan | QuadScan]
                // DoubleScan
                case 'D':
                case 'd':
                    timings->scan_count = 2;
                    pc += 10;
                    break;
                // QuadScan
                case 'Q':
                case 'q':
                    timings->scan_count = 4;
                    pc += 8;
                    break;
                // parse [FrontPorchBegins | SyncBegins | BackPorchBegins | VisibleBegins] [MultiScanBlank]
                // FrontPorchBegins
                case 'F':
                case 'f':
                    timings->h_starting_block = vga_scan_start::front_porch;
                    pc += 16;
                    break;
                // SyncBegins
                case 'S':
                case 's':
                    timings->h_starting_block = vga_scan_start::sync;
                    pc += 10;
                    break;
                // BackPorchBegins
                case 'B':
                case 'b':
                    timings->h_starting_block = vga_scan_start::back_porch;
                    pc += 15;
                    break;
                // VisibleBegins
                case 'V':
                case 'v':
                    timings->h_starting_block = vga_scan_start::visible_area;
                    pc += 13;
                    break;
                // MultiScanBlank
                case 'M':
                case 'm':
                    timings->multi_scan_black = 1;
                    pc += 14;
                    break;
                case ' ':
                    ++pc;
                    break;
                default:
                    return false;
            }
        }

        return true;
    }
    return false;
}


// Suspend vertical sync interrupt
// Warning: After call to suspendBackgroundPrimitiveExecution() adding primitives may cause a deadlock.
// To avoid this a call to "processPrimitives()" should be performed very often.
// Can be nested
void vga_base_controller::suspend_background_primitive_execution()
{
    _primitive_processing_suspended = _primitive_processing_suspended + 1;
}


// Resume vertical sync interrupt after suspendBackgroundPrimitiveExecution()
// Can be nested
void vga_base_controller::resume_background_primitive_execution()
{
    _primitive_processing_suspended = esp::tmax(0, _primitive_processing_suspended - 1);
}


void vga_base_controller::start_gpio_stream()
{
    _gpio_stream.play(_timings.frequency, _dma_buffers);
}


void vga_base_controller::set_resolution(char const *modeline, int view_port_width,
                                         int view_port_height, bool double_buffered)
{
    vga_timings timings;
    if (convert_modeline_to_timings(modeline, &timings))
        set_resolution(timings, view_port_width, view_port_height, double_buffered);
}


void vga_base_controller::set_resolution(vga_timings const &timings, int view_port_width,
                                         int view_port_height, bool double_buffered)
{
    // just in case setResolution() was called before
    end();

    _timings = timings;

    // inform base class about screen size
    set_screen_size(_timings.h_visible_area, _timings.v_visible_area);

    set_double_buffered(double_buffered);

    _hv_sync = pack_hv_sync(false, false);

    _h_line_size = _timings.h_front_porch + _timings.h_sync_pulse + _timings.h_back_porch
        + _timings.h_visible_area;

    _h_blank_line_with_v_sync = (uint8_t *)heap_caps_malloc(_h_line_size, MALLOC_CAP_DMA);
    _h_blank_line = (uint8_t *)heap_caps_malloc(_h_line_size, MALLOC_CAP_DMA);

    _view_port_width = ~3
        & (view_port_width <= 0 || view_port_width >= _timings.h_visible_area
               ? _timings.h_visible_area
               : view_port_width); // view port width must be 32 bit aligned
    _view_port_height = view_port_height <= 0 || view_port_height >= _timings.v_visible_area
        ? _timings.v_visible_area
        : view_port_height;

    // adjust view port size if necessary
    check_view_port_size();

    // need to center viewport?
    _view_port_col = (_timings.h_visible_area - _view_port_width) / 2;
    _view_port_row = (_timings.v_visible_area - _view_port_height) / 2;

    // view port col and row must be 32 bit aligned
    _view_port_col = _view_port_col & ~3;
    _view_port_row = _view_port_row & ~3;

    _raw_frame_height = _timings.v_visible_area + _timings.v_front_porch + _timings.v_sync_pulse
        + _timings.v_back_porch;

    // allocate DMA descriptors
    set_dma_buffers_count(calc_required_dma_buffers_count(_view_port_height));

    // allocate the viewport
    allocate_view_port();

    // adjust again view port size if necessary
    check_view_port_size();

    // this may free space if m_viewPortHeight has been reduced
    set_dma_buffers_count(calc_required_dma_buffers_count(_view_port_height));

    // fill buffers
    fill_vert_buffers(0);
    fill_horiz_buffers(0);

    reset_paint_state();

    if (_double_buffer_over_dma)
        _dma_buffers_head->qe.stqe_next = (lldesc_t *)&_dma_buffers_visible[0];
}


// this method may adjust m_viewPortHeight to the actual number of allocated rows.
// to reduce memory allocation overhead try to allocate the minimum number of blocks.
void vga_base_controller::allocate_view_port(uint32_t alloc_caps, int rowlen)
{
    int lines_count[VIEW_PORT_MEMORY_POOL_COUNT]; // where store number of lines for each pool
    int pools_count = 0; // number of allocated pools
    int remaining_lines = _view_port_height;
    _view_port_height = 0; // m_viewPortHeight needs to be recalculated

    if (is_double_buffered())
        remaining_lines *= 2;

    // allocate pools
    _view_port_memory_pool = (uint8_t **)heap_caps_malloc(
        sizeof(uint8_t *) * (VIEW_PORT_MEMORY_POOL_COUNT + 1), MALLOC_CAP_32BIT);
    while (remaining_lines > 0 && pools_count < VIEW_PORT_MEMORY_POOL_COUNT)
    {
        int largest_block = heap_caps_get_largest_free_block(alloc_caps);
        if (largest_block < MIN_FREE_LARGEST_BLOCK)
            break;
        lines_count[pools_count] = esp::tmax(
            1, esp::tmin(remaining_lines, (largest_block - MIN_FREE_LARGEST_BLOCK) / rowlen));
        _view_port_memory_pool[pools_count] =
            (uint8_t *)heap_caps_malloc(lines_count[pools_count] * rowlen, alloc_caps);
        if (_view_port_memory_pool[pools_count] == nullptr)
            break;
        remaining_lines -= lines_count[pools_count];
        _view_port_height += lines_count[pools_count];
        ++pools_count;
    }
    _view_port_memory_pool[pools_count] = nullptr;

    // fill m_viewPort[] with line pointers
    if (is_double_buffered())
    {
        _view_port_height /= 2;
        _view_port_visible = _view_ports[1] = (volatile uint8_t **)heap_caps_malloc(
            sizeof(uint8_t *) * _view_port_height, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    }
    _view_port = _view_ports[0] = (volatile uint8_t **)heap_caps_malloc(
        sizeof(uint8_t *) * _view_port_height, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
    if (!is_double_buffered())
        _view_port_visible = _view_port;
    for (int p = 0, l = 0; p < pools_count; ++p)
    {
        uint8_t *pool = _view_port_memory_pool[p];
        for (int i = 0; i < lines_count[p]; ++i)
        {
            if (l + i < _view_port_height)
                _view_port[l + i] = pool;
            else
                _view_port_visible[l + i - _view_port_height] =
                    pool; // set only when double buffered is enabled
            pool += rowlen;
        }
        l += lines_count[p];
    }
}


uint8_t IRAM_ATTR vga_base_controller::pack_hv_sync(bool h_sync, bool v_sync)
{
    uint8_t hsync_value = (_timings.h_sync_logic == '+' ? (h_sync ? 1 : 0) : (h_sync ? 0 : 1));
    uint8_t vsync_value = (_timings.v_sync_logic == '+' ? (v_sync ? 1 : 0) : (v_sync ? 0 : 1));
    return (vsync_value << VGA_VSYNC_BIT) | (hsync_value << VGA_HSYNC_BIT);
}


uint8_t IRAM_ATTR vga_base_controller::prepare_pixel_with_sync(esp::gfx::rgb222 rgb, bool h_sync,
                                                               bool v_sync)
{
    return pack_hv_sync(h_sync, v_sync) | (rgb.b << VGA_BLUE_BIT) | (rgb.g << VGA_GREEN_BIT)
        | (rgb.r << VGA_RED_BIT);
}


int vga_base_controller::calc_required_dma_buffers_count(int view_port_height)
{
    int right_pad_size = _timings.h_visible_area - _view_port_width - _view_port_col;
    int buffers_count = _timings.scan_count * (_raw_frame_height + view_port_height);

    switch (_timings.h_starting_block)
    {
        case vga_scan_start::front_porch:
            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
            buffers_count += _timings.scan_count * (right_pad_size > 0 ? view_port_height : 0);
            break;
        case vga_scan_start::sync:
            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
            buffers_count += _timings.scan_count * view_port_height;
            break;
        case vga_scan_start::back_porch:
            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
            buffers_count += _timings.scan_count * view_port_height;
            break;
        case vga_scan_start::visible_area:
            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
            buffers_count += _timings.scan_count * (_view_port_col > 0 ? view_port_height : 0);
            break;
    }

    return buffers_count;
}


// refill buffers changing Front Porch and Back Porch
// offsetX : (< 0 : left  > 0 right)
void vga_base_controller::fill_horiz_buffers(int offset_x)
{
    // fill all with no hsync

    fill(_h_blank_line, 0, _h_line_size, 0, 0, 0, false, false);
    fill(_h_blank_line_with_v_sync, 0, _h_line_size, 0, 0, 0, false, true);

    // calculate hsync pos and fill it

    int16_t porch_sum = _timings.h_front_porch + _timings.h_back_porch;
    _timings.h_front_porch = esp::tmax(8, (int16_t)_timings.h_front_porch - offset_x);
    _timings.h_back_porch = esp::tmax(8, porch_sum - _timings.h_front_porch);
    _timings.h_front_porch = porch_sum - _timings.h_back_porch;

    int sync_pos = 0;

    switch (_timings.h_starting_block)
    {
        case vga_scan_start::front_porch:
            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
            sync_pos = _timings.h_front_porch;
            break;
        case vga_scan_start::sync:
            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
            sync_pos = 0;
            break;
        case vga_scan_start::back_porch:
            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
            sync_pos = _timings.h_back_porch + _timings.h_visible_area + _timings.h_front_porch;
            break;
        case vga_scan_start::visible_area:
            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
            sync_pos = _timings.h_visible_area + _timings.h_front_porch;
            break;
    }

    fill(_h_blank_line, sync_pos, _timings.h_sync_pulse, 0, 0, 0, true, false);
    fill(_h_blank_line_with_v_sync, sync_pos, _timings.h_sync_pulse, 0, 0, 0, true, true);
}


void vga_base_controller::fill_vert_buffers(int offset_y)
{
    int16_t porch_sum = _timings.v_front_porch + _timings.v_back_porch;
    _timings.v_front_porch = esp::tmax(1, (int16_t)_timings.v_front_porch - offset_y);
    _timings.v_back_porch = esp::tmax(1, porch_sum - _timings.v_front_porch);
    _timings.v_front_porch = porch_sum - _timings.v_back_porch;

    // associate buffers pointer to DMA info buffers
    //
    //  Vertical order:
    //    VisibleArea
    //    Front Porch
    //    Sync
    //    Back Porch

    int v_visible_area_pos = 0;
    int v_front_porch_pos = v_visible_area_pos + _timings.v_visible_area;
    int v_sync_pos = v_front_porch_pos + _timings.v_front_porch;
    int v_back_porch_pos = v_sync_pos + _timings.v_sync_pulse;

    int right_pad_size = _timings.h_visible_area - _view_port_width - _view_port_col;

    for (int line = 0, dma_buf_idx = 0; line < _raw_frame_height; ++line)
    {

        bool is_v_visible_area = (line < v_front_porch_pos);
        bool is_v_front_porch = (line >= v_front_porch_pos && line < v_sync_pos);
        bool is_v_sync = (line >= v_sync_pos && line < v_back_porch_pos);
        bool is_v_back_porch = (line >= v_back_porch_pos);

        for (int scan = 0; scan < _timings.scan_count; ++scan)
        {

            bool is_start_of_vert_front_porch = (line == v_front_porch_pos && scan == 0);

            if (is_v_sync)
            {

                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line_with_v_sync, _h_line_size, scan,
                                     is_start_of_vert_front_porch);
            }
            else if (is_v_front_porch || is_v_back_porch)
            {

                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _h_line_size, scan,
                                     is_start_of_vert_front_porch);
            }
            else if (is_v_visible_area)
            {

                int visible_area_line = line - v_visible_area_pos;
                bool is_viewport = visible_area_line >= _view_port_row
                    && visible_area_line < _view_port_row + _view_port_height;
                int h_invisible_area_size = _h_line_size - _timings.h_visible_area;

                if (is_viewport)
                {
                    // visible, this is the viewport
                    switch (_timings.h_starting_block)
                    {
                        case vga_scan_start::front_porch:
                            // FRONTPORCH -> SYNC -> BACKPORCH -> VISIBLEAREA
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 h_invisible_area_size + _view_port_col, scan,
                                                 is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            if (right_pad_size > 0)
                                set_dma_buffer_blank(
                                    dma_buf_idx++, _h_blank_line + h_invisible_area_size,
                                    right_pad_size, scan, is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::sync:
                            // SYNC -> BACKPORCH -> VISIBLEAREA -> FRONTPORCH
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 _timings.h_sync_pulse + _timings.h_back_porch
                                                     + _view_port_col,
                                                 scan, is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(dma_buf_idx++,
                                                 _h_blank_line + _h_line_size
                                                     - _timings.h_front_porch - right_pad_size,
                                                 _timings.h_front_porch + right_pad_size, scan,
                                                 is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::back_porch:
                            // BACKPORCH -> VISIBLEAREA -> FRONTPORCH -> SYNC
                            set_dma_buffer_blank(dma_buf_idx++, _h_blank_line,
                                                 _timings.h_back_porch + _view_port_col, scan,
                                                 is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(
                                dma_buf_idx++,
                                _h_blank_line + _h_line_size - _timings.h_front_porch
                                    - _timings.h_sync_pulse - right_pad_size,
                                _timings.h_front_porch + _timings.h_sync_pulse + right_pad_size,
                                scan, is_start_of_vert_front_porch);
                            break;
                        case vga_scan_start::visible_area:
                            // VISIBLEAREA -> FRONTPORCH -> SYNC -> BACKPORCH
                            if (_view_port_col > 0)
                                set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _view_port_col,
                                                     scan, is_start_of_vert_front_porch);
                            set_dma_buffer_view(dma_buf_idx++, visible_area_line - _view_port_row,
                                                scan, is_start_of_vert_front_porch);
                            set_dma_buffer_blank(dma_buf_idx++,
                                                 _h_blank_line + _timings.h_visible_area
                                                     - right_pad_size,
                                                 h_invisible_area_size + right_pad_size, scan,
                                                 is_start_of_vert_front_porch);
                            break;
                    }
                }
                else
                {
                    // not visible
                    set_dma_buffer_blank(dma_buf_idx++, _h_blank_line, _h_line_size, scan,
                                         is_start_of_vert_front_porch);
                }
            }
        }
    }
}


// address must be allocated with MALLOC_CAP_DMA or even an address of another already allocated buffer
// allocated buffer length (in bytes) must be 32 bit aligned
// Max length is 4092 bytes
void vga_base_controller::set_dma_buffer_blank(int index, void volatile *address, int length,
                                               int scan, bool is_start_of_vert_front_porch)
{
    int size = (length + 3) & (~3);
    _dma_buffers[index].eof = 0;
    _dma_buffers[index].size = size;
    _dma_buffers[index].length = length;
    _dma_buffers[index].buf = (uint8_t *)address;
    on_setup_dma_buffer(&_dma_buffers[index], is_start_of_vert_front_porch, scan, false, 0);
    if (_double_buffer_over_dma && is_double_buffered())
    {
        _dma_buffers_visible[index].eof = 0;
        _dma_buffers_visible[index].size = size;
        _dma_buffers_visible[index].length = length;
        _dma_buffers_visible[index].buf = (uint8_t *)address;
        on_setup_dma_buffer(&_dma_buffers_visible[index], is_start_of_vert_front_porch, scan, false,
                            0);
    }
}


bool vga_base_controller::is_multi_scan_black_line(int scan)
{
    return (scan > 0 && _timings.multi_scan_black == 1 && _timings.h_starting_block == front_porch);
}


// address must be allocated with MALLOC_CAP_DMA or even an address of another already allocated buffer
// allocated buffer length (in bytes) must be 32 bit aligned
// Max length is 4092 bytes
void vga_base_controller::set_dma_buffer_view(int index, int row, int scan,
                                              volatile uint8_t **view_port, bool on_visible_dma)
{
    uint8_t *buffer_ptr = nullptr;
    if (is_multi_scan_black_line(scan))
        buffer_ptr =
            (uint8_t
                 *)(_h_blank_line + _h_line_size
                    - _timings
                          .h_visible_area); // this works only when HSYNC, FrontPorch and BackPorch are at the beginning of m_HBlankLine
    else if (view_port)
        buffer_ptr = (uint8_t *)view_port[row];
    lldesc_t volatile *dma_buffers = on_visible_dma ? _dma_buffers_visible : _dma_buffers;
    dma_buffers[index].size = (_view_port_width + 3) & (~3);
    dma_buffers[index].length = _view_port_width;
    dma_buffers[index].buf = buffer_ptr;
}


// address must be allocated with MALLOC_CAP_DMA or even an address of another already allocated buffer
// allocated buffer length (in bytes) must be 32 bit aligned
// Max length is 4092 bytes
void vga_base_controller::set_dma_buffer_view(int index, int row, int scan,
                                              bool is_start_of_vert_front_porch)
{
    set_dma_buffer_view(index, row, scan, _view_port, false);
    if (!is_multi_scan_black_line(scan))
        on_setup_dma_buffer(&_dma_buffers[index], is_start_of_vert_front_porch, scan, true, row);
    if (is_double_buffered())
    {
        set_dma_buffer_view(index, row, scan, _view_port_visible, true);
        if (!is_multi_scan_black_line(scan))
            on_setup_dma_buffer(&_dma_buffers_visible[index], is_start_of_vert_front_porch, scan,
                                true, row);
    }
}


void volatile *vga_base_controller::get_dma_buffer(int index, int *length)
{
    *length = _dma_buffers[index].length;
    return (void volatile *)_dma_buffers[index].buf;
}


// buffer: buffer to fill (buffer size must be 32 bit aligned)
// startPos: initial position (in pixels)
// length: number of pixels to fill
//
// Returns next pos to fill (startPos + length)
int vga_base_controller::fill(uint8_t volatile *buffer, int start_pos, int length, uint8_t red,
                              uint8_t green, uint8_t blue, bool h_sync, bool v_sync)
{
    uint8_t pattern = prepare_pixel_with_sync((esp::gfx::rgb222){red, green, blue}, h_sync, v_sync);
    for (int i = 0; i < length; ++i, ++start_pos)
        VGA_PIXELINROW(buffer, start_pos) = pattern;
    return start_pos;
}


void vga_base_controller::move_screen(int offset_x, int offset_y)
{
    suspend_background_primitive_execution();
    fill_vert_buffers(offset_y);
    fill_horiz_buffers(offset_x);
    resume_background_primitive_execution();
}


void vga_base_controller::shrink_screen(int shrink_x, int shrink_y)
{
    vga_timings *curr_timings = resolution_timings();

    curr_timings->h_back_porch = esp::tmax(curr_timings->h_back_porch + 4 * shrink_x, 4);
    curr_timings->h_front_porch = esp::tmax(curr_timings->h_front_porch + 4 * shrink_x, 4);

    curr_timings->v_back_porch = esp::tmax(curr_timings->v_back_porch + shrink_y, 1);
    curr_timings->v_front_porch = esp::tmax(curr_timings->v_front_porch + shrink_y, 1);

    set_resolution(*curr_timings, _view_port_width, _view_port_height, is_double_buffered());
}


void IRAM_ATTR vga_base_controller::swap_buffers()
{
    esp::tswap(_view_port, _view_port_visible);
    if (_double_buffer_over_dma)
    {
        esp::tswap(_dma_buffers, _dma_buffers_visible);
        _dma_buffers_head->qe.stqe_next = (lldesc_t *)&_dma_buffers_visible[0];
    }
}


bool vga_base_controller::suspend_double_buffering(bool value)
{
    auto prev_value = esp::gfx::bitmapped_display_controller::suspend_double_buffering(value);
    if (prev_value != value && is_double_buffered())
    {
        if (value)
        {
            _view_ports[0] = _view_port;
            _view_ports[1] = _view_port_visible;
            _view_port = _view_port_visible;
        }
        else
        {
            _view_port = _view_ports[0];
            _view_port_visible = _view_ports[1];
        }
    }
    return prev_value;
}


} // end of namespace
