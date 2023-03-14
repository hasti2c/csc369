function(add_benchmark benchmark_name benchmark_source)
  add_executable(${benchmark_name} ${benchmark_source} timer.h marker.h)

  target_compile_options(
      ${benchmark_name}
      PRIVATE
        -g3 -Wall -Wextra -Werror
  )
endfunction()

add_benchmark(simpleloop simpleloop.c)
add_benchmark(repeatloop repeatloop.c)
add_benchmark(blocked blocked.c)
add_benchmark(matmul matmul.c)

#NOTE: All programs except simpleloop are compiled with -O1 so that
#      local variables in their loops are stored in CPU registers.
#      Otherwise the results for some of the replacement algorithms
#      are skewed due to the large number of stack variable accesses.
target_compile_options(
    repeatloop
    PRIVATE
      -O1
)

target_compile_options(
    blocked
    PRIVATE
      -O1
)

target_compile_options(
    matmul
    PRIVATE
      -O1
)
