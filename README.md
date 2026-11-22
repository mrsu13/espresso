# espresso

ESP32 firmware for the LilyGO TTGO VGA32 v1.4 board. For now it tests the VGA output, the PS/2
keyboard and mouse, and the sound, and prints the board info and a debug log to the serial
console.

Requires ESP-IDF 6.1; load its environment first, then use the scripts:

```sh
. ~/esp/esp-idf/export.sh   # path to your ESP-IDF 6.1
./build.sh                  # build/espresso.bin
./install.sh                # flash through /dev/ttyUSB0
./clean.sh                  # remove build/
```

## Source layout

The whole program is one ESP-IDF component, `src` (see `src/CMakeLists.txt`):

  * `src/main.cpp`: the entry point.
  * `src/app`: the application.
  * `src/tools`: helpers for the board (button, SD card, chip info) and helpers shared by the
    modules below.
  * `src/gfx`, `src/vga`, `src/ps2`, `src/sound`: graphics, VGA output, PS/2 keyboard and mouse,
    and sound (namespaces `esp::gfx`, `esp::vga`, `esp::ps2` and `esp::sound`).
  * `src/fonts`: the Unicode Terminus 10×18 font tables and the script that generates them.
  * `src/3rdparty`: the yyjson and utf8.h libraries.

## FabGL

The graphics, VGA, PS/2 and sound classes and the shared helpers in `src/tools/esp_tools.*` are
derived from [FabGL](http://www.fabgl.com) 1.0.9 by Fabrizio Di Vittorio (GPL-3.0), taken from
its ESP-IDF 6 port (https://github.com/shaeberling/FabGL, branch idf6-port, commit 3f21e97).
`Sprite::nextFrame()` includes a fix by Dean Belfield from vdp-gl
(https://github.com/AgonConsole8/vdp-gl). `src/ps2/esp_ps2_ulp_macro_ex.cpp` is based on
`ulp_macro.c` from ESP-IDF (Espressif, Apache-2.0).

## License

GPL-3.0, see `LICENSE`. The third-party parts keep their own licences: yyjson is MIT, utf8.h is
public domain, and the Terminus font is SIL OFL 1.1 (`src/fonts/LICENSE.terminus`).
