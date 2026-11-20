# espresso

ESP32 firmware: mounts the SD card and prints the board info and debug log
to the serial console.

Requires ESP-IDF 6.1; load its environment first, then use the scripts:

```sh
. ~/esp/esp-idf/export.sh   # path to your ESP-IDF 6.1
./build.sh                  # build/espresso.bin
./install.sh                # flash through /dev/ttyUSB0
./clean.sh                  # remove build/
```
