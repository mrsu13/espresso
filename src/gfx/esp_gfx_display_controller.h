// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::gfx::BitmappedDisplayController definition.
 */


#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_gfx_config.h"
#include "esp_gfx_utils.h"
#include "esp_tools.h"


namespace esp::gfx
{


/*
  Notes:
    - all positions can have negative and outofbound coordinates. Shapes are always clipped correctly.
*/
enum primitive_cmd : uint8_t
{

    // Needed to send the updated area of screen buffer on some displays (ie SSD1306)
    flush,

    // Refresh display. Some displays (ie SSD1306) aren't repainted if there aren't primitives
    // so posting Refresh allows to resend the screenbuffer
    // params: rect (rectangle to refresh)
    refresh,

    // Reset paint state
    // params: none
    reset,

    // Set current pen color
    // params: color
    set_pen_color,

    // Set current brush color
    // params: color
    set_brush_color,

    // Paint a pixel at specified coordinates, using current pen color
    // params: color
    set_pixel,

    // Paint a pixel at specified coordinates using the specified color
    // params: pixelDesc
    set_pixel_at,

    // Move current position to the specified one
    // params: point
    move_to,

    // Draw a line from current position to the specified one, using current pen color. Update current position.
    // params: point
    line_to,

    // Fill a rectangle using current brush color
    // params: rect
    fill_rect,

    // Draw a rectangle using current pen color
    // params: rect
    draw_rect,

    // Fill an ellipse, current position is the center, using current brush color
    // params: size
    fill_ellipse,

    // Draw an ellipse, current position is the center, using current pen color
    // params: size
    draw_ellipse,

    // Fill viewport with brush color
    // params: none
    clear,

    // Scroll vertically without copying buffers
    // params: ivalue (scroll amount, can be negative)
    v_scroll,

    // Scroll horizontally (time consuming operation!)
    // params: ivalue (scroll amount, can be negative)
    h_scroll,

    // Draw a glyph (BW image)
    // params: glyph
    draw_glyph,

    // Set paint options
    // params: glyphOptions
    set_glyph_options,

    // Set gluph options
    // params: paintOptions
    set_paint_options,

    // Invert a rectangle
    // params: rect
    invert_rect,

    // Copy (overlapping) rectangle to current position
    // params: rect (source rectangle)
    copy_rect,

    // Set scrolling region
    // params: rect
    set_scrolling_region,

    // Swap foreground (pen) and background (brush) colors of all pixels inside the specified rectangles. Other colors remain untaltered.
    // params: rect
    swap_fgbg,

    // Draw a bitmap
    // params: bitmapDrawingInfo
    draw_bitmap,

    // Refresh sprites
    // no params
    refresh_sprites,

    // Swap buffers (m_doubleBufferedEnabled must be True)
    // params: notifyTask
    swap_buffers,

    // Fill a path, using current brush color
    // params: path
    fill_path,

    // Draw a path, using current pen color
    // params: path
    draw_path,

    // Set axis origin
    // params: point
    set_origin,

    // Set clipping rectangle
    // params: rect
    set_clipping_rect,

    // Set pen width
    // params: ivalue
    set_pen_width,

    // Set line ends
    // params: lineEnds
    set_line_ends,
};


/** \ingroup Enumerations
 * @brief This enum defines named colors.
 *
 * First eight full implement all available colors when 1 bit per channel mode is used (having 8 colors).
 */
enum color
{
    black, /**< Equivalent to RGB888(0,0,0) */
    red, /**< Equivalent to RGB888(128,0,0) */
    green, /**< Equivalent to RGB888(0,128,0) */
    yellow, /**< Equivalent to RGB888(128,128,0) */
    blue, /**< Equivalent to RGB888(0,0,128) */
    magenta, /**< Equivalent to RGB888(128,0,128) */
    cyan, /**< Equivalent to RGB888(0,128,128) */
    white, /**< Equivalent to RGB888(128,128,128) */
    bright_black, /**< Equivalent to RGB888(64,64,64) */
    bright_red, /**< Equivalent to RGB888(255,0,0) */
    bright_green, /**< Equivalent to RGB888(0,255,0) */
    bright_yellow, /**< Equivalent to RGB888(255,255,0) */
    bright_blue, /**< Equivalent to RGB888(0,0,255) */
    bright_magenta, /**< Equivalent to RGB888(255,0,255) */
    bright_cyan, /**< Equivalent to RGB888(0,255,255) */
    bright_white, /**< Equivalent to RGB888(255,255,255) */
};


/**
 * @brief Represents a 24 bit RGB color.
 *
 * For each channel minimum value is 0, maximum value is 255.
 */
struct rgb888
{
    uint8_t r; /**< The Red channel   */
    uint8_t g; /**< The Green channel */
    uint8_t b; /**< The Blue channel  */

    rgb888()
        : r(0)
        , g(0)
        , b(0)
    {
    }
    rgb888(color color);
    rgb888(uint8_t red, uint8_t green, uint8_t blue)
        : r(red)
        , g(green)
        , b(blue)
    {
    }
} __attribute__((packed));


inline bool operator==(rgb888 const &lhs, rgb888 const &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}


inline bool operator!=(rgb888 const &lhs, rgb888 const &rhs)
{
    return lhs.r != rhs.r || lhs.g != rhs.g || lhs.b == rhs.b;
}


/**
 * @brief Represents a 32 bit RGBA color.
 *
 * For each channel minimum value is 0, maximum value is 255.
 */
struct rgba8888
{
    uint8_t r; /**< The Red channel   */
    uint8_t g; /**< The Green channel */
    uint8_t b; /**< The Blue channel  */
    uint8_t a; /**< The Alpha channel */

    rgba8888()
        : r(0)
        , g(0)
        , b(0)
        , a(0)
    {
    }
    rgba8888(int red, int green, int blue, int alpha)
        : r(red)
        , g(green)
        , b(blue)
        , a(alpha)
    {
    }
};


/**
 * @brief Represents a 6 bit RGB color.
 *
 * When 1 bit per channel (8 colors) is used then the maximum value (white) is 1 (R = 1, G = 1, B = 1).
 * When 2 bits per channel (64 colors) are used then the maximum value (white) is 3 (R = 3, G = 3, B = 3).
 */
struct rgb222
{
    uint8_t r : 2; /**< The Red channel   */
    uint8_t g : 2; /**< The Green channel */
    uint8_t b : 2; /**< The Blue channel  */

    rgb222()
        : r(0)
        , g(0)
        , b(0)
    {
    }
    rgb222(uint8_t red, uint8_t green, uint8_t blue)
        : r(red)
        , g(green)
        , b(blue)
    {
    }
    rgb222(rgb888 const &value);

    static bool low_bit_only; // true= 8 colors, false 64 colors
};


inline bool operator==(rgb222 const &lhs, rgb222 const &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}


inline bool operator!=(rgb222 const &lhs, rgb222 const &rhs)
{
    return lhs.r != rhs.r || lhs.g != rhs.g || lhs.b == rhs.b;
}


/**
 * @brief Represents an 8 bit ABGR color
 *
 * For each channel minimum value is 0, maximum value is 3.
 */
struct rgba2222
{
    uint8_t r : 2; /**< The Red channel (LSB) */
    uint8_t g : 2; /**< The Green channel */
    uint8_t b : 2; /**< The Blue channel  */
    uint8_t a : 2; /**< The Alpha channel (MSB) */

    rgba2222(int red, int green, int blue, int alpha)
        : r(red)
        , g(green)
        , b(blue)
        , a(alpha)
    {
    }
};


//   0 .. 63  => 0
//  64 .. 127 => 1
// 128 .. 191 => 2
// 192 .. 255 => 3
uint8_t rgb888_to_packed_rgb222(rgb888 const &rgb);


/**
 * @brief Represents a glyph position, size and binary data.
 *
 * A glyph is a bitmap (1 bit per pixel). The fabgl::Terminal uses glyphs to render characters.
 */
struct glyph
{
    int16_t x; /**< Horizontal glyph coordinate */
    int16_t y; /**< Vertical glyph coordinate */
    uint8_t width; /**< Glyph horizontal size */
    uint8_t height; /**< Glyph vertical size */
    uint8_t const *
        data; /**< Byte aligned binary data of the glyph. A 0 represents background or a transparent pixel. A 1 represents foreground. */

    glyph()
        : x(0)
        , y(0)
        , width(0)
        , height(0)
        , data(nullptr)
    {
    }
    glyph(int x_, int y_, int width_, int height_, uint8_t const *data_)
        : x(x_)
        , y(y_)
        , width(width_)
        , height(height_)
        , data(data_)
    {
    }
} __attribute__((packed));


/**
 * @brief Specifies various glyph painting options.
 */
union glyph_options
{
    struct
    {
        uint16_t fill_background
            : 1; /**< If enabled glyph background is filled with current background color. */
        uint16_t bold : 1; /**< If enabled produces a bold-like style. */
        uint16_t reduce_luminosity
            : 1; /**< If enabled reduces luminosity. To implement characters faint. */
        uint16_t italic
            : 1; /**< If enabled skews the glyph on the right. To implement characters italic. */
        uint16_t invert
            : 1; /**< If enabled swaps foreground and background colors. To implement characters inverse (XORed with PaintOptions.swapFGBG) */
        uint16_t blank
            : 1; /**< If enabled the glyph is filled with the background color. To implement characters invisible or blink. */
        uint16_t underline
            : 1; /**< If enabled the glyph is underlined. To implement characters underline. */
        uint16_t double_width
            : 2; /**< If enabled the glyph is doubled. To implement characters double width. 0 = normal, 1 = double width, 2 = double width - double height top, 3 = double width - double height bottom. */
        uint16_t user_opt_1 : 1; /**< User defined option */
        uint16_t user_opt_2 : 1; /**< User defined option */
    };
    uint16_t value;

    /** @brief Helper method to set or reset fillBackground */
    glyph_options &set_fill_background(bool value)
    {
        fill_background = value;
        return *this;
    }

    /** @brief Helper method to set or reset bold */
    glyph_options &set_bold(bool value)
    {
        bold = value;
        return *this;
    }

    /** @brief Helper method to set or reset italic */
    glyph_options &set_italic(bool value)
    {
        italic = value;
        return *this;
    }

    /** @brief Helper method to set or reset underlined */
    glyph_options &set_underline(bool value)
    {
        underline = value;
        return *this;
    }

    /** @brief Helper method to set or reset doubleWidth */
    glyph_options &set_double_width(uint8_t value)
    {
        double_width = value;
        return *this;
    }

    /** @brief Helper method to set or reset foreground and background swapping */
    glyph_options &set_invert(uint8_t value)
    {
        invert = value;
        return *this;
    }

    /** @brief Helper method to set or reset foreground and background swapping */
    glyph_options &set_blank(uint8_t value)
    {
        blank = value;
        return *this;
    }
} __attribute__((packed));


/** \ingroup Enumerations
 * @brief This enum defines the display controller native pixel format
 */
enum class native_pixel_format : uint8_t
{
    mono, /**< 1 bit per pixel. 0 = black, 1 = white */
    sbgr2222, /**< 8 bit per pixel: VHBBGGRR (bit 7=VSync 6=HSync 5=B 4=B 3=G 2=G 1=R 0=R). Each color channel can have values from 0 to 3 (maxmum intensity). */
    rgb565be, /**< 16 bit per pixel: RGB565 big endian. */
    palette2, /**< 1 bit palette (2 colors), packed as 8 pixels per byte. */
    palette4, /**< 2 bit palette (4 colors), packed as 4 pixels per byte. */
    palette8, /**< 3 bit palette (8 colors), packed as 8 pixels every 3 bytes. */
    palette16, /**< 4 bit palette (16 colors), packed as two pixels per byte. */
};


/** \ingroup Enumerations
 * @brief This enum defines a pixel format
 */
enum class pixel_format : uint8_t
{
    undefined, /**< Undefined pixel format */
    native, /**< Native device pixel format */
    mask, /**< 1 bit per pixel. 0 = transparent, 1 = opaque (color must be specified apart) */
    rgba2222, /**< 8 bit per pixel: AABBGGRR (bit 7=A 6=A 5=B 4=B 3=G 2=G 1=R 0=R). AA = 0 fully transparent, AA = 3 fully opaque. Each color channel can have values from 0 to 3 (maxmum intensity). */
    rgba8888 /**< 32 bits per pixel: RGBA (R=byte 0, G=byte1, B=byte2, A=byte3). Minimum value for each channel is 0, maximum is 255. */
};


/** \ingroup Enumerations
* @brief This enum defines line ends when pen width is greater than 1
*/
enum class line_ends : uint8_t
{
    none, /**< No line ends */
    circle, /**< Circle line ends */
};


/**
 * @brief Represents an image
 */
struct bitmap
{
    int16_t width; /**< Bitmap horizontal size */
    int16_t height; /**< Bitmap vertical size */
    pixel_format format; /**< Bitmap pixel format */
    rgb888 foreground_color; /**< Foreground color when format is PixelFormat::Mask */
    uint8_t *data; /**< Bitmap binary data */
    bool data_allocated; /**< If true data is released when bitmap is destroyed */

    bitmap()
        : width(0)
        , height(0)
        , format(pixel_format::undefined)
        , foreground_color(rgb888(255, 255, 255))
        , data(nullptr)
        , data_allocated(false)
    {
    }
    bitmap(int width_, int height_, void const *data_, pixel_format format_, bool copy = false);
    bitmap(int width_, int height_, void const *data_, pixel_format format_,
           rgb888 foreground_color_, bool copy = false);
    ~bitmap();

    void set_pixel(int x, int y, int value); // use with PixelFormat::Mask. value can be 0 or not 0
    void set_pixel(int x, int y, rgba2222 value); // use with PixelFormat::RGBA2222
    void set_pixel(int x, int y, rgba8888 value); // use with PixelFormat::RGBA8888

    int alpha(int x, int y);

private:

    void allocate();
    void copy_from(void const *src_data);
};


struct bitmap_drawing_info
{
    int16_t x;
    int16_t y;
    esp::gfx::bitmap const *bitmap;

    bitmap_drawing_info(int x_, int y_, esp::gfx::bitmap const *bitmap_)
        : x(x_)
        , y(y_)
        , bitmap(bitmap_)
    {
    }
} __attribute__((packed));


/** \ingroup Enumerations
 * @brief This enum defines a set of predefined mouse cursors.
 */
enum cursor_name : uint8_t
{
    cursor_pointer_amiga_like, /**< 11x11 Amiga like colored mouse pointer */
    cursor_pointer_simple_reduced, /**< 10x15 mouse pointer */
    cursor_pointer_simple, /**< 11x19 mouse pointer */
    cursor_pointer_shadowed, /**< 11x19 shadowed mouse pointer */
    cursor_pointer, /**< 12x17 mouse pointer */
    cursor_pen, /**< 16x16 pen */
    cursor_cross_1, /**< 9x9 cross */
    cursor_cross_2, /**< 11x11 cross */
    cursor_point, /**< 5x5 point */
    cursor_left_arrow, /**< 11x11 left arrow */
    cursor_right_arrow, /**< 11x11 right arrow */
    cursor_down_arrow, /**< 11x11 down arrow */
    cursor_up_arrow, /**< 11x11 up arrow */
    cursor_move, /**< 19x19 move */
    cursor_resize_1, /**< 12x12 resize orientation 1 */
    cursor_resize_2, /**< 12x12 resize orientation 2 */
    cursor_resize_3, /**< 11x17 resize orientation 3 */
    cursor_resize_4, /**< 17x11 resize orientation 4 */
    cursor_text_input, /**< 7x15 text input */
};


/**
 * @brief Defines a cursor.
 */
struct cursor
{
    int16_t hotspot_x; /**< Cursor horizontal hotspot (0 = left bitmap side) */
    int16_t hotspot_y; /**< Cursor vertical hotspot (0 = upper bitmap side) */
    esp::gfx::bitmap bitmap; /**< Cursor bitmap */
};


/**
 * @brief Represents a sprite.
 *
 * A sprite contains one o more bitmaps (esp::gfx::Bitmap object) and has
 * a position in a scene (fabgl::Scane class). Only one bitmap is displayed at the time.<br>
 * Bitmaps can have different sizes.
 */
struct sprite
{
    volatile int16_t x;
    volatile int16_t y;
    bitmap **frames; // array of pointer to Bitmap
    int16_t frames_count;
    int16_t current_frame;
    int16_t saved_x;
    int16_t saved_y;
    int16_t saved_background_width;
    int16_t saved_background_height;
    uint8_t *saved_background;
    struct
    {
        uint8_t visible : 1;
        // A static sprite should be positioned before dynamic sprites.
        // It is never re-rendered unless allowDraw is 1. Static sprites always sets allowDraw=0 after drawings.
        uint8_t is_static : 1;
        // This is always '1' for dynamic sprites and always '0' for static sprites.
        uint8_t allow_draw : 1;
    };

    sprite();
    ~sprite();
    bitmap *frame()
    {
        return frames ? frames[current_frame] : nullptr;
    }
    int frame_index()
    {
        return current_frame;
    }
    // one store: drawing code running concurrently never sees currentFrame == framesCount (fix by Dean Belfield, vdp-gl)
    void next_frame()
    {
        current_frame = (current_frame < frames_count - 1) ? current_frame + 1 : 0;
    }
    sprite *set_frame(int frame)
    {
        current_frame = frame;
        return this;
    }
    sprite *add_bitmap(bitmap *bitmap);
    sprite *add_bitmap(bitmap *bitmap[], int count);
    void clear_bitmaps();
    int width()
    {
        return frames[current_frame]->width;
    }
    int height()
    {
        return frames[current_frame]->height;
    }
    sprite *move_by(int offset_x, int offset_y);
    sprite *move_by(int offset_x, int offset_y, int wrap_around_width, int wrap_around_height);
    sprite *move_to(int x, int y);
};


struct path
{
    point const *points;
    int points_count;
    bool free_points; // deallocate points after drawing
} __attribute__((packed));


/**
 * @brief Specifies general paint options.
 */
struct paint_options
{
    uint8_t swap_fgbg : 1; /**< If enabled swaps foreground and background colors */
    uint8_t negate
        : 1; /**< If enabled performs NOT logical operator on destination. Implemented only for straight lines and non-filled rectangles. */

    paint_options()
        : swap_fgbg(false)
        , negate(false)
    {
    }
} __attribute__((packed));


struct pixel_desc
{
    point pos;
    rgb888 color;
} __attribute__((packed));


struct primitive
{
    primitive_cmd cmd;
    union
    {
        int16_t ivalue;
        rgb888 color;
        point position;
        esp::gfx::size size;
        esp::gfx::glyph glyph;
        esp::gfx::rect rect;
        esp::gfx::glyph_options glyph_options;
        esp::gfx::paint_options paint_options;
        esp::gfx::bitmap_drawing_info bitmap_drawing_info;
        esp::gfx::path path;
        esp::gfx::pixel_desc pixel_desc;
        esp::gfx::line_ends line_ends;
        TaskHandle_t notify_task;
    } __attribute__((packed));

    primitive() {}
    primitive(primitive_cmd cmd_)
        : cmd(cmd_)
    {
    }
    primitive(primitive_cmd cmd_, esp::gfx::rect const &rect_)
        : cmd(cmd_)
        , rect(rect_)
    {
    }
} __attribute__((packed));


struct paint_state
{
    rgb888 pen_color;
    rgb888 brush_color;
    point position; // value already traslated to "origin"
    esp::gfx::glyph_options glyph_options;
    esp::gfx::paint_options paint_options;
    rect scrolling_region;
    point origin;
    rect clipping_rect; // relative clipping rectangle
    rect
        abs_clipping_rect; // actual absolute clipping rectangle (calculated when setting "origin" or "clippingRect")
    int16_t pen_width;
    esp::gfx::line_ends line_ends;
};


/** \ingroup Enumerations
 * @brief This enum defines types of display controllers
 */
enum class display_controller_type
{
    bitmapped, /**< The display controller can represents text and bitmapped graphics */
};


/**
 * @brief Represents the base abstract class for all display controllers
 */
class base_display_controller
{

public:

    virtual void set_resolution(char const *modeline, int view_port_width = -1,
                                int view_port_height = -1, bool double_buffered = false) = 0;

    virtual void begin() = 0;

    /**
   * @brief Determines the display controller type
   *
   * @return Display controller type.
   */
    virtual display_controller_type controller_type() = 0;

    /**
   * @brief Determines number of colors this display can provide
   *
   * @return Number of colors.
   */
    virtual int colors_count() = 0;

    /**
   * @brief Determines the screen width in pixels.
   *
   * @return Screen width in pixels.
   */
    int screen_width()
    {
        return _screen_width;
    }

    /**
   * @brief Determines the screen height in pixels.
   *
   * @return Screen height in pixels.
   */
    int screen_height()
    {
        return _screen_height;
    }

    /**
   * @brief Determines horizontal size of the viewport.
   *
   * @return Horizontal size of the viewport.
   */
    int view_port_width()
    {
        return _view_port_width;
    }

    /**
   * @brief Determines vertical size of the viewport.
   *
   * @return Vertical size of the viewport.
   */
    int view_port_height()
    {
        return _view_port_height;
    }

protected:

    // inherited classes should call setScreenSize once display size is known
    void set_screen_size(int width, int height)
    {
        _screen_width = width;
        _screen_height = height;
    }

    // we store here these info to avoid to have virtual methods (due the -vtables in flash- problem)
    int16_t _screen_width;
    int16_t _screen_height;
    volatile int16_t _view_port_width;
    volatile int16_t _view_port_height;
};


/**
 * @brief Represents the base abstract class for bitmapped display controllers
 */
class bitmapped_display_controller : public base_display_controller
{

public:

    bitmapped_display_controller();
    virtual ~bitmapped_display_controller();

    display_controller_type controller_type()
    {
        return display_controller_type::bitmapped;
    }

    /**
   * @brief Represents the native pixel format used by this display.
   *
   * @return Display native pixel format
   */
    virtual esp::gfx::native_pixel_format native_pixel_format() = 0;

    esp::gfx::paint_state &paint_state()
    {
        return _paint_state;
    }

    void add_primitive(primitive &primitive);

    void primitives_execution_wait();

    /**
   * @brief Enables or disables drawings inside vertical retracing time.
   *
   * Primitives are processed in background (for example in vertical retracing for VGA or in a separated task for SPI/I2C displays).
   * This method can disable (or reenable) this behavior, making drawing instantaneous. Flickering may occur when
   * drawings are executed out of retracing time.<br>
   * When background executing is disabled the queue is emptied executing all pending primitives.
   * Some displays (SPI/I2C) may be not updated at all when enableBackgroundPrimitiveExecution is False.
   *
   * @param value When true drawings are done on background, when false drawings are executed instantly.
   */
    void enable_background_primitive_execution(bool value);

    /**
   * @brief Enables or disables execution time limitation inside vertical retracing interrupt
   *
   * Disabling interrupt execution timeout may generate flickering but speedup drawing operations.
   *
   * @param value True enables timeout (default), False disables timeout
   */
    void enable_background_primitive_timeout(bool value)
    {
        _background_primitive_timeout_enabled = value;
    }

    bool background_primitive_timeout_enabled()
    {
        return _background_primitive_timeout_enabled;
    }

    /**
   * @brief Suspends drawings.
   *
   * Suspends drawings disabling vertical sync interrupt.<br>
   * After call to suspendBackgroundPrimitiveExecution() adding new primitives may cause a deadlock.<br>
   * To avoid it a call to "processPrimitives()" should be performed very often.<br>
   * This method maintains a counter so can be nested.
   */
    virtual void suspend_background_primitive_execution() = 0;

    /**
   * @brief Resumes drawings after suspendBackgroundPrimitiveExecution().
   *
   * Resumes drawings enabling vertical sync interrupt.
   */
    virtual void resume_background_primitive_execution() = 0;

    /**
   * @brief Draws immediately all primitives in the queue.
   *
   * Draws all primitives before they are processed in the vertical sync interrupt.<br>
   * May generate flickering because don't care of vertical sync.
   */
    void process_primitives();

    /**
   * @brief Sets the list of active sprites.
   *
   * A sprite is an image that keeps background unchanged.<br>
   * There is no limit to the number of active sprites, but flickering and slow
   * refresh happens when a lot of sprites (or large sprites) are visible.<br>
   * To empty the list of active sprites call BitmappedDisplayController.removeSprites().
   *
   * @param sprites The list of sprites to make currently active.
   * @param count Number of sprites in the list.
   *
   * Example:
   *
   *     // define a sprite with user data (velX and velY)
   *     struct MySprite : Sprite {
   *       int  velX;
   *       int  velY;
   *     };
   *
   *     static MySprite sprites[10];
   *
   *     VGAController.setSprites(sprites, 10);
   */
    template<typename T>
    void set_sprites(T *sprites, int count)
    {
        set_sprites(sprites, count, sizeof(T));
    }

    /**
   * @brief Empties the list of active sprites.
   *
   * Call this method when you don't need active sprites anymore.
   */
    void remove_sprites()
    {
        set_sprites(nullptr, 0, 0);
    }

    /**
   * @brief Forces the sprites to be updated.
   *
   * Screen is automatically updated whenever a primitive is painted (look at Canvas).<br>
   * When a sprite updates its image or its position (or any other property) it is required
   * to force a refresh using this method.<br>
   * BitmappedDisplayController.refreshSprites() is required also when using the double buffered mode, to paint sprites.
   */
    void refresh_sprites();

    /**
   * @brief Determines whether BitmappedDisplayController is on double buffered mode.
   *
   * @return True if BitmappedDisplayController is on double buffered mode.
   */
    bool is_double_buffered()
    {
        return _double_buffered_setup;
    }

    /**
   * @brief Determines whether BitmappedDisplayController is on double buffered mode and it is actually enabled.
   *
   * Use BitmappedDisplayController.suspendDoubleBuffering() to suspend or resume double buffering.
   *
   * @return True if BitmappedDisplayController is on double buffered mode and it is actually enabled.
   */
    bool is_double_buffered_enabled()
    {
        return _double_buffered_enabled;
    }

    /**
   * @brief Suspends or resumes double buffering
   *
   * Suspend is only allowed when display controller has been initialized for double buffering.<br>
   * Look at BitmappedDisplayController.isDoubleBufferedEnabled() to check if double buffering is suspended or not.
   *
   * @return Returns previous value (true = suspended, false = double buffering)
   */
    virtual bool suspend_double_buffering(bool value);

    /**
   * @brief Sets mouse cursor and make it visible.
   *
   * @param cursor Cursor to use when mouse pointer need to be painted. nullptr = disable mouse pointer.
   */
    void set_mouse_cursor(cursor *cursor);

    /**
   * @brief Sets mouse cursor from a set of predefined cursors.
   *
   * @param cursorName Name (enum) of predefined cursor.
   *
   * Example:
   *
   *     VGAController.setMouseCursor(CursorName::CursorPointerShadowed);
   */
    void set_mouse_cursor(cursor_name cursor_name);

    /**
   * @brief Sets mouse cursor position.
   *
   * @param X Mouse cursor horizontal position.
   * @param Y Mouse cursor vertical position.
   */
    void set_mouse_cursor_pos(int x, int y);

    virtual void read_screen(rect const &rect, rgb888 *dest_buf) = 0;


    // statics (used for common default properties)

    /**
   * @brief Size of display controller primitives queue
   *
   * Application should change before begin() method.
   *
   * Default value is DISPLAY_CONTROLLER_QUEUE_SIZE (defined in esp_gfx_config.h)
   */
    static int queue_size;


protected:

    //// abstract methods

    virtual void set_pixel_at(pixel_desc const &pixel_desc, esp::gfx::rect &update_rect) = 0;

    virtual void abs_draw_line(int x1, int y1, int x2, int y2, rgb888 color) = 0;

    virtual void raw_fill_row(int y, int x1, int x2, rgb888 color) = 0;

    virtual void draw_ellipse(size const &size, esp::gfx::rect &update_rect) = 0;

    virtual void clear(esp::gfx::rect &update_rect) = 0;

    virtual void v_scroll(int scroll, esp::gfx::rect &update_rect) = 0;

    virtual void h_scroll(int scroll, esp::gfx::rect &update_rect) = 0;

    virtual void draw_glyph(glyph const &glyph, glyph_options glyph_options, rgb888 pen_color,
                            rgb888 brush_color, esp::gfx::rect &update_rect) = 0;

    virtual void invert_rect(rect const &rect, esp::gfx::rect &update_rect) = 0;

    virtual void swap_fgbg(rect const &rect, esp::gfx::rect &update_rect) = 0;

    virtual void copy_rect(rect const &source, esp::gfx::rect &update_rect) = 0;

    virtual void swap_buffers() = 0;

    virtual int bitmap_save_pixel_size() = 0;

    virtual void raw_draw_bitmap_native(int dest_x, int dest_y, bitmap const *bitmap, int x1,
                                        int y1, int x_count, int y_count) = 0;

    virtual void raw_draw_bitmap_mask(int dest_x, int dest_y, bitmap const *bitmap,
                                      void *save_background, int x1, int y1, int x_count,
                                      int y_count) = 0;

    virtual void raw_draw_bitmap_rgba2222(int dest_x, int dest_y, bitmap const *bitmap,
                                          void *save_background, int x1, int y1, int x_count,
                                          int y_count) = 0;

    virtual void raw_draw_bitmap_rgba8888(int dest_x, int dest_y, bitmap const *bitmap,
                                          void *save_background, int x1, int y1, int x_count,
                                          int y_count) = 0;

    //// implemented methods

    void exec_primitive(primitive const &prim, esp::gfx::rect &update_rect, bool inside_isr);

    void update_absolute_clipping_rect();

    rgb888 actual_pen_color();

    rgb888 actual_brush_color();

    void line_to(point const &position, esp::gfx::rect &update_rect);

    void draw_rect(rect const &rect, esp::gfx::rect &update_rect);

    void draw_path(path const &path, esp::gfx::rect &update_rect);

    void abs_draw_thick_line(int x1, int y1, int x2, int y2, int pen_width, rgb888 const &color);

    void fill_rect(rect const &rect, rgb888 const &color, esp::gfx::rect &update_rect);

    void fill_ellipse(int center_x, int center_y, size const &size, rgb888 const &color,
                      esp::gfx::rect &update_rect);

    void fill_path(path const &path, rgb888 const &color, esp::gfx::rect &update_rect);

    void set_sprites(sprite *sprites, int count, int sprite_size);

    sprite *get_sprite(int index);

    int sprites_count()
    {
        return _sprites_count;
    }

    void hide_sprites(esp::gfx::rect &update_rect);

    void show_sprites(esp::gfx::rect &update_rect);

    void draw_bitmap(bitmap_drawing_info const &bitmap_drawing_info, esp::gfx::rect &update_rect);

    void abs_draw_bitmap(int dest_x, int dest_y, bitmap const *bitmap, void *save_background,
                         bool ignore_clipping_rect);

    void set_double_buffered(bool value);

    bool get_primitive(primitive *primitive, int time_out_ms = 0);

    bool get_primitive_isr(primitive *primitive);

    void wait_for_primitives();

    sprite *mouse_cursor()
    {
        return &_mouse_cursor;
    }

    void reset_paint_state();

private:

    void primitive_replace_dynamic_buffers(primitive &primitive);


    esp::gfx::paint_state _paint_state;

    volatile bool _double_buffered_setup; // true = controller initialized for double buffering
    volatile bool _double_buffered_enabled; // true = double buffering actually enabled

    volatile QueueHandle_t _exec_queue; // current primitives queue
    volatile QueueHandle_t _exec_queue_alt; // alternate (in pause) primitive queue

    bool _background_primitive_execution_enabled; // when False primitives are execute immediately
    volatile bool
        _background_primitive_timeout_enabled; // when False VSyncInterrupt() has not timeout

    void *_sprites; // pointer to array of sprite structures
    int _sprite_size; // size of sprite structure
    int _sprites_count; // number of sprites in m_sprites array
    bool _sprites_hidden; // true between hideSprites() and showSprites()

    // mouse cursor (mouse pointer) support
    sprite _mouse_cursor;
    int16_t _mouse_hotspot_x;
    int16_t _mouse_hotspot_y;

    // memory pool used to allocate buffers of primitives
    light_memory_pool _prim_dyn_mem_pool;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenericBitmappedDisplayController


class generic_bitmapped_display_controller : public bitmapped_display_controller
{

protected:


    template<typename TPreparePixel, typename TRawSetPixel>
    void generic_set_pixel_at(pixel_desc const &pixel_desc, esp::gfx::rect &update_rect,
                              TPreparePixel prepare_pixel, TRawSetPixel raw_set_pixel)
    {
        const int x = pixel_desc.pos.x + paint_state().origin.x;
        const int y = pixel_desc.pos.y + paint_state().origin.y;

        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        if (x >= clipX1 && x <= clipX2 && y >= clipY1 && y <= clipY2)
        {
            update_rect = update_rect.merge(rect(x, y, x, y));
            hide_sprites(update_rect);
            raw_set_pixel(x, y, prepare_pixel(pixel_desc.color));
        }
    }


    // coordinates are absolute values (not relative to origin)
    // line clipped on current absolute clipping rectangle
    template<typename TPreparePixel, typename TRawFillRow, typename TRawInvertRow,
             typename TRawSetPixel, typename TRawInvertPixel>
    void generic_abs_draw_line(int x1, int y1, int x2, int y2, rgb888 const &color,
                               TPreparePixel prepare_pixel, TRawFillRow raw_fill_row,
                               TRawInvertRow raw_invert_row, TRawSetPixel raw_set_pixel,
                               TRawInvertPixel raw_invert_pixel)
    {
        if (paint_state().pen_width > 1)
        {
            abs_draw_thick_line(x1, y1, x2, y2, paint_state().pen_width, color);
            return;
        }
        auto pattern = prepare_pixel(color);
        if (y1 == y2)
        {
            // horizontal line
            if (y1 < paint_state().abs_clipping_rect.y1 || y1 > paint_state().abs_clipping_rect.y2)
                return;
            if (x1 > x2)
                esp::tswap(x1, x2);
            if (x1 > paint_state().abs_clipping_rect.x2 || x2 < paint_state().abs_clipping_rect.x1)
                return;
            x1 = esp::iclamp(x1, paint_state().abs_clipping_rect.x1,
                             paint_state().abs_clipping_rect.x2);
            x2 = esp::iclamp(x2, paint_state().abs_clipping_rect.x1,
                             paint_state().abs_clipping_rect.x2);
            if (paint_state().paint_options.negate)
                raw_invert_row(y1, x1, x2);
            else
                raw_fill_row(y1, x1, x2, pattern);
        }
        else if (x1 == x2)
        {
            // vertical line
            if (x1 < paint_state().abs_clipping_rect.x1 || x1 > paint_state().abs_clipping_rect.x2)
                return;
            if (y1 > y2)
                esp::tswap(y1, y2);
            if (y1 > paint_state().abs_clipping_rect.y2 || y2 < paint_state().abs_clipping_rect.y1)
                return;
            y1 = esp::iclamp(y1, paint_state().abs_clipping_rect.y1,
                             paint_state().abs_clipping_rect.y2);
            y2 = esp::iclamp(y2, paint_state().abs_clipping_rect.y1,
                             paint_state().abs_clipping_rect.y2);
            if (paint_state().paint_options.negate)
            {
                for (int y = y1; y <= y2; ++y)
                    raw_invert_pixel(x1, y);
            }
            else
            {
                for (int y = y1; y <= y2; ++y)
                    raw_set_pixel(x1, y, pattern);
            }
        }
        else
        {
            // other cases (Bresenham's algorithm)
            // TODO: to optimize
            //   Unfortunately here we cannot clip exactly using Sutherland-Cohen algorithm (as done before)
            //   because the starting line (got from clipping algorithm) may not be the same of Bresenham's
            //   line (think to continuing an existing line).
            //   Possible solutions:
            //      - "Yevgeny P. Kuzmin" algorithm:
            //               https://stackoverflow.com/questions/40884680/how-to-use-bresenhams-line-drawing-algorithm-with-clipping
            //               https://github.com/ktfh/ClippedLine/blob/master/clip.hpp
            // For now Sutherland-Cohen algorithm is only used to check the line is actually visible,
            // then test for every point inside the main Bresenham's loop.
            if (!clip_line(x1, y1, x2, y2, paint_state().abs_clipping_rect,
                           true)) // true = do not change line coordinates!
                return;
            const int dx = abs(x2 - x1);
            const int dy = abs(y2 - y1);
            const int sx = x1 < x2 ? 1 : -1;
            const int sy = y1 < y2 ? 1 : -1;
            int err = (dx > dy ? dx : -dy) / 2;
            while (true)
            {
                if (paint_state().abs_clipping_rect.contains(x1, y1))
                {
                    if (paint_state().paint_options.negate)
                        raw_invert_pixel(x1, y1);
                    else
                        raw_set_pixel(x1, y1, pattern);
                }
                if (x1 == x2 && y1 == y2)
                    break;
                int e2 = err;
                if (e2 > -dx)
                {
                    err -= dy;
                    x1 += sx;
                }
                if (e2 < dy)
                {
                    err += dx;
                    y1 += sy;
                }
            }
        }
    }


    // McIlroy's algorithm
    template<typename TPreparePixel, typename TRawSetPixel>
    void generic_draw_ellipse(size const &size, esp::gfx::rect &update_rect,
                              TPreparePixel prepare_pixel, TRawSetPixel raw_set_pixel)
    {
        auto pattern = prepare_pixel(actual_pen_color());

        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        const int center_x = paint_state().position.x;
        const int center_y = paint_state().position.y;

        const int halfWidth = size.width / 2;
        const int halfHeight = size.height / 2;

        update_rect = update_rect.merge(rect(center_x - halfWidth, center_y - halfHeight,
                                             center_x + halfWidth, center_y + halfHeight));
        hide_sprites(update_rect);

        const int a2 = halfWidth * halfWidth;
        const int b2 = halfHeight * halfHeight;
        const int crit1 = -(a2 / 4 + halfWidth % 2 + b2);
        const int crit2 = -(b2 / 4 + halfHeight % 2 + a2);
        const int crit3 = -(b2 / 4 + halfHeight % 2);
        const int d2xt = 2 * b2;
        const int d2yt = 2 * a2;
        int x = 0; // travels from 0 up to halfWidth
        int y = halfHeight; // travels from halfHeight down to 0
        int t = -a2 * y;
        int dxt = 2 * b2 * x;
        int dyt = -2 * a2 * y;

        while (y >= 0 && x <= halfWidth)
        {
            const int col1 = center_x - x;
            const int col2 = center_x + x;
            const int row1 = center_y - y;
            const int row2 = center_y + y;

            if (col1 >= clipX1 && col1 <= clipX2)
            {
                if (row1 >= clipY1 && row1 <= clipY2)
                    raw_set_pixel(col1, row1, pattern);
                if (row2 >= clipY1 && row2 <= clipY2)
                    raw_set_pixel(col1, row2, pattern);
            }
            if (col2 >= clipX1 && col2 <= clipX2)
            {
                if (row1 >= clipY1 && row1 <= clipY2)
                    raw_set_pixel(col2, row1, pattern);
                if (row2 >= clipY1 && row2 <= clipY2)
                    raw_set_pixel(col2, row2, pattern);
            }

            if (t + b2 * x <= crit1 || t + a2 * y <= crit3)
            {
                x++;
                dxt += d2xt;
                t += dxt;
            }
            else if (t - a2 * y > crit2)
            {
                y--;
                dyt += d2yt;
                t += dyt;
            }
            else
            {
                x++;
                dxt += d2xt;
                t += dxt;
                y--;
                dyt += d2yt;
                t += dyt;
            }
        }
    }


    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    void generic_draw_glyph(glyph const &glyph, glyph_options glyph_options, rgb888 pen_color,
                            rgb888 brush_color, esp::gfx::rect &update_rect,
                            TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                            TRawSetPixelInRow raw_set_pixel_in_row)
    {
        if (!glyph_options.bold && !glyph_options.italic && !glyph_options.blank
            && !glyph_options.underline && !glyph_options.double_width && glyph.width <= 32)
            generic_draw_glyph_light(glyph, glyph_options, pen_color, brush_color, update_rect,
                                     prepare_pixel, raw_get_row, raw_set_pixel_in_row);
        else
            generic_draw_glyph_full(glyph, glyph_options, pen_color, brush_color, update_rect,
                                    prepare_pixel, raw_get_row, raw_set_pixel_in_row);
    }


    // TODO: Italic doesn't work well when clipping rect is specified
    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    void generic_draw_glyph_full(glyph const &glyph, glyph_options glyph_options, rgb888 pen_color,
                                 rgb888 brush_color, esp::gfx::rect &update_rect,
                                 TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                                 TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        const int orig_x = paint_state().origin.x;
        const int orig_y = paint_state().origin.y;

        const int glyphX = glyph.x + orig_x;
        const int glyphY = glyph.y + orig_y;

        if (glyphX > clipX2 || glyphY > clipY2)
            return;

        int16_t glyph_width = glyph.width;
        int16_t glyph_height = glyph.height;
        uint8_t const *glyph_data = glyph.data;
        int16_t glyph_width_byte = (glyph_width + 7) / 8;
        int16_t glyph_size = glyph_height * glyph_width_byte;

        bool fill_background = glyph_options.fill_background;
        bool bold = glyph_options.bold;
        bool italic = glyph_options.italic;
        bool blank = glyph_options.blank;
        bool underline = glyph_options.underline;
        int double_width = glyph_options.double_width;

        // modify glyph to handle top half and bottom half double height
        // doubleWidth = 1 is handled directly inside drawing routine
        if (double_width > 1)
        {
            uint8_t *new_glyph_data = (uint8_t *)alloca(glyph_size);
            // doubling top-half or doubling bottom-half?
            int offset = (double_width == 2 ? 0 : (glyph_height >> 1));
            for (int y = 0; y < glyph_height; ++y)
                for (int x = 0; x < glyph_width_byte; ++x)
                    new_glyph_data[x + y * glyph_width_byte] =
                        glyph_data[x + (offset + (y >> 1)) * glyph_width_byte];
            glyph_data = new_glyph_data;
        }

        // a very simple and ugly skew (italic) implementation!
        int skew_adder = 0, skew_h1 = 0, skew_h2 = 0;
        if (italic)
        {
            skew_adder = 2;
            skew_h1 = glyph_height / 3;
            skew_h2 = skew_h1 * 2;
        }

        int16_t x1 = 0;
        int16_t x_count = glyph_width;
        int16_t dest_x = glyphX;

        if (dest_x < clipX1)
        {
            x1 = (clipX1 - dest_x) / (double_width ? 2 : 1);
            dest_x = clipX1;
        }
        if (x1 >= glyph_width)
            return;

        if (dest_x + x_count + skew_adder > clipX2 + 1)
            x_count = clipX2 + 1 - dest_x - skew_adder;
        if (x1 + x_count > glyph_width)
            x_count = glyph_width - x1;

        int16_t y1 = 0;
        int16_t y_count = glyph_height;
        int dest_y = glyphY;

        if (dest_y < clipY1)
        {
            y1 = clipY1 - dest_y;
            dest_y = clipY1;
        }
        if (y1 >= glyph_height)
            return;

        if (dest_y + y_count > clipY2 + 1)
            y_count = clipY2 + 1 - dest_y;
        if (y1 + y_count > glyph_height)
            y_count = glyph_height - y1;

        update_rect = update_rect.merge(
            rect(dest_x, dest_y, dest_x + x_count + skew_adder - 1, dest_y + y_count - 1));
        hide_sprites(update_rect);

        if (glyph_options.invert ^ paint_state().paint_options.swap_fgbg)
            esp::tswap(pen_color, brush_color);

        // a very simple and ugly reduce luminosity (faint) implementation!
        if (glyph_options.reduce_luminosity)
        {
            if (pen_color.r > 128)
                pen_color.r = 128;
            if (pen_color.g > 128)
                pen_color.g = 128;
            if (pen_color.b > 128)
                pen_color.b = 128;
        }

        auto pen_pattern = prepare_pixel(pen_color);
        auto brush_pattern = prepare_pixel(brush_color);
        auto bold_pattern = bold
            ? prepare_pixel(rgb888(pen_color.r / 2 + 1, pen_color.g / 2 + 1, pen_color.b / 2 + 1))
            : prepare_pixel(rgb888(0, 0, 0));

        for (int y = y1; y < y1 + y_count; ++y, ++dest_y)
        {

            // true if previous pixel has been set
            bool prev_set = false;

            auto dstrow = raw_get_row(dest_y);
            auto srcrow = glyph_data + y * glyph_width_byte;

            if (underline && y == glyph_height - UNDERLINE_POSITION - 1)
            {

                for (int x = x1, adest_x = dest_x + skew_adder;
                     x < x1 + x_count && adest_x <= clipX2; ++x, ++adest_x)
                {
                    raw_set_pixel_in_row(dstrow, adest_x, blank ? brush_pattern : pen_pattern);
                    if (double_width)
                    {
                        ++adest_x;
                        if (adest_x > clipX2)
                            break;
                        raw_set_pixel_in_row(dstrow, adest_x, blank ? brush_pattern : pen_pattern);
                    }
                }
            }
            else
            {

                for (int x = x1, adest_x = dest_x + skew_adder;
                     x < x1 + x_count && adest_x <= clipX2; ++x, ++adest_x)
                {
                    if ((srcrow[x >> 3] << (x & 7)) & 0x80 && !blank)
                    {
                        raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
                        prev_set = true;
                    }
                    else if (bold && prev_set)
                    {
                        raw_set_pixel_in_row(dstrow, adest_x, bold_pattern);
                        prev_set = false;
                    }
                    else if (fill_background)
                    {
                        raw_set_pixel_in_row(dstrow, adest_x, brush_pattern);
                        prev_set = false;
                    }
                    else
                    {
                        prev_set = false;
                    }
                    if (double_width)
                    {
                        ++adest_x;
                        if (adest_x > clipX2)
                            break;
                        if (fill_background)
                            raw_set_pixel_in_row(dstrow, adest_x,
                                                 prev_set ? pen_pattern : brush_pattern);
                        else if (prev_set)
                            raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
                    }
                }
            }

            if (italic && (y == skew_h1 || y == skew_h2))
                --skew_adder;
        }
    }


    // assume:
    //   glyph.width <= 32
    //   glyphOptions.fillBackground = 0 or 1
    //   glyphOptions.invert : 0 or 1
    //   glyphOptions.reduceLuminosity: 0 or 1
    //   glyphOptions.... others = 0
    //   paintState().paintOptions.swapFGBG: 0 or 1
    template<typename TPreparePixel, typename TRawGetRow, typename TRawSetPixelInRow>
    void generic_draw_glyph_light(glyph const &glyph, glyph_options glyph_options, rgb888 pen_color,
                                  rgb888 brush_color, esp::gfx::rect &update_rect,
                                  TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                                  TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        const int orig_x = paint_state().origin.x;
        const int orig_y = paint_state().origin.y;

        const int glyphX = glyph.x + orig_x;
        const int glyphY = glyph.y + orig_y;

        if (glyphX > clipX2 || glyphY > clipY2)
            return;

        int16_t glyph_width = glyph.width;
        int16_t glyph_height = glyph.height;
        uint8_t const *glyph_data = glyph.data;
        int16_t glyph_width_byte = (glyph_width + 7) / 8;

        int16_t x1 = 0;
        int16_t x_count = glyph_width;
        int16_t dest_x = glyphX;

        int16_t y1 = 0;
        int16_t y_count = glyph_height;
        int dest_y = glyphY;

        if (dest_x < clipX1)
        {
            x1 = clipX1 - dest_x;
            dest_x = clipX1;
        }
        if (x1 >= glyph_width)
            return;

        if (dest_x + x_count > clipX2 + 1)
            x_count = clipX2 + 1 - dest_x;
        if (x1 + x_count > glyph_width)
            x_count = glyph_width - x1;

        if (dest_y < clipY1)
        {
            y1 = clipY1 - dest_y;
            dest_y = clipY1;
        }
        if (y1 >= glyph_height)
            return;

        if (dest_y + y_count > clipY2 + 1)
            y_count = clipY2 + 1 - dest_y;
        if (y1 + y_count > glyph_height)
            y_count = glyph_height - y1;

        update_rect =
            update_rect.merge(rect(dest_x, dest_y, dest_x + x_count - 1, dest_y + y_count - 1));
        hide_sprites(update_rect);

        if (glyph_options.invert ^ paint_state().paint_options.swap_fgbg)
            esp::tswap(pen_color, brush_color);

        // a very simple and ugly reduce luminosity (faint) implementation!
        if (glyph_options.reduce_luminosity)
        {
            if (pen_color.r > 128)
                pen_color.r = 128;
            if (pen_color.g > 128)
                pen_color.g = 128;
            if (pen_color.b > 128)
                pen_color.b = 128;
        }

        bool fill_background = glyph_options.fill_background;

        auto pen_pattern = prepare_pixel(pen_color);
        auto brush_pattern = prepare_pixel(brush_color);

        for (int y = y1; y < y1 + y_count; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            uint8_t const *srcrow = glyph_data + y * glyph_width_byte;

            uint32_t src = (srcrow[0] << 24) | (srcrow[1] << 16) | (srcrow[2] << 8) | (srcrow[3]);
            src <<= x1;
            if (fill_background)
            {
                // filled background
                for (int x = x1, adest_x = dest_x; x < x1 + x_count; ++x, ++adest_x, src <<= 1)
                    raw_set_pixel_in_row(dstrow, adest_x,
                                         src & 0x80000000 ? pen_pattern : brush_pattern);
            }
            else
            {
                // transparent background
                for (int x = x1, adest_x = dest_x; x < x1 + x_count; ++x, ++adest_x, src <<= 1)
                    if (src & 0x80000000)
                        raw_set_pixel_in_row(dstrow, adest_x, pen_pattern);
            }
        }
    }


    template<typename TRawInvertRow>
    void generic_invert_rect(rect const &rect, esp::gfx::rect &update_rect,
                             TRawInvertRow raw_invert_row)
    {
        const int orig_x = paint_state().origin.x;
        const int orig_y = paint_state().origin.y;

        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        const int x1 = esp::iclamp(rect.x1 + orig_x, clipX1, clipX2);
        const int y1 = esp::iclamp(rect.y1 + orig_y, clipY1, clipY2);
        const int x2 = esp::iclamp(rect.x2 + orig_x, clipX1, clipX2);
        const int y2 = esp::iclamp(rect.y2 + orig_y, clipY1, clipY2);

        update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
        hide_sprites(update_rect);

        for (int y = y1; y <= y2; ++y)
            raw_invert_row(y, x1, x2);
    }


    template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
             typename TRawSetPixelInRow>
    void generic_swap_fgbg(rect const &rect, esp::gfx::rect &update_rect,
                           TPreparePixel prepare_pixel, TRawGetRow raw_get_row,
                           TRawGetPixelInRow raw_get_pixel_in_row,
                           TRawSetPixelInRow raw_set_pixel_in_row)
    {
        auto pen_pattern = prepare_pixel(paint_state().pen_color);
        auto brush_pattern = prepare_pixel(paint_state().brush_color);

        int orig_x = paint_state().origin.x;
        int orig_y = paint_state().origin.y;

        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        const int x1 = esp::iclamp(rect.x1 + orig_x, clipX1, clipX2);
        const int y1 = esp::iclamp(rect.y1 + orig_y, clipY1, clipY2);
        const int x2 = esp::iclamp(rect.x2 + orig_x, clipX1, clipX2);
        const int y2 = esp::iclamp(rect.y2 + orig_y, clipY1, clipY2);

        update_rect = update_rect.merge(esp::gfx::rect(x1, y1, x2, y2));
        hide_sprites(update_rect);

        for (int y = y1; y <= y2; ++y)
        {
            auto row = raw_get_row(y);
            for (int x = x1; x <= x2; ++x)
            {
                auto px = raw_get_pixel_in_row(row, x);
                if (px == pen_pattern)
                    raw_set_pixel_in_row(row, x, brush_pattern);
                else if (px == brush_pattern)
                    raw_set_pixel_in_row(row, x, pen_pattern);
            }
        }
    }


    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow>
    void generic_copy_rect(rect const &source, esp::gfx::rect &update_rect, TRawGetRow raw_get_row,
                           TRawGetPixelInRow raw_get_pixel_in_row,
                           TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int clipX1 = paint_state().abs_clipping_rect.x1;
        const int clipY1 = paint_state().abs_clipping_rect.y1;
        const int clipX2 = paint_state().abs_clipping_rect.x2;
        const int clipY2 = paint_state().abs_clipping_rect.y2;

        int orig_x = paint_state().origin.x;
        int orig_y = paint_state().origin.y;

        int src_x = source.x1 + orig_x;
        int src_y = source.y1 + orig_y;
        int width = source.x2 - source.x1 + 1;
        int height = source.y2 - source.y1 + 1;
        int dest_x = paint_state().position.x;
        int dest_y = paint_state().position.y;
        int delta_x = dest_x - src_x;
        int delta_y = dest_y - src_y;

        int inc_x = delta_x < 0 ? 1 : -1;
        int inc_y = delta_y < 0 ? 1 : -1;

        int start_x = delta_x < 0 ? dest_x : dest_x + width - 1;
        int start_y = delta_y < 0 ? dest_y : dest_y + height - 1;

        update_rect = update_rect.merge(rect(src_x, src_y, src_x + width - 1, src_y + height - 1));
        update_rect =
            update_rect.merge(rect(dest_x, dest_y, dest_x + width - 1, dest_y + height - 1));
        hide_sprites(update_rect);

        for (int y = start_y, i = 0; i < height; y += inc_y, ++i)
        {
            if (y >= clipY1 && y <= clipY2)
            {
                auto src_row = raw_get_row(y - delta_y);
                auto dst_row = raw_get_row(y);
                for (int x = start_x, j = 0; j < width; x += inc_x, ++j)
                {
                    if (x >= clipX1 && x <= clipX2)
                        raw_set_pixel_in_row(dst_row, x,
                                             raw_get_pixel_in_row(src_row, x - delta_x));
                }
            }
        }
    }


    template<typename TRawGetRow, typename TRawSetPixelInRow, typename TDataType>
    void generic_raw_draw_bitmap_native(int dest_x, int dest_y, TDataType *data, int width, int x1,
                                        int y1, int x_count, int y_count, TRawGetRow raw_get_row,
                                        TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int yEnd = y1 + y_count;
        const int xEnd = x1 + x_count;
        for (int y = y1; y < yEnd; ++y, ++dest_y)
        {
            auto dstrow = raw_get_row(dest_y);
            auto src = data + y * width + x1;
            for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++src)
                raw_set_pixel_in_row(dstrow, adest_x, *src);
        }
    }


    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    void generic_raw_draw_bitmap_mask(int dest_x, int dest_y, bitmap const *bitmap,
                                      TBackground *save_background, int x1, int y1, int x_count,
                                      int y_count, TRawGetRow raw_get_row,
                                      TRawGetPixelInRow raw_get_pixel_in_row,
                                      TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int width = bitmap->width;
        const int yEnd = y1 + y_count;
        const int xEnd = x1 + x_count;
        auto data = bitmap->data;
        const int rowlen = (bitmap->width + 7) / 8;

        if (save_background)
        {

            // save background and draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto save_px = save_background + y * width + x1;
                auto src = data + y * rowlen;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++save_px)
                {
                    *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                    if ((src[x >> 3] << (x & 7)) & 0x80)
                        raw_set_pixel_in_row(dstrow, adest_x);
                }
            }
        }
        else
        {

            // just draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto src = data + y * rowlen;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x)
                {
                    if ((src[x >> 3] << (x & 7)) & 0x80)
                        raw_set_pixel_in_row(dstrow, adest_x);
                }
            }
        }
    }


    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    void generic_raw_draw_bitmap_rgba2222(int dest_x, int dest_y, bitmap const *bitmap,
                                          TBackground *save_background, int x1, int y1, int x_count,
                                          int y_count, TRawGetRow raw_get_row,
                                          TRawGetPixelInRow raw_get_pixel_in_row,
                                          TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int width = bitmap->width;
        const int yEnd = y1 + y_count;
        const int xEnd = x1 + x_count;
        auto data = bitmap->data;

        if (save_background)
        {

            // save background and draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto save_px = save_background + y * width + x1;
                auto src = data + y * width + x1;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++save_px, ++src)
                {
                    *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                    if (*src & 0xc0) // alpha > 0 ?
                        raw_set_pixel_in_row(dstrow, adest_x, *src);
                }
            }
        }
        else
        {

            // just draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto src = data + y * width + x1;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++src)
                {
                    if (*src & 0xc0) // alpha > 0 ?
                        raw_set_pixel_in_row(dstrow, adest_x, *src);
                }
            }
        }
    }


    template<typename TRawGetRow, typename TRawGetPixelInRow, typename TRawSetPixelInRow,
             typename TBackground>
    void generic_raw_draw_bitmap_rgba8888(int dest_x, int dest_y, bitmap const *bitmap,
                                          TBackground *save_background, int x1, int y1, int x_count,
                                          int y_count, TRawGetRow raw_get_row,
                                          TRawGetPixelInRow raw_get_pixel_in_row,
                                          TRawSetPixelInRow raw_set_pixel_in_row)
    {
        const int width = bitmap->width;
        const int yEnd = y1 + y_count;
        const int xEnd = x1 + x_count;
        auto data = (rgba8888 const *)bitmap->data;

        if (save_background)
        {

            // save background and draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto save_px = save_background + y * width + x1;
                auto src = data + y * width + x1;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++save_px, ++src)
                {
                    *save_px = raw_get_pixel_in_row(dstrow, adest_x);
                    if (src->a)
                        raw_set_pixel_in_row(dstrow, adest_x, *src);
                }
            }
        }
        else
        {

            // just draw the bitmap
            for (int y = y1; y < yEnd; ++y, ++dest_y)
            {
                auto dstrow = raw_get_row(dest_y);
                auto src = data + y * width + x1;
                for (int x = x1, adest_x = dest_x; x < xEnd; ++x, ++adest_x, ++src)
                {
                    if (src->a)
                        raw_set_pixel_in_row(dstrow, adest_x, *src);
                }
            }
        }
    }


    // Scroll is done copying and filling rows
    // scroll < 0 -> scroll UP
    // scroll > 0 -> scroll DOWN
    template<typename TRawCopyRow, typename TRawFillRow>
    void generic_v_scroll(int scroll, esp::gfx::rect &update_rect, TRawCopyRow raw_copy_row,
                          TRawFillRow raw_fill_row)
    {
        hide_sprites(update_rect);
        rgb888 color = actual_brush_color();
        int y1 = paint_state().scrolling_region.y1;
        int y2 = paint_state().scrolling_region.y2;
        int x1 = paint_state().scrolling_region.x1;
        int x2 = paint_state().scrolling_region.x2;
        int height = y2 - y1 + 1;

        if (scroll < 0)
        {

            // scroll UP

            for (int i = 0; i < height + scroll; ++i)
            {
                // copy X1..X2 of (Y1 + i - scroll) to (Y1 + i)
                raw_copy_row(x1, x2, (y1 + i - scroll), (y1 + i));
            }
            // fill lower area with brush color
            for (int i = height + scroll; i < height; ++i)
                raw_fill_row(y1 + i, x1, x2, color);
        }
        else if (scroll > 0)
        {

            // scroll DOWN
            for (int i = height - scroll - 1; i >= 0; --i)
            {
                // copy X1..X2 of (Y1 + i) to (Y1 + i + scroll)
                raw_copy_row(x1, x2, (y1 + i), (y1 + i + scroll));
            }

            // fill upper area with brush color
            for (int i = 0; i < scroll; ++i)
                raw_fill_row(y1 + i, x1, x2, color);
        }
    }


    // scroll is done swapping rows and rows pointers
    // scroll < 0 -> scroll UP
    // scroll > 0 -> scroll DOWN
    template<typename TSwapRowsCopying, typename TSwapRowsPointers, typename TRawFillRow>
    void generic_v_scroll(int scroll, esp::gfx::rect &update_rect,
                          TSwapRowsCopying swap_rows_copying, TSwapRowsPointers swap_rows_pointers,
                          TRawFillRow raw_fill_row)
    {
        hide_sprites(update_rect);
        rgb888 color = actual_brush_color();
        const int y1 = paint_state().scrolling_region.y1;
        const int y2 = paint_state().scrolling_region.y2;
        const int x1 = paint_state().scrolling_region.x1;
        const int x2 = paint_state().scrolling_region.x2;
        const int height = y2 - y1 + 1;

        const int view_port_width = this->view_port_width();

        if (scroll < 0)
        {

            // scroll UP

            for (int i = 0; i < height + scroll; ++i)
            {

                // these are necessary to maintain invariate out of scrolling regions
                if (x1 > 0)
                    swap_rows_copying(y1 + i, y1 + i - scroll, 0, x1 - 1);
                if (x2 < view_port_width - 1)
                    swap_rows_copying(y1 + i, y1 + i - scroll, x2 + 1, view_port_width - 1);

                // swap scan lines
                swap_rows_pointers(y1 + i, y1 + i - scroll);
            }

            // fill lower area with brush color
            for (int i = height + scroll; i < height; ++i)
                raw_fill_row(y1 + i, x1, x2, color);
        }
        else if (scroll > 0)
        {

            // scroll DOWN
            for (int i = height - scroll - 1; i >= 0; --i)
            {

                // these are necessary to maintain invariate out of scrolling regions
                if (x1 > 0)
                    swap_rows_copying(y1 + i, y1 + i + scroll, 0, x1 - 1);
                if (x2 < view_port_width - 1)
                    swap_rows_copying(y1 + i, y1 + i + scroll, x2 + 1, view_port_width - 1);

                // swap scan lines
                swap_rows_pointers(y1 + i, y1 + i + scroll);
            }

            // fill upper area with brush color
            for (int i = 0; i < scroll; ++i)
                raw_fill_row(y1 + i, x1, x2, color);
        }
    }


    // Scroll is done copying and filling columns
    // scroll < 0 -> scroll LEFT
    // scroll > 0 -> scroll RIGHT
    template<typename TPreparePixel, typename TRawGetRow, typename TRawGetPixelInRow,
             typename TRawSetPixelInRow>
    void generic_h_scroll(int scroll, esp::gfx::rect &update_rect, TPreparePixel prepare_pixel,
                          TRawGetRow raw_get_row, TRawGetPixelInRow raw_get_pixel_in_row,
                          TRawSetPixelInRow raw_set_pixel_in_row)
    {
        hide_sprites(update_rect);
        auto pattern = prepare_pixel(actual_brush_color());

        int y1 = paint_state().scrolling_region.y1;
        int y2 = paint_state().scrolling_region.y2;
        int x1 = paint_state().scrolling_region.x1;
        int x2 = paint_state().scrolling_region.x2;

        if (scroll < 0)
        {
            // scroll left
            for (int y = y1; y <= y2; ++y)
            {
                auto row = raw_get_row(y);
                for (int x = x1; x <= x2 + scroll; ++x)
                {
                    auto c = raw_get_pixel_in_row(row, x - scroll);
                    raw_set_pixel_in_row(row, x, c);
                }
                // fill right area with brush color
                for (int x = x2 + 1 + scroll; x <= x2; ++x)
                    raw_set_pixel_in_row(row, x, pattern);
            }
        }
        else if (scroll > 0)
        {
            // scroll right
            for (int y = y1; y <= y2; ++y)
            {
                auto row = raw_get_row(y);
                for (int x = x2 - scroll; x >= x1; --x)
                {
                    auto c = raw_get_pixel_in_row(row, x);
                    raw_set_pixel_in_row(row, x + scroll, c);
                }
                // fill left area with brush color
                for (int x = x1; x < x1 + scroll; ++x)
                    raw_set_pixel_in_row(row, x, pattern);
            }
        }
    }
};


} // end of namespace
