include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/tools.cmake")

esp_inc_dirs(src esp_yyjson_inc_dirs)
esp_src_dirs(src esp_yyjson_src_dirs)

idf_component_register(INCLUDE_DIRS ${esp_yyjson_inc_dirs}
                       SRC_DIRS ${esp_yyjson_src_dirs})

# yyjson (checked with 0.9.0 and 0.13.0) trips false -Wmaybe-uninitialized
# warnings under GCC 15 at -Og, and ESP-IDF builds with -Werror.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-maybe-uninitialized)
