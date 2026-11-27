# Вся программа — один компонент ESP-IDF: каталог src со всеми
# подкаталогами, в том числе сторонние библиотеки (3rdparty) и таблицы
# шрифта (fonts). Каталоги заголовков и исходников собираются по дереву
# файлов макросами из cmake/tools.cmake, когда CMake настраивает проект;
# новые файлы подхватываются после idf.py reconfigure. Движок ff
# (3rdparty/ff) в списки не входит: в его дереве есть тесты, примеры и
# свои сторонние библиотеки, а собирается он своим CMakeLists.txt (ниже).
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/tools.cmake")

set(esp_ff_dir "${CMAKE_CURRENT_LIST_DIR}/3rdparty/ff")
esp_inc_dirs("${CMAKE_CURRENT_LIST_DIR}" esp_inc_dir_list "${esp_ff_dir}")
esp_src_dirs("${CMAKE_CURRENT_LIST_DIR}" esp_src_dir_list "${esp_ff_dir}")

# Компоненты ESP-IDF, которыми пользуется программа. Каталога main в
# проекте нет, поэтому ESP-IDF не подключает их сам.
idf_component_register(INCLUDE_DIRS ${esp_inc_dir_list}
                       SRC_DIRS ${esp_src_dir_list}
                       PRIV_REQUIRES efuse
                                     esp_app_format
                                     esp_driver_gpio
                                     esp_driver_sdm
                                     esp_driver_sdspi
                                     esp_driver_spi
                                     esp_driver_uart
                                     esp_timer
                                     fatfs
                                     sdmmc
                                     spi_flash
                                     ulp)

# Отладочный вывод: вызовы ESP_LOGD компилируются при любом значении
# CONFIG_LOG_MAXIMUM_LEVEL, а включает их во время работы app/esp_app.cpp —
# в сборках с проверками (без NDEBUG).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LOG_LOCAL_LEVEL=ESP_LOG_DEBUG)

# Движок ff — подмодуль git; собирается своим CMakeLists.txt как
# подпроект, статической библиотекой ff::ff (раздел 7.1 документа об
# устройстве). Параметры ff задаются через кэш до add_subdirectory(): ff
# объявляет их через option(), а значение из кэша сильнее значения по
# умолчанию. Заголовок ff.h есть и у FatFs, поэтому программа включает
# главный заголовок ff как <ff/src/ff.h> — по пути от каталога
# 3rdparty, который есть среди путей заголовков компонента.
set(FF_32BIT ON CACHE BOOL "ff: 32-разрядные ячейки и вещественные числа одинарной точности." FORCE)
set(FF_SAFE_MEM OFF CACHE BOOL "ff: без проверки адресов — как в настоящем ретрокомпьютере." FORCE)
set(FF_R_TRUSTED ON CACHE BOOL "ff: без лишних проверок стека возвратов в циклах — на 5 % быстрее." FORCE)
set(FF_WITH_SYSTEM OFF CACHE BOOL "ff: без слова system — на плате ему нечего запускать." FORCE)
set(FF_WITH_FILES ON CACHE BOOL "ff: файловые слова и load — для файлов на SD-карте." FORCE)
set(FF_WITH_TR ON CACHE BOOL "ff: тексты ff на русском и английском." FORCE)
set(FF_PAD_INIT_SIZE 2048 CACHE STRING "ff: кусок временной области строк — 2 КБ: 32 КБ были бы половиной памяти Форта." FORCE)
set(FF_DICT_ARENA_SLAB 4096 CACHE STRING "ff: кусок памяти слов — 4 КБ: кусок в 64 КБ занял бы всю память Форта, и одним куском его в куче нет." FORCE)
set(FF_BUILD_SHARED OFF CACHE BOOL "ff: только статическая библиотека." FORCE)
add_subdirectory("${esp_ff_dir}" ff)
target_link_libraries(${COMPONENT_LIB} PRIVATE ff::ff)

# Код, полученный из FabGL (gfx, vga, ps2, sound и в tools — esp_tools,
# esp_core_usage, esp_current_video_mode), отлажен с оптимизацией по
# размеру (-Os), как в ядре Arduino и в порте FabGL; самые нагруженные
# файлы к тому же сами включают -O2 или -O3.
# Обработчики прерываний видеовывода должны успевать подготовить строки к
# выводу, поэтому этот код собирается с -Os и при отладочной сборке
# проекта (-Og).
file(GLOB esp_size_optimized_sources
     "${CMAKE_CURRENT_LIST_DIR}/gfx/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/vga/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/ps2/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/sound/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/tools/esp_core_usage.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/tools/esp_current_video_mode.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/tools/esp_tools.cpp")
set_source_files_properties(${esp_size_optimized_sources} PROPERTIES COMPILE_OPTIONS -Os)

# yyjson (проверено на версиях 0.9.0 и 0.13.0) вызывает у GCC 15 при -Og
# ложные предупреждения -Wmaybe-uninitialized, а ESP-IDF собирает
# с -Werror, то есть считает их ошибками.
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/3rdparty/yyjson/yyjson.c"
                            PROPERTIES COMPILE_OPTIONS -Wno-maybe-uninitialized)
