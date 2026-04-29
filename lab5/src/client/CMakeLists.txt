
add_library(client_lib STATIC
    client.cpp
    client.hpp
)
target_link_libraries(client_lib PUBLIC
    messenger_lib
    time_lib
)

add_executable(client
    main.cpp
)
target_link_libraries(client PUBLIC
    client_lib
)
