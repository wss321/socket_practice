cmake_minimum_required(VERSION 3.21)
project(socket_practice)

set(CMAKE_CXX_STANDARD 11)

add_executable(server server.cpp)
add_executable(client client.cpp)

add_executable(multi_thread_server multi_thread_server.cpp)
target_link_libraries(multi_thread_server -lpthread)

add_executable(multi_process_server multi_process_server.cpp)
add_executable(select_server select_server.cpp)
add_executable(poll_server poll_server.cpp)
add_executable(epoll_server epoll_server.cpp)