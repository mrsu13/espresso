# Обязательная часть проекта ESP-IDF: эти строки должны идти именно
# в таком порядке.
cmake_minimum_required(VERSION 3.22)

# Вся программа — один компонент ESP-IDF, каталог src (src/CMakeLists.txt);
# каталогов main и components в проекте нет. Каталоги компонентов задаются
# до включения project.cmake.
set(EXTRA_COMPONENT_DIRS src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espresso)
