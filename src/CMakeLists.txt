# Вся программа — один компонент ESP-IDF: каталог src со всеми
# подкаталогами, в том числе сторонние библиотеки (3rdparty) и таблицы
# шрифта (fonts). Каталоги заголовков и исходников собираются по дереву
# файлов макросами из cmake/tools.cmake, когда CMake настраивает проект;
# новые файлы подхватываются после idf.py reconfigure.
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/tools.cmake")

esp_inc_dirs("${CMAKE_CURRENT_LIST_DIR}" esp_inc_dir_list)
esp_src_dirs("${CMAKE_CURRENT_LIST_DIR}" esp_src_dir_list)

# Компоненты ESP-IDF, которыми пользуется программа. Каталога main в
# проекте нет, поэтому ESP-IDF не подключает их сам. Заголовок
# tools/esp_tools.h берёт из esp_wifi.h номер ядра задачи Wi-Fi, чтобы
# занять под видеовывод другое ядро.
idf_component_register(INCLUDE_DIRS ${esp_inc_dir_list}
                       SRC_DIRS ${esp_src_dir_list}
                       PRIV_REQUIRES efuse
                                     esp_app_format
                                     esp_driver_gpio
                                     esp_driver_sdm
                                     esp_driver_sdspi
                                     esp_driver_spi
                                     esp_timer
                                     esp_wifi
                                     fatfs
                                     sdmmc
                                     spi_flash
                                     ulp)

# Отладочный вывод: вызовы ESP_LOGD компилируются при любом значении
# CONFIG_LOG_MAXIMUM_LEVEL, а включает их во время работы app/esp_app.cpp —
# в сборках с проверками (без NDEBUG).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LOG_LOCAL_LEVEL=ESP_LOG_DEBUG)

# Код, полученный из FabGL (gfx, vga, ps2, sound и tools/esp_tools.cpp),
# отлажен с оптимизацией по размеру (-Os), как в ядре Arduino и в порте
# FabGL; самые нагруженные файлы к тому же сами включают -O2 или -O3.
# Обработчики прерываний видеовывода должны успевать подготовить строки к
# выводу, поэтому этот код собирается с -Os и при отладочной сборке
# проекта (-Og).
file(GLOB esp_size_optimized_sources
     "${CMAKE_CURRENT_LIST_DIR}/gfx/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/vga/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/ps2/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/sound/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/tools/esp_tools.cpp")
set_source_files_properties(${esp_size_optimized_sources} PROPERTIES COMPILE_OPTIONS -Os)

# yyjson (проверено на версиях 0.9.0 и 0.13.0) вызывает у GCC 15 при -Og
# ложные предупреждения -Wmaybe-uninitialized, а ESP-IDF собирает
# с -Werror, то есть считает их ошибками.
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/3rdparty/yyjson/yyjson.c"
                            PROPERTIES COMPILE_OPTIONS -Wno-maybe-uninitialized)
