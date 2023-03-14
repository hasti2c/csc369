set(CSC369_A3_EXE sim)

add_executable(
    ${CSC369_A3_EXE}
    clock.c
    list.h
    lru.c
    pagetable.c
    pagetable.h
    pagetable_generic.h
    rand.c
    rr.c
    sim.c
    sim.h
    swap.c
    swap.h
    timer.h
)

# Require the C11 standard.
set_target_properties(
    ${CSC369_A3_EXE}
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
)

target_compile_options(
    ${CSC369_A3_EXE}
    PRIVATE
      -g3 -Wall -Wextra -Werror -MMD
)
