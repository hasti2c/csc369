cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(
    csc369_a4
    VERSION 20231
    LANGUAGES C
)

# Globally set that all executables be compiled into a 'bin' directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
