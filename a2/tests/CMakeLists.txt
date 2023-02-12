function(add_check_exe check_exe_name check_exe_sources)
    add_executable(${check_exe_name} ${check_exe_sources})

    target_link_libraries(
        ${check_exe_name}
        PRIVATE
            CSC369::a2_thread
            Check::check
            Threads::Threads
    )

    # Require the C11 standard.
    set_target_properties(
        ${check_exe_name}
        PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
    )

    target_compile_options(
        ${check_exe_name}
        PRIVATE
            -Wall
    )
endfunction()


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

add_check_exe(check_a2_thread_a1 check_a2_thread_a1.c)
add_check_exe(check_a2_thread check_a2_thread.c)
