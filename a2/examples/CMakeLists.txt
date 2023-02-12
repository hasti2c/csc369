function(add_example example_name example_sources)
  add_executable(${example_name} ${example_sources})

  target_link_libraries(
      ${example_name}
      PRIVATE
        CSC369::a2_thread
  )

  # Require the C11 standard.
  set_target_properties(
      ${example_name}
      PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
  )

  target_compile_options(
      ${example_name}
      PRIVATE
        -Wall
  )
endfunction()

add_example(hot_potato hot_potato.c)
add_example(spin_and_join spin_and_join.c)

#add_executable(hot_potato hot_potato.c)
#
#target_link_libraries(
#    hot_potato
#    PRIVATE
#      CSC369::a2_thread
#)
#
#add_executable(spin_and_join spin_and_join.c)
#
#target_link_libraries(
#    spin_and_join
#    PRIVATE
#      CSC369::a2_thread
#)
