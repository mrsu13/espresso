# Часть библиотеки FabGL: вывод на VGA, клавиатура и мышь PS/2, звук.
# Источник, версия и изменения относительно оригинала указаны в файле
# INFO.md. Нужные FabGL настройки ESP-IDF — в sdkconfig.defaults проекта.
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/tools.cmake")

esp_inc_dirs(src esp_fabgl_inc_dirs)
esp_src_dirs(src esp_fabgl_src_dirs)

# В REQUIRES — компоненты, заголовки которых включают заголовки FabGL;
# в PRIV_REQUIRES — те, что нужны только исходным текстам. Заголовок
# fabglconf.h берёт из esp_wifi.h номер ядра задачи Wi-Fi, чтобы занять
# под видеовывод другое ядро.
idf_component_register(INCLUDE_DIRS ${esp_fabgl_inc_dirs}
                       SRC_DIRS ${esp_fabgl_src_dirs}
                       REQUIRES esp_driver_gpio esp_driver_sdm esp_timer esp_wifi ulp
                       PRIV_REQUIRES spi_flash)

# FabGL отлажена с оптимизацией по размеру (-Os), как в ядре Arduino и в
# конфигурации порта; самые нагруженные модули к тому же сами включают -O2
# или -O3. Обработчики прерываний видеовывода должны успевать подготовить
# строки к выводу, поэтому библиотека собирается с -Os и при отладочной
# сборке проекта (-Og).
target_compile_options(${COMPONENT_LIB} PRIVATE -Os)
