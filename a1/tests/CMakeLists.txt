find_path(CHECK_INCLUDE_DIR
    NAMES
        check.h
    PATHS
        /u/csc369h/winter/pub/dep/include
)

find_library(CHECK_LIB check
    PATHS
        /u/csc369h/winter/pub/dep/lib
)

if(NOT CHECK_LIB OR NOT CHECK_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find the check library.")
else()
    message("Found check library: ${CHECK_LIB}")
    message("Found check include directory: ${CHECK_INCLUDE_DIR}")

    add_library(Check::check UNKNOWN IMPORTED)
    set_target_properties(
        Check::check PROPERTIES
        IMPORTED_LOCATION "${CHECK_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CHECK_INCLUDE_DIR}"
    )
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CSC369_A1_TESTS_EXE ${PROJECT_NAME}_tests)
add_executable(
    ${CSC369_A1_TESTS_EXE}
    check_thread.c
)

target_link_libraries(
    ${CSC369_A1_TESTS_EXE}
    PRIVATE
        CSC369::thread
        Check::check
        Threads::Threads
)

# Require the C11 standard.
set_target_properties(
    ${CSC369_A1_TESTS_EXE}
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
)
