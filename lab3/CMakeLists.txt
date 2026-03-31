cmake_minimum_required(VERSION 3.14)
project(NetworkLabs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE "Debug")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "-g")
endif()

add_library(messenger_lib STATIC
    messaging.hpp
    messaging.cpp
)

add_subdirectory(server)
add_subdirectory(client)