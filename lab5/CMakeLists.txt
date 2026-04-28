cmake_minimum_required(VERSION 3.14)
project(NetworkLabs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE "Debug")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "-g")
endif()

add_library(logger_lib STATIC
    logger/logger.hpp
    logger/logger.cpp
)
target_include_directories(logger_lib PUBLIC ${CMAKE_SOURCE_DIR})


add_library(messenger_lib STATIC
    messaging/messaging.hpp
    messaging/messaging.cpp
)
target_link_libraries(messenger_lib PUBLIC
    logger_lib
)
target_include_directories(messenger_lib PUBLIC ${CMAKE_SOURCE_DIR})

add_subdirectory(server)
add_subdirectory(client)