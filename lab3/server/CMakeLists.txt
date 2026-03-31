add_library(thread_pool_lib STATIC
    thread_pool/thread_pool.hpp
    thread_pool/thread_pool.cpp
)

add_library(server_lib STATIC
    server.cpp
    server.hpp
)
target_link_libraries(server_lib PUBLIC
    messenger_lib
    thread_pool_lib
)

add_executable(server
    main.cpp
)
target_link_libraries(server PUBLIC
    server_lib
)
