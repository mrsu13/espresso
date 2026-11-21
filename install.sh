#!/bin/sh

# Прошивает плату и открывает монитор. /dev/ttyUSB0 — порт USB платы
# TTGO VGA32; выход из монитора — Ctrl+].
idf.py -p /dev/ttyUSB0 flash monitor
