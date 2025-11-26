cmake_minimum_required(VERSION 3.10)

project(TinyKV C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Turn on warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra)
endif()

# Server target
add_executable(server
    server.c
    kvstore.c
)

# Client target
add_executable(client
    client.c
)

# Threads (for server)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

# So both can include "common.h" and "kvstore.h" from the current dir
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
