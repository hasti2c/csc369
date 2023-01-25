add_executable(handout_example handout_example.c)

target_link_libraries(
    handout_example
    PRIVATE
        CSC369::thread
)
