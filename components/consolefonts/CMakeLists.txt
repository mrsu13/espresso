# Растровый шрифт консоли Linux Terminus 10 × 18 для FabGL, индексированный
# по кодам Unicode. Источник, лицензия и способ получения указаны в INFO.md.
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/tools.cmake")

esp_inc_dirs(src esp_consolefonts_inc_dirs)
esp_src_dirs(src esp_consolefonts_src_dirs)

idf_component_register(INCLUDE_DIRS ${esp_consolefonts_inc_dirs}
                       SRC_DIRS ${esp_consolefonts_src_dirs})
