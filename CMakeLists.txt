cmake_minimum_required(VERSION 3.16)
project(peregrine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra")

find_package(nlohmann_json)
find_package(cppzmq)

add_subdirectory(src)
