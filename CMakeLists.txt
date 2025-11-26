cmake_minimum_required(VERSION 3.10)

project(Programming_Project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra)
endif()

# Server target
add_executable(server
    Server.c
    Data_Store.c
)

# Client target
add_executable(client
    Client.c
)

# Threads for server
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

# Headers are in the project root
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
