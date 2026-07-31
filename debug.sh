#!/bin/bash

mkdir -p build_debug

cmake -S . -B build_debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build_debug -j$(nproc)
