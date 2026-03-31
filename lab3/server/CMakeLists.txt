add_library(client_session_lib STATIC
    client_session.hpp
    client_session.cpp
)
target_link_libraries(client_session_lib PUBLIC
    messenger_lib
)

add_library(session_registry_lib STATIC
    session_registry.hpp
    session_registry.cpp
)
target_link_libraries(session_registry_lib PUBLIC
    client_session_lib
)

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
    client_session_lib
    session_registry_lib
)
