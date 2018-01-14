cmake_minimum_required (VERSION 2.6)
project (pbw_app_info)

set (CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories("thirdparty/elfio/")

add_executable(pbw_api_info
  # Third-Party
  thirdparty/miniz/miniz.c
  thirdparty/miniz/miniz_tdef.c
  thirdparty/miniz/miniz_tinfl.c
  thirdparty/miniz/miniz_zip.c

  # pbw_app_info
  src/ArArchive.cpp
  src/PblAppArchive.cpp
  src/PblAppBinary.cpp
  src/PblLibrary.cpp
  src/main.cpp
)

