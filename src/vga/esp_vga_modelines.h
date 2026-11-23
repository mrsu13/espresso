// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Готовые модлайны видеорежимов для esp::vga::vga_base_controller::set_resolution().
 *
 * Модлайн — строка в формате XFree86: имя режима, частота пикселей в МГц,
 * четыре горизонтальных отсчёта (конец видимой части, начало и конец
 * синхроимпульса, полная длина строки), четыре вертикальных и необязательные
 * слова: полярность синхроимпульсов (+HSync, -VSync...), DoubleScan или
 * QuadScan, начальный участок строки (FrontPorchBegins, SyncBegins,
 * BackPorchBegins, VisibleBegins) и MultiScanBlank. Разбирает её
 * esp::vga::vga_base_controller::convert_modeline_to_timings().
 */


namespace esp::vga
{

/**
 * @brief 256×192, 50 Гц, каждая строка выводится дважды.
 *
 * Нужен монитор или преобразователь, который принимает такую развёртку.
 */
constexpr char VGA_256X192_50HZ[] =
    "\"256x192@50\" 8.13 256 288 296 328 192 196 198 202 -HSync -VSync DoubleScan";

/** @brief 256×384, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_256X384_60HZ[] =
    "\"256x384@60\" 17.09 256 272 304 352 384 387 391 404 -HSync -VSync DoubleScan";

/** @brief 320×200, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_320X200_60HZ_D[] =
    "\"320x200@60HzD\" 25.175 320 328 376 400 200 226 227 262 -HSync -VSync DoubleScan";

/**
 * @brief 320×200, 70 Гц, каждая строка выводится дважды.
 *
 * То же, что VGA_640X200_70HZ, с половинной шириной.
 */
constexpr char VGA_320X200_70HZ[] =
    "\"320x200@70Hz\" 12.5875 320 328 376 400 200 206 207 224 -HSync -VSync DoubleScan";

/** @brief 320×200, 75 Гц, каждая строка выводится дважды. */
constexpr char VGA_320X200_75HZ[] =
    "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan";

/** @brief 320×200, 75 Гц, каждая строка выводится дважды, повторы строк чёрные, как на старом мониторе. */
constexpr char VGA_320X200_75HZ_RETRO[] =
    "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan MultiScanBlank";

/** @brief 320×240, 60 Гц, каждая строка выводится дважды. */
constexpr char QVGA_320X240_60HZ[] =
    "\"320x240@60Hz\" 12.6 320 328 376 400 240 245 246 262 -HSync -VSync DoubleScan";

/** @brief 400×300, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_400X300_60HZ[] =
    "\"400x300@60Hz\" 20 400 420 484 528 300 300 302 314 -HSync -VSync DoubleScan";

/** @brief 480×300, 75 Гц, каждая строка выводится дважды. */
constexpr char VGA_480X300_75HZ[] =
    "\"480x300@75Hz\" 31.22 480 504 584 624 300 319 322 333 -HSync -VSync DoubleScan";

/** @brief 512×192, 60 Гц, каждая строка выводится четыре раза. */
constexpr char VGA_512X192_60HZ[] =
    "\"512x192@60Hz\" 32.5 512 524 592 672 192 193 194 202 -HSync -VSync QuadScan";

/** @brief 512×384, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_512X384_60HZ[] =
    "\"512x384@60Hz\" 32.5 512 524 592 672 384 385 388 403 -HSync -VSync DoubleScan";

/** @brief 512×448, 60 Гц. */
constexpr char VGA_512X448_60HZ[] =
    "\"512x448@60Hz\" 21.21 512 542 598 672 448 469 472 527 -HSync -VSync";

/** @brief 512×512, 58 Гц. */
constexpr char VGA_512X512_58HZ[] =
    "\"512x512@58Hz\" 21.21 512 538 594 668 512 513 516 545 -HSync -VSync";

/** @brief 640×200, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_640X200_60HZ_D[] =
    "\"640x200@60HzD\" 25.175 640 656 752 800 200 226 227 262 -HSync -VSync doublescan";

/**
 * @brief 640×200, 70 Гц, каждая строка выводится дважды.
 *
 * То же, что VGA_640X400_70HZ, с половинным числом строк.
 */
constexpr char VGA_640X200_70HZ[] =
    "\"640x200@70Hz\" 25.175 640 656 752 800 200 206 207 224 -HSync -VSync DoubleScan";

/** @brief 640×200, 70 Гц, каждая строка выводится дважды, повторы строк чёрные, как на старом мониторе. */
constexpr char VGA_640X200_70HZ_RETRO[] =
    "\"640x200@70Hz\" 25.175 640 663 759 808 200 208 211 226 -HSync -VSync DoubleScan MultiScanBlank";

/** @brief 640×240, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_640X240_60HZ[] =
    "\"640x240@60Hz\" 25.175 640 656 752 800 240 245 247 262 -HSync -VSync DoubleScan";

/**
 * @brief 640×240, 60 Гц, развёртка NTSC.
 *
 * Нужен монитор или преобразователь, который принимает такую развёртку. Подобрал Paul Rickards (http://biosrhythm.com).
 */
constexpr char NTSC_640X240_60HZ[] =
    "\"NTSC 640x240 (60Hz)\" 12.312 640 662 719 784 240 244 247 262 -hsync -vsync";

/** @brief 640×350, 70 Гц. */
constexpr char VGA_640X350_70HZ[] =
    "\"640x350@70Hz\" 25.175 640 656 752 800 350 387 389 449 -HSync -VSync";

/** @brief 640×350, 70 Гц, второй вариант. */
constexpr char VGA_640X350_70HZ_ALT1[] =
    "\"640x350@70HzAlt1\" 25.175 640 658 754 808 350 387 389 449 -HSync -VSync";

/** @brief 640×350, 85 Гц. */
constexpr char VESA_640X350_85HZ[] =
    "\"640x350@85Hz\" 31.5 640 672 736 832 350 382 385 445 -HSync -VSync";

/** @brief 640×382, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_640X382_60HZ[] =
    "\"640x382@60Hz\" 40 640 672 740 840 382 385 387 397 -HSync +VSync DoubleScan";

/** @brief 640×384, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_640X384_60HZ[] =
    "\"640x384@60Hz\" 40 640 672 740 840 384 384 386 397 -HSync +VSync DoubleScan";

/** @brief 640×400, 70 Гц. */
constexpr char VGA_640X400_70HZ[] =
    "\"640x400@70Hz\" 25.175 640 656 752 800 400 412 414 449 -HSync -VSync";

/**
 * @brief 640×400, 60 Гц.
 *
 * Развёртка 640×480 с меньшим числом видимых строк.
 */
constexpr char VGA_640X400_60HZ[] =
    "\"640x400@60Hz-mod\" 25.175 640 656 752 800 400 452 454 525 -HSync -VSync";

/** @brief 640×480, 60 Гц. */
constexpr char VGA_640X480_60HZ[] =
    "\"640x480@60Hz\" 25.175 640 656 752 800 480 490 492 525 -HSync -VSync";

/** @brief 640×480, 60 Гц, второй вариант. */
constexpr char VGA_640X480_60HZ_ALT1[] =
    "\"640x480@60HzAlt1\" 27.5 640 672 768 864 480 482 488 530 -HSync -VSync";

/** @brief 640×480, 60 Гц, каждая строка выводится дважды. */
constexpr char VGA_640X480_60HZ_D[] =
    "\"640x480@60HzD\" 54.00 640 688 744 900 480 480 482 500 +HSync +VSync DoubleScan";

/** @brief 640×480, 73 Гц. */
constexpr char VGA_640X480_73HZ[] =
    "\"640x480@73Hz\" 31.5 640 664 704 832 480 489 491 520 -HSync -VSync";

/** @brief 640×480, 75 Гц. */
constexpr char VESA_640X480_75HZ[] =
    "\"640x480@75Hz\" 31.5 640 656 720 840 480 481 484 500 -HSync -VSync";

/**
 * @brief 720×240, 60 Гц, развёртка NTSC.
 *
 * Нужен монитор или преобразователь, который принимает такую развёртку. Подобрал Paul Rickards (http://biosrhythm.com).
 */
constexpr char NTSC_720X240_60HZ[] =
    "\"NTSC 720x240 (60Hz)\" 13.820 720 744 809 880 240 244 247 262 -hsync -vsync";

/**
 * @brief 720×288, 50 Гц, развёртка PAL.
 *
 * Нужен монитор или преобразователь, который принимает такую развёртку. Подобрал Paul Rickards (http://biosrhythm.com).
 */
constexpr char PAL_720X288_50HZ[] =
    "\"PAL 720x288 (50Hz)\" 13.853 720 741 806 888 288 290 293 312 -hsync -vsync";

/** @brief 720×348, 50 Гц, каждая строка выводится дважды. */
constexpr char VGA_720X348_50HZ_D[] =
    "\"720x348@50HzD\" 30.84 720 752 808 840 348 355 358 366 -hsync -vsync doublescan";

/** @brief 720×348, 59 Гц, каждая строка выводится дважды. */
constexpr char VGA_720X348_59HZ_D[] =
    "\"720x348@59d\" 38.87 720 788 852 896 348 350 359 366 -HSync -VSync DoubleScan";

/** @brief 720×348, 73 Гц. */
constexpr char VGA_720X348_73HZ[] =
    "\"720x348@73Hz\" 27 720 736 799 872 348 379 381 433 -HSync -VSync";

/**
 * @brief 720×350, 70 Гц.
 *
 * Подобрал Stan Pechal.
 */
constexpr char VGA_720X350_70HZ[] =
    "\"720x350@70Hz\" 28.32 720 738 846 900 350 387 389 449 -HSync -VSync";

/** @brief 720×400, 70 Гц. */
constexpr char VGA_720X400_70HZ[] =
    "\"720x400@70Hz\" 28.32  720 738 846 900  400 412 414 449 -hsync +vsync";

/** @brief 720×400, 85 Гц. */
constexpr char VESA_720X400_85HZ[] =
    "\"720x400@85Hz\" 35.5 720 756 828 936 400 401 404 446 -HSync -VSync";

/** @brief 720×576, 50 Гц, развёртка PAL. */
constexpr char PAL_720X576_50HZ[] =
    "\"720x576@50Hz\" 27 720 732 795 864 576 581 586 625 -HSync -VSync";

/**
 * @brief 768×288, 50 Гц, развёртка PAL.
 *
 * Нужен монитор или преобразователь, который принимает такую развёртку. Подобрал Paul Rickards (http://biosrhythm.com).
 */
constexpr char PAL_768X288_50HZ[] =
    "\"PAL 768x288 (50Hz)\" 14.726 768 790 859 944 288 290 293 312 -hsync -vsync";

/** @brief 768×576, 60 Гц. */
constexpr char VESA_768X576_60HZ[] =
    "\"768x576@60Hz\" 34.96 768 792 872 976 576 577 580 597 -HSync -VSync";

/** @brief 800×300, 60 Гц, каждая строка выводится дважды. */
constexpr char SVGA_800X300_60HZ[] =
    "\"800x300@60Hz\" 40 800 840 968 1056 300 301 303 314 -HSync -VSync DoubleScan";

/** @brief 800×600, 56 Гц. */
constexpr char SVGA_800X600_56HZ[] =
    "\"800x600@56Hz\" 36 800 824 896 1024 600 601 603 625 -HSync -VSync";

/** @brief 800×600, 60 Гц. */
constexpr char SVGA_800X600_60HZ[] =
    "\"800x600@60Hz\" 40 800 840 968 1056 600 601 605 628 -HSync -VSync";

/** @brief 960×540, 60 Гц. */
constexpr char SVGA_960X540_60HZ[] =
    "\"960x540@60Hz\" 37.26 960 976 1008 1104 540 542 548 563 +hsync +vsync";

/** @brief 1024×768, 60 Гц. */
constexpr char SVGA_1024X768_60HZ[] =
    "\"1024x768@60Hz\" 65 1024 1048 1184 1344 768 771 777 806 -HSync -VSync";

/** @brief 1024×768, 70 Гц. */
constexpr char SVGA_1024X768_70HZ[] =
    "\"1024x768@70Hz\" 75 1024 1048 1184 1328 768 771 777 806 -HSync -VSync";

/** @brief 1024×768, 75 Гц. */
constexpr char SVGA_1024X768_75HZ[] =
    "\"1024x768@75Hz\" 78.80 1024 1040 1136 1312 768 769 772 800 +HSync +VSync";

/** @brief 1280×600, 60 Гц. */
constexpr char SVGA_1280X600_60HZ[] =
    "\"1280x600@60Hz\" 61.5 1280 1336 1464 1648 600 601 604 622 -HSync -VSync";

/** @brief 1280×720, 60 Гц. */
constexpr char SVGA_1280X720_60HZ[] =
    "\"1280x720@60Hz\" 74.48 1280 1468 1604 1664 720 721 724 746 +hsync +vsync";

/** @brief 1280×720, 60 Гц, второй вариант. */
constexpr char SVGA_1280X720_60HZ_ALT1[] =
    "\"1280x720@60HzAlt1\" 73.78 1280 1312 1592 1624 720 735 742 757";

/** @brief 1280×768, 50 Гц. */
constexpr char SVGA_1280X768_50HZ[] =
    "\"1280x768@50Hz\" 64.050004 1280 1312 1552 1584 768 784 791 807 -HSync -VSync";

}
