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
)
