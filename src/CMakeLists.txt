add_library(
    peregrine STATIC
        logger.cpp
)
target_include_directories(
    peregrine
        PUBLIC ../include
        PRIVATE .
)
target_link_libraries(
    peregrine
        nlohmann_json::nlohmann_json
        cppzmq
        fmt::fmt
)

add_library(
    peregrine_wrapper STATIC
        logger_c.cpp
)
target_include_directories(
    peregrine_wrapper
        PUBLIC ../include
        PRIVATE .
)
target_link_libraries(
    peregrine_wrapper
        peregrine
)
