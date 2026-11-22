// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 *
 * @brief This file contains esp::gfx::Canvas definition.
 */


#include <stddef.h>
#include <stdint.h>

#include "esp_gfx_display_controller.h"


namespace esp::gfx
{


/**
* @brief A class with a set of drawing methods.
*
* This class interfaces directly to the display controller and provides
* a set of primitives to paint lines, circles, etc. and to scroll regions, copy
* rectangles and draw glyphs.<br>
* For default origin is at the top left, starting from (0, 0) up to (Canvas Width - 1, Canvas Height - 1).
*
* Example:
*
*     // Setup pins and resolution (5 GPIOs hence we have up to 8 colors)
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
*     VGAController.setResolution(VGA_640x350_70Hz);
*
*     // Paint a green rectangle with red border
*     esp::gfx::Canvas cv(&VGAController);
*     cv.setPenColor(Color::BrightRed);
*     cv.setBrushColor(Color::BrightGreen);
*     cv.fillRectangle(0, 0, cv.getWidth() - 1, cv.getHeight() - 1);
*     cv.drawRectangle(0, 0, cv.getWidth() - 1, cv.getHeight() - 1);
*
*/
class canvas
{

public:

    canvas(bitmapped_display_controller *display_controller);

    /**
   * @brief Determines the canvas width in pixels.
   *
   * This is equivalent to VGA Controller viewport width.
   *
   * @return The canvas width in pixels.
   */
    int width()
    {
        return _display_controller->view_port_width();
    }

    /**
   * @brief Determines the canvas height in pixels.
   *
   * This is equivalent to VGA Controller viewport height.
   *
   * @return The canvas height in pixels.
   */
    int height()
    {
        return _display_controller->view_port_height();
    }

    /**
   * @brief Waits for drawing queue to become empty.
   *
   * @param waitVSync If true drawings are done on vertical retracing (slow), if false drawings are perfomed immediately (fast with flickering).
   */
    void wait_completion(bool wait_v_sync = true);

    /**
   * @brief Suspends drawings.
   *
   * After call to beginUpdate() adding new primitives may cause a deadlock.<br>
   * To avoid this a call to "Canvas.waitCompletion(false)" should be performed very often.
   */
    void begin_update();

    /**
   * @brief Resumes drawings after beginUpdate().
   */
    void end_update();

    /**
   * @brief Swaps screen buffer when double buffering is enabled.
   *
   * Double buffering is enabled calling VGAController.setResolution() with doubleBuffered = true.<br>
   * Buffers swap is always executed in vertical retracing (at VSync pulse).
   */
    void swap_buffers();

    /**
   * @brief Sets the axes origin.
   *
   * Setting axes origin will translate every coordinate by the specified value (except for sprites).
   *
   * @param X Horizontal translation (0 = no translation).
   * @param Y Vertical translation (0 = no translation).
   */
    void set_origin(int x, int y);

    /**
   * @brief Sets the axes origin.
   *
   * Setting axes origin will translate every coordinate by the specified value (except for sprites).
   *
   * @param origin Origin coordinates.
   */
    void set_origin(point const &origin);

    /**
   * @brief Gets last origin set using setOrigin().
   *
   * @return Last origin set.
   */
    point origin()
    {
        return _origin;
    }

    /**
   * @brief Sets clipping rectangle relative to the origin.
   *
   * The default clipping rectangle covers the entire canvas area.
   *
   * @param rect Clipping rectangle relative to the origin.
   */
    void set_clipping_rect(rect const &rect);

    /**
   * @brief Gets last clipping rectangle set using setClippingRect().
   *
   * @return Last clipping rectangle set.
   */
    rect clipping_rect();


    /**
   * @brief Fills the entire canvas with the brush color.
   */
    void clear();

    /**
   * @brief Resets paint state and other display controller settings.
   */
    void reset();

    /**
   * @brief Defines the scrolling region.
   *
   * A scrolling region is the rectangle area where Canvas.scroll() method can operate.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordiante.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordiante.
   */
    void set_scrolling_region(int x1, int y1, int x2, int y2);

    /**
   * @brief Scrolls pixels horizontally and/or vertically.
   *
   * Scrolling occurs inside the scrolling region defined by Canvas.setScrollingRegion().<br>
   * Vertical scroll is done moving line pointers, so it is very fast to perform.<br>
   * Horizontal scroll is done moving pixel by pixel (CPU intensive task).
   *
   * @param offsetX Number of pixels to scroll left (offsetX < 0) or right (offsetX > 0).
   * @param offsetY Number of pixels to scroll up (offsetY < 0) or down (offsetY > 0).
   */
    void scroll(int offset_x, int offset_y);

    /**
   * @brief Moves current pen position to the spcified coordinates.
   *
   * Some methods expect initial pen position to be specified, like Canvas.lineTo().
   *
   * @param X Horizontal pen position.
   * @param Y Vertical pen position.
   *
   * Example:
   *
   *     // Paint a triangle
   *     Canvas.moveTo(10, 12);
   *     Canvas.lineTo(30, 40);
   *     Canvas.lineTo(10, 12);
   */
    void move_to(int x, int y);

    /**
   * @brief Sets pen (foreground) color specifying color components.
   *
   * @param red Red color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   * @param green Green color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   * @param blue Blue color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   *
   * Example:
   *
   *     // Set white pen
   *     Canvas.setPenColor(255, 255, 255);
   */
    void set_pen_color(uint8_t red, uint8_t green, uint8_t blue);

    /**
   * @brief Sets pen (foreground) color using a color name.
   *
   * @param color Color name.
   *
   * Example:
   *
   *      // Set white pen
   *      Canvas.setPenColor(Color::BrightWhite);
   */
    void set_pen_color(color color);

    /**
   * @brief Sets pen (foreground) color specifying color components.
   *
   * @param color Color RGB888 components.
   *
   * Example:
   *
   *      // Set white pen
   *      Canvas.setPenColor(Color::BrightWhite);
   */
    void set_pen_color(rgb888 const &color);

    /**
   * @brief Sets brush (background) color specifying color components.
   *
   * @param red Red color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   * @param green Green color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   * @param blue Blue color component. Minimum value is 0, maximum value is 1 with 8 colors and 3 with 64 colors.
   *
   * Example:
   *
   *     // Set blue brush with 8 colors setup
   *     Canvas.setBrushColor(0, 0, 1);
   *
   *     // Set blue brush with 64 colors setup
   *     Canvas.setBrushColor(0, 0, 3);
   */
    void set_brush_color(uint8_t red, uint8_t green, uint8_t blue);

    /**
   * @brief Sets brush (background) color using a color name.
   *
   * @param color The color name.
   *
   * Example:
   *
   *      // Set blue brush
   *      Canvas.setBrushColor(Color::BrightBlue);
   */
    void set_brush_color(color color);

    /**
   * @brief Sets brush (background) color specifying color components.
   *
   * @param color The color RGB888 components.
   *
   * Example:
   *
   *      // Set blue brush
   *      Canvas.setBrushColor(Color::BrightBlue);
   */
    void set_brush_color(rgb888 const &color);

    /**
   * @brief Sets pen width for lines, rectangles and paths
   *
   * @param value Pen width (minimum is 1).
   *
   * Example:
   * 
   *     Canvas.setPenWidth(4);
   *     Canvas.drawLine(10, 10, 100, 10);
   */
    void set_pen_width(int value);

    /**
   * @brief Sets line ends shape
   *
   * @param value Line ends shape.
   *
   * Example:
   *
   *     Canvas.setPenWidth(4);
   *     Canvas.setLineEnds(LineEnds::Circle);
   *     Canvas.drawLine(10, 10, 100, 10);
   */
    void set_line_ends(line_ends value);

    /**
   * @brief Fills a single pixel with the pen color.
   *
   * @param X Horizontal pixel position.
   * @param Y Vertical pixel position.
   */
    void set_pixel(int x, int y);

    /**
   * @brief Fills a single pixel with the specified color.
   *
   * @param X Horizontal pixel position.
   * @param Y Vertical pixel position.
   * @param color Pixe color.
   */
    void set_pixel(int x, int y, rgb888 const &color);

    /**
   * @brief Fills a single pixel with the specified color.
   *
   * @param pos Pixel position.
   * @param color Pixe color.
   */
    void set_pixel(point const &pos, rgb888 const &color);

    /**
   * @brief Draws a line starting from current pen position.
   *
   * Starting pen position is specified using Canvas.moveTo() method or
   * from ending position of the last call to Canvas.lineTo().<br>
   * The line has the current pen color.
   *
   * @param X Horizontal ending line position.
   * @param Y Vertical ending line position.
   *
   * Example:
   *
   *     // Paint white a triangle
   *     Canvas.setPenColor(Color::BrightWhite);
   *     Canvas.moveTo(10, 12);
   *     Canvas.lineTo(30, 40);
   *     Canvas.lineTo(10, 12);
   */
    void line_to(int x, int y);

    /**
   * @brief Draws a line specifying initial and ending coordinates.
   *
   * The line has the current pen color.
   *
   * @param X1 Starting horizontal coordinate.
   * @param Y1 Starting vertical coordinate.
   * @param X2 Ending horizontal coordinate.
   * @param Y2 Ending vertical coordinate.
   *
   * Example:
   *
   *     // Paint a blue X over the whole canvas
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.drawLine(0, 0, Canvas.getWidth() - 1, Canvas.getHeight() - 1);
   *     Canvas.drawLine(Canvas.getWidth() - 1, 0, 0, Canvas.getHeight() - 1);
   */
    void draw_line(int x1, int y1, int x2, int y2);

    /**
   * @brief Draws a rectangle using the current pen color.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordiante.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordiante.
   *
   * Example:
   *
   *     // Paint a yellow rectangle
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawRectangle(10, 10, 100, 100);
   */
    void draw_rectangle(int x1, int y1, int x2, int y2);

    /**
   * @brief Draws a rectangle using the current pen color.
   *
   * @param rect Rectangle coordinates.
   */
    void draw_rectangle(rect const &rect);

    /**
   * @brief Fills a rectangle using the current brush color.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordiante.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordiante.
   *
   * Example:
   *
   *     // Paint a filled yellow rectangle
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(10, 10, 100, 100);
   *
   *     // Paint a yellow rectangle with blue border
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(10, 10, 100, 100);
   *     Canvas.drawRectangle(10, 10, 100, 100);
   */
    void fill_rectangle(int x1, int y1, int x2, int y2);

    /**
   * @brief Fills a rectangle using the current brush color.
   *
   * @param rect Rectangle coordinates.
   *
   * Example:
   *
   *     // Paint a filled yellow rectangle
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(Rect(10, 10, 100, 100));
   */
    void fill_rectangle(rect const &rect);

    /**
   * @brief Inverts a rectangle.
   *
   * The Not logic operator is applied to pixels inside the rectangle.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordiante.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordiante.
   */
    void invert_rectangle(int x1, int y1, int x2, int y2);

    /**
   * @brief Inverts a rectangle.
   *
   * The Not logic operator is applied to pixels inside the rectangle.
   *
   * @param rect Rectangle coordinates.
   */
    void invert_rectangle(rect const &rect);

    /**
   * @brief Swaps pen and brush colors of the specified rectangle.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordiante.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordiante.
   */
    void swap_rectangle(int x1, int y1, int x2, int y2);

    /**
   * @brief Draws an ellipse specifying center and size, using current pen color.
   *
   * @param X Horizontal coordinate of the ellipse center.
   * @param Y Vertical coordinate of the ellipse center.
   * @param width Ellipse width.
   * @param height Ellipse height.
   *
   * Example:
   *
   *     // Paint a yellow ellipse
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawEllipse(100, 100, 40, 40);
   */
    void draw_ellipse(int x, int y, int width, int height);

    /**
   * @brief Fills an ellipse specifying center and size, using current brush color.
   *
   * @param X Horizontal coordinate of the ellipse center.
   * @param Y Vertical coordinate of the ellipse center.
   * @param width Ellipse width.
   * @param height Ellipse height.
   *
   * Example:
   *
   *     // Paint a filled yellow ellipse
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillEllipse(100, 100, 40, 40);
   *
   *     // Paint a yellow ellipse with blue border
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillEllipse(100, 100, 40, 40);
   *     Canvas.drawEllipse(100, 100, 40, 40);
   */
    void fill_ellipse(int x, int y, int width, int height);

    /**
   * @brief Draws a glyph at specified position.
   *
   * A Glyph is a monochrome bitmap (1 bit per pixel) that can be painted using pen (foreground) and brush (background) colors.<br>
   * Various drawing options can be set using Canvas.setGlyphOptions() method.<br>
   * Glyphs are used by Terminal to render characters.
   *
   * @param X Horizontal coordinate where to draw the glyph.
   * @param Y Vertical coordinate where to draw the glyph.
   * @param width Horizontal size of the glyph.
   * @param height Vertical size of the glyph.
   * @param data Memory buffer containing the glyph. Each line is byte aligned. The size must be "(width + 7) / 8 * height" (integer division!).
   * @param index Optional index inside data. Use when the buffer contains multiple glyphs.
   *
   * Example:
   *
   *     // draw an 'A' using the predefined 8x8 font
   *     Canvas.setPenColor(Color::BrightGreen);
   *     const esp::gfx::FontInfo * f = &fabgl::FONT_8x8;
   *     Canvas.drawGlyph(0, 0, f->width, f->height, f->data, 0x41);
   *
   *     // draw a 12x8 sprite
   *     const uint8_t enemy[] = {
   *       0x0f, 0x00, 0x7f, 0xe0, 0xff, 0xf0, 0xe6, 0x70, 0xff,
   *       0xf0, 0x39, 0xc0, 0x66, 0x60, 0x30, 0xc0,
   *     };
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawGlyph(50, 80, 12, 8, enemy);
   *
   */
    void draw_glyph(int x, int y, int width, int height, uint8_t const *data, int index = 0);

    /**
   * @brief Sets drawing options for the next glyphs.
   *
   * Setting glyph options allows to slightly change how a glyph is rendered, applying
   * effects like Bold, Italic, Inversion, double width or height and so on.<br>
   * Because Canvas draws text using glyphs these options affects
   * also how text is rendered.
   *
   * @param options A bit field to specify multiple options
   *
   * Example:
   *
   *     // Draw "Hello World!"
   *     Canvas.setGlyphOptions(GlyphOptions().FillBackground(true).DoubleWidth(1));
   *     Canvas.drawText(20, 20, "Hello World!");
   */
    void set_glyph_options(glyph_options options);

    /**
   * @brief Resets glyph options.
   */
    void reset_glyph_options();

    /**
   * @brief Sets paint options.
   */
    void set_paint_options(paint_options options);

    /**
   * @brief Resets paint options.
   */
    void reset_paint_options();

    /**
   * @brief Gets info about currently selected font.
   *
   * @return FontInfo structure representing font info and data.
   */
    font_info const *get_font_info()
    {
        return _font_info;
    }

    /**
   * @brief Selects a font to use for the next text drawings.
   *
   * @param fontInfo Pointer to font structure containing font info and glyphs data.
   *
   * Examples:
   *
   *     // Set a font for about 40x14 text screen
   *     Canvas.selectFont(fabgl::getPresetFontInfo(Canvas.getWidth(), Canvas.getHeight(), 40, 14));
   *
   *     // Set the 8x8 predefined font
   *     Canvas.selectFont(&fabgl::FONT_8x8);
   */
    void select_font(font_info const *font_info);

    /**
   * @brief Draws a character at specified position.
   *
   * drawChar() uses currently selected font (selectFont() method) and currently selected glyph options (setGlyphOptions() method).
   *
   * @param X Horizontal position of character left side.
   * @param Y Vertical position of character top side.
   * @param c Character to draw (an index in the character font glyphs set).
   *
   * Example:
   *
   *     // Draw a 'C' at position 100, 100
   *     Canvas.drawChar(100, 100, 'C');
   */
    void draw_char(int x, int y, char c);

    /**
   * @brief Draws a string at specified position.
   *
   * drawText() uses currently selected font (selectFont() method) and currently selected glyph options (setGlyphOptions() method).
   *
   * @param X Horizontal position of first character left side.
   * @param Y Vertical position of first character top side.
   * @param text String to draw (indexes in the character font glyphs set).
   * @param wrap If true text is wrapped at the end of line.
   *
   * Example:
   *
   *     // Draw a 'Hello World!' at position 100, 100
   *     Canvas.drawText(100, 100, "Hellow World!");
   */
    void draw_text(int x, int y, char const *text, bool wrap = false);

    /**
   * @brief Draws a string at specified position.
   *
   * drawText() uses the specified font and currently selected glyph options (setGlyphOptions() method).
   *
   * @param fontInfo Pointer to font structure containing font info and glyphs data.
   * @param X Horizontal position of first character left side.
   * @param Y Vertical position of first character top side.
   * @param text String to draw (indexes in the character font glyphs set).
   * @param wrap If true text is wrapped at the end of line.
   *
   * Example:
   *
   *     // Draw a 'Hello World!' at position 100, 100
   *     Canvas.drawText(&fabgl::FONT_8x8, 100, 100, "Hellow World!");
   */
    void draw_text(font_info const *font_info, int x, int y, char const *text, bool wrap = false);

    /**
   * @brief Draws a string at specified position. Add ellipses before truncation.
   *
   * @param fontInfo Pointer to font structure containing font info and glyphs data.
   * @param X Horizontal position of first character left side.
   * @param Y Vertical position of first character top side.
   * @param text String to draw (indexes in the character font glyphs set).
   * @param maxX Maximum horizontal coordinate allowed.
   */
    void draw_text_with_ellipsis(font_info const *font_info, int x, int y, char const *text,
                                 int max_x);

    /**
   * @brief Calculates text extension in pixels.
   *
   * @param fontInfo Pointer to font structure containing font info and glyphs data.
   * @param text String to calculate length (indexes in the character font glyphs set).
   */
    int text_extent(font_info const *font_info, char const *text);

    /**
   * @brief Calculates text extension in pixels.
   *
   * @param text String to calculate length (indexes in the character font glyphs set).
   */
    int text_extent(char const *text);

    /**
   * @brief Draws formatted text at specified position.
   *
   * drawTextFmt() uses currently selected font (selectFont() method) and currently selected glyph options (setGlyphOptions() method).
   *
   * @param X Horizontal position of first character left side.
   * @param Y Vertical position of first character top side.
   * @param format Format specifier like printf.
   *
   * Example:
   *
   *     Canvas.drawTextFmt(100, 100, "Free DMA memory: %d", heap_caps_get_free_size(MALLOC_CAP_DMA));
   */
    void draw_text_fmt(int x, int y, const char *format, ...);

    /**
   * @brief Copies a screen rectangle to the specified position.
   *
   * Source and destination rectangles can overlap. The copy is performed pixel by pixel and it is a slow operation.
   *
   * @param sourceX Left source rectangle horizontal position.
   * @param sourceY Top source rectangle vertical position.
   * @param destX Left destination rectangle horizontal position.
   * @param destY Top destination rectangle vertical position.
   * @param width Rectangle horizontal size.
   * @param height Rectangle vertical size.
   */
    void copy_rect(int source_x, int source_y, int dest_x, int dest_y, int width, int height);

    /**
   * @brief Draws a bitmap at specified position.
   *
   * A bitmap is an rectangular image with one byte per pixel. Each pixel has up to 64 colors (2 bits per channel) and
   * can have 4 level of transparency. At the moment only level 0 (full transparent) and level 3 (full opaque)
   * is supported.
   *
   * @param X Horizontal position of bitmap left side.
   * @param Y Vertical position of bitmap top side.
   * @param bitmap Pointer to bitmap structure.
   */
    void draw_bitmap(int x, int y, bitmap const *bitmap);

    /**
   * @brief Draws a sequence of lines.
   *
   * @param points A pointer to an array of Point objects. Points array is copied to a temporary buffer.
   * @param pointsCount Number of points in the array.
   *
   * Example:
   *
   *     Point points[3] = { {10, 10}, {20, 10}, {15, 20} };
   *     Canvas.setPenColor(Color::Red);
   *     Canvas.drawPath(points, 3);
   */
    void draw_path(point const *points, int points_count);

    /**
   * @brief Fills the polygon enclosed in a sequence of lines.
   *
   * @param points A pointer to an array of Point objects. Points array is copied to a temporary buffer.
   * @param pointsCount Number of points in the array.
   *
   * Example:
   *
   *     Point points[3] = { {10, 10}, {20, 10}, {15, 20} };
   *     Canvas.setBrushColor(Color::Red);
   *     Canvas.fillPath(points, 3);
   */
    void fill_path(point const *points, int points_count);

    /**
   * @brief Reads the pixel at specified position.
   *
   * Screen reading may occur while other drawings are in progress, so the result may be not updated. To avoid it, call getPixel() after
   * waitCompletion() or disable background drawings.
   *
   * @param X Horizontal coordinate.
   * @param Y Vertical coordinate.
   *
   * @return Pixel color as RGB888 structure.
   */
    rgb888 pixel(int x, int y);

private:

    bitmapped_display_controller *_display_controller;

    font_info const *_font_info;
    uint8_t
        _text_horiz_rate; // specify character size: 1 = m_fontInfo.width, 2 = m_fontInfo.width * 2, etc...

    point _origin;
    rect _clipping_rect;
};


} // end of namespace
