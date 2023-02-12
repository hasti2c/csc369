set(CSC369_A2_THREAD_LIB ${PROJECT_NAME})

add_library(
  ${CSC369_A2_THREAD_LIB}
  csc369_interrupts.h
  csc369_interrupts.c
  csc369_thread.h
  csc369_thread.c
)

add_library(CSC369::a2_thread ALIAS ${CSC369_A2_THREAD_LIB})

target_include_directories(
  ${CSC369_A2_THREAD_LIB}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Require the C11 standard.
set_target_properties(
  ${CSC369_A2_THREAD_LIB}
  PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
)

target_compile_options(
    ${CSC369_A2_THREAD_LIB}
    PRIVATE
      -D_GNU_SOURCE -Wall -Wextra
)
