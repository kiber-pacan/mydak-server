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
        core/util/proto.hpp

        core/main/connection.cpp
        core/main/connection.hpp

        core/main/server.cpp
        core/main/server.hpp

        core/main/client.cpp
        core/main/client.hpp
  
        core/util/optional_ref.hpp
        core/util/slot.hpp
        core/util/slot_vector.hpp

        core/util/logger.hpp
        core/util/logger.cpp
        core/database/database.cpp
        core/database/database.hpp
        core/parameters/parameters.hpp
        core/parameters/parameters.cpp
        core/util/tools.hpp
        core/util/tools.hpp
        core/util/indices.hpp
        core/util/indices.hpp
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core/
        ${CMAKE_CURRENT_SOURCE_DIR}/core/database
        ${CMAKE_CURRENT_SOURCE_DIR}/core/main
        ${CMAKE_CURRENT_SOURCE_DIR}/core/parameters
        ${CMAKE_CURRENT_SOURCE_DIR}/core/util
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
find_package(nlohmann_json REQUIRED)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        sodium
        tomlplusplus::tomlplusplus
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        OpenSSL::SSL
        Boost::headers
        boost_charconv
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

