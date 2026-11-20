#!/bin/sh

# Прошивает плату и открывает монитор. /dev/ttyACM0 — мост USB CH9102F
# платы T-Display; выход из монитора — Ctrl+].
idf.py -p /dev/ttyACM0 flash monitor
