# test binary
add_executable(
    test
        test.cpp
)
target_include_directories(
    test
        PUBLIC ../include
        PRIVATE .
)
target_link_libraries(
    test
        peregrine
)

# wrapper_test binary
add_executable(
    wrapper_test
        wrapper_test/client.c
        wrapper_test/client.cpp
)
target_include_directories(
    wrapper_test
        PUBLIC ../include
        PRIVATE .
)
target_link_libraries(
    wrapper_test
        peregrine_wrapper
)
