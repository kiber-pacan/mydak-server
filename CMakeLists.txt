# Set cmake version
cmake_minimum_required(VERSION 3.28)

# Setting c++ 26 standard
set(CMAKE_CXX_STANDARD 26)
set(DCMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

# LLVM
#set(CMAKE_CXX_COMPILER_ID LLVM)
#set(CMAKE_C_COMPILER_ID LLVM)

# Declaring project
project(mydak-websocket)
cmake_policy(SET CMP0167 OLD)

include(FetchContent)

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME}
  core/main.cpp
  
  core/Connection.cpp
  core/Connection.hpp
  
  core/Server.cpp
  core/Server.hpp
)

# For using in c++ code
#target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
#        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
#	  )

# Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})


# Dependencies
include(CPM.cmake)

# ASIO
CPMAddPackage(
  NAME asio_cmake
  GIT_REPOSITORY "https://github.com/OlivierLDff/asio.cmake"
  GIT_TAG "main"
)

# LIBSODIUM
CPMAddPackage(
  NAME libsodium_cmake
  GIT_REPOSITORY "https://github.com/robinlinden/libsodium-cmake.git"
  GIT_TAG "cfebfd3da486d5a86c644c8b47067e5411c7599c"
)

# TOML++
CPMAddPackage(
  NAME tomlplusplus_cmake
  GIT_REPOSITORY "https://github.com/marzer/tomlplusplus.git"
  GIT_TAG "v3.4.0"
)



target_link_libraries(${EXECUTABLE_NAME} PUBLIC
  asio::asio
  sodium
  tomlplusplus::tomlplusplus
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
