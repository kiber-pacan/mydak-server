#!/bin/bash

mkdir -p build_debug

cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug

cmake --build build_debug -j$(nproc)
