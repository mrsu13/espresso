# Сторонняя библиотека yyjson для разбора и записи JSON; версия и
# источник указаны в файле INFO.
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/tools.cmake")

esp_inc_dirs(src esp_yyjson_inc_dirs)
esp_src_dirs(src esp_yyjson_src_dirs)

idf_component_register(INCLUDE_DIRS ${esp_yyjson_inc_dirs}
                       SRC_DIRS ${esp_yyjson_src_dirs})

# yyjson (проверено на версиях 0.9.0 и 0.13.0) вызывает у GCC 15 при -Og
# ложные предупреждения -Wmaybe-uninitialized, а ESP-IDF собирает
# с -Werror, то есть считает их ошибками.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-maybe-uninitialized)
