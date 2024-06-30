cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(FetchContent)

project(external-connectivity)

FetchContent_Declare(magic_enum GIT_REPOSITORY https://github.com/Neargye/magic_enum.git)
FetchContent_MakeAvailable(magic_enum)
