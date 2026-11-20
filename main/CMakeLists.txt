include("${CMAKE_CURRENT_LIST_DIR}/../cmake/tools.cmake")

esp_inc_dirs(src esp_main_inc_dirs)
esp_src_dirs(src esp_main_src_dirs)

idf_component_register(INCLUDE_DIRS ${esp_main_inc_dirs}
                       SRC_DIRS ${esp_main_src_dirs}
                       REQUIRES esp_driver_gpio
                                esp_lcd
                                sdmmc
                       PRIV_REQUIRES efuse
                                     esp_app_format
                                     esp_driver_sdspi
                                     esp_driver_spi
                                     fatfs
                                     spi_flash
                                     yyjson)

# Debug output: compile the application's ESP_LOGD calls in, whatever
# CONFIG_LOG_MAXIMUM_LEVEL says; esp_app.cpp enables them at run time
# in builds with assertions enabled (no NDEBUG).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LOG_LOCAL_LEVEL=ESP_LOG_DEBUG)
