
add_library(logger_lib STATIC
    logger/logger.hpp
    logger/logger.cpp
)
target_include_directories(logger_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)


add_library(time_lib STATIC
    time_formatting.cpp
)
target_include_directories(time_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)


add_library(messenger_lib STATIC
    messaging/messaging.hpp
    messaging/messaging.cpp
)
target_link_libraries(messenger_lib PUBLIC
    logger_lib
    nlohmann_json
)
target_include_directories(messenger_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)



add_subdirectory(server)
add_subdirectory(client)