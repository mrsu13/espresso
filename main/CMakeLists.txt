include("${CMAKE_CURRENT_LIST_DIR}/../cmake/tools.cmake")

# Каталоги заголовков и исходников собираются по дереву src макросами
# из cmake/tools.cmake; новые файлы подхватываются после idf.py reconfigure.
esp_inc_dirs(src esp_main_inc_dirs)
esp_src_dirs(src esp_main_src_dirs)

# REQUIRES — компоненты, чьи заголовки включаются в заголовки модуля main;
# PRIV_REQUIRES — компоненты, нужные только его исходным файлам.
idf_component_register(INCLUDE_DIRS ${esp_main_inc_dirs}
                       SRC_DIRS ${esp_main_src_dirs}
                       REQUIRES esp_driver_gpio
                                fabgl
                                sdmmc
                       PRIV_REQUIRES consolefonts
                                     efuse
                                     esp_app_format
                                     esp_driver_sdspi
                                     esp_driver_spi
                                     fatfs
                                     spi_flash
                                     utf8
                                     yyjson)

# Отладочный вывод: вызовы ESP_LOGD модуля main компилируются при любом
# значении CONFIG_LOG_MAXIMUM_LEVEL, а включает их во время работы
# esp_app.cpp — в сборках с проверками (без NDEBUG).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LOG_LOCAL_LEVEL=ESP_LOG_DEBUG)
