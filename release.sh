#!/bin/bash

mkdir -p build_release

cmake -S . -B build_release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build_release -j$(nproc)
