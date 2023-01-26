#!/usr/bin/bash
set -e
conan install -if conan .
mkdir -p build
pushd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan/conan_toolchain.cmake -DCMAKE_BUILD_TYPE="Release"
popd
