# Макросы, которые составляют списки каталогов компонента ESP-IDF по
# дереву файлов. Списки строятся, когда CMake настраивает проект, поэтому
# после добавления файлов нужно выполнить idf.py reconfigure.

# esp_inc_dirs(root return_list): каталог root и все его непустые
# подкаталоги — для параметра INCLUDE_DIRS.
macro(esp_inc_dirs root return_list)
    unset(file_list)
    unset(dir_list)
    file(
        GLOB_RECURSE file_list
        LIST_DIRECTORIES true
        "${root}/*"
    )
    foreach (file_path ${file_list})
        get_filename_component(dir_path ${file_path} DIRECTORY)
        set(dir_list ${dir_list} ${dir_path})
    endforeach()
    list(REMOVE_DUPLICATES dir_list)
    set(${return_list} ${dir_list})
endmacro()

# esp_src_dirs(root return_list): каталоги внутри root, в которых есть
# файлы .c, .cpp или .cxx, — для параметра SRC_DIRS.
macro(esp_src_dirs root return_list)
    unset(file_list)
    unset(dir_list)
    file(
        GLOB_RECURSE file_list
        LIST_DIRECTORIES false
        "${root}/*"
    )
    foreach (file_path ${file_list})
        get_filename_component(ext ${file_path} LAST_EXT)
        if(ext STREQUAL ".c" OR ext STREQUAL ".cpp" OR ext STREQUAL ".cxx")
            get_filename_component(dir_path ${file_path} DIRECTORY)
            set(dir_list ${dir_list} ${dir_path})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES dir_list)
    set(${return_list} ${dir_list})
endmacro()
