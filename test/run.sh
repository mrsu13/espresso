#!/bin/sh

# Собирает проверки на ПК в test/build и запускает их.
set -e
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build build --parallel
./build/espresso_tests
