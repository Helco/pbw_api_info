cmake_minimum_required (VERSION 2.6)
project (pbw_app_info)

###################################################################
# Utilities
###################################################################

set (CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

###################################################################
# pbl_api_info
###################################################################

include_directories("thirdparty/elfio/")

set(sources_miniz
  thirdparty/miniz/miniz.c
  thirdparty/miniz/miniz_tdef.c
  thirdparty/miniz/miniz_tinfl.c
  thirdparty/miniz/miniz_zip.c
)
assign_source_group(${sources_miniz})

set(sources_pbw_api_info
  src/ArArchive.cpp
  src/PblAppArchive.cpp
  src/PblAppBinary.cpp
  src/PblLibrary.cpp
  src/main.cpp
)
assign_source_group(${sources_pbw_api_info})

add_executable(pbw_api_info
  ${sources_miniz}
  ${sources_pbw_api_info}
)
