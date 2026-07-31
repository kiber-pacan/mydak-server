# Set cmake version
cmake_minimum_required(VERSION 4.3)

# Setting c++ 26 standard
set(CMAKE_CXX_STANDARD 26)

# Declaring project
project(mydak-server)


include(FetchContent)

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME}
  core/main.cpp
  core/proto.hpp
  
  core/connection.cpp
  core/connection.hpp
  
  core/server.cpp
  core/server.hpp

  core/client/client.cpp
  core/client/client.hpp
  
  core/util/optional_ref.hpp
  core/util/slot.hpp
  core/util/slot_vector.hpp

  core/util/logger.hpp
  core/util/logger.cpp
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/core/
  ${CMAKE_CURRENT_SOURCE_DIR}/core/util
  ${CMAKE_CURRENT_SOURCE_DIR}/core/client
)



# Dependencies
include(CPM.cmake)


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

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
  sodium
  tomlplusplus::tomlplusplus
  OpenSSL::Crypto
  OpenSSL::SSL
  Boost::headers
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
