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
