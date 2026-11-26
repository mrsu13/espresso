# Проверки на ПК. Части терминала esp::vt, которые не зависят от ESP-IDF,
# собираются обычным компилятором вместе с наборами проверок из этого
# каталога; запуск — test/run.sh.
cmake_minimum_required(VERSION 3.20)
project(espresso_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(esp_src_dir "${CMAKE_CURRENT_LIST_DIR}/../src")

# Из программы берутся только файлы, не зависящие от ESP-IDF.
add_executable(espresso_tests
               main.cpp
               vt/esp_vt_input_parser_test.cpp
               vt/esp_vt_keys_test.cpp
               vt/esp_vt_line_edit_test.cpp
               vt/esp_vt_line_history_test.cpp
               vt/esp_vt_parser_test.cpp
               vt/esp_vt_scrollback_test.cpp
               vt/esp_vt_screen_test.cpp
               "${esp_src_dir}/vt/esp_vt_command.cpp"
               "${esp_src_dir}/vt/esp_vt_input_parser.cpp"
               "${esp_src_dir}/vt/esp_vt_keys.cpp"
               "${esp_src_dir}/vt/esp_vt_line_edit.cpp"
               "${esp_src_dir}/vt/esp_vt_line_history.cpp"
               "${esp_src_dir}/vt/esp_vt_parser.cpp"
               "${esp_src_dir}/vt/esp_vt_screen.cpp"
               "${esp_src_dir}/vt/esp_vt_scrollback.cpp"
               "${esp_src_dir}/vt/esp_vt_utf8.cpp")

target_include_directories(espresso_tests PRIVATE
                           "${CMAKE_CURRENT_LIST_DIR}"
                           "${CMAKE_CURRENT_LIST_DIR}/vt"
                           "${esp_src_dir}/vt"
                           "${esp_src_dir}/ps2"
                           "${esp_src_dir}/tools")

# Предупреждения — ошибки, как в сборке ESP-IDF; выход за границы памяти и
# неопределённое поведение ловят санитайзеры.
target_compile_options(espresso_tests PRIVATE
                       -Wall -Wextra -Werror
                       -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(espresso_tests PRIVATE -fsanitize=address,undefined)
