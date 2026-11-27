# Espresso

ESP32 firmware for the LilyGO TTGO VGA32 v1.4 board: a standalone Forth computer with a VGA
monitor and a PS/2 keyboard (design: `doc/md/40-design.md`). It runs the ff Forth engine on a
terminal — a VT220 subset on the monitor, mirrored to the serial port — with a line editor and a
line history kept on the SD card; the `board-test` word checks the VGA output, keyboard, mouse
and sound.

The Forth engine [ff](https://github.com/stoneagesoft/ff) is a git submodule, so clone with
submodules (or run `git submodule update --init` in an existing clone).

Requires ESP-IDF 6.1; load its environment first, then use the scripts:

```sh
git clone --recurse-submodules <this repository>
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
  * `src/vt`: the terminal — a VT220 subset on the VGA screen, PS/2 and UART0 input, the line
    editor (`esp::vt`); the parts that don't depend on ESP-IDF have host tests in `test`
    (`test/run.sh`).
  * `src/shell`: the Forth shell and its words (`esp::shell`).
  * `src/fonts`: the Unicode Terminus 10×18 font tables and the script that generates them.
  * `src/3rdparty`: the yyjson and utf8.h libraries, and the ff submodule, which is built by its
    own `CMakeLists.txt` as a subproject.

## FabGL

The graphics, VGA, PS/2 and sound classes and the shared helpers in `src/tools/esp_tools.*` are
derived from [FabGL](http://www.fabgl.com) 1.0.9 by Fabrizio Di Vittorio (GPL-3.0), taken from
its ESP-IDF 6 port (https://github.com/shaeberling/FabGL, branch idf6-port, commit 3f21e97).
`sprite::next_frame()` includes a fix by Dean Belfield from vdp-gl
(https://github.com/AgonConsole8/vdp-gl). `src/ps2/esp_ps2_ulp_macro_ex.cpp` is based on
`ulp_macro.c` from ESP-IDF (Espressif, Apache-2.0).

## License

GPL-3.0, see `LICENSE`. The third-party parts keep their own licences: yyjson and ff are MIT,
utf8.h is public domain, and the Terminus font is SIL OFL 1.1 (`src/fonts/LICENSE.terminus`).
