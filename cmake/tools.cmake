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
