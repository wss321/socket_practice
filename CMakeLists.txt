cmake_minimum_required(VERSION 3.21)
project(socket_practice)

set(CMAKE_CXX_STANDARD 11)

add_executable(server server.cpp)
add_executable(client client.cpp)
