cmake_minimum_required(VERSION 3.10)
project(TinyKV C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -pthread")

add_executable(server Server.c Data_Store.c)
add_executable(client Client.c)
