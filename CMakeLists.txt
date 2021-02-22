cmake_minimum_required(VERSION 3.4.1)

project(1808_debug)

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

set(link_libs   rknn_api
		pthread
		dl
)

add_executable(1808_debug
        src/tcp_comm.cpp
        src/server_text_detection.cpp
        )

target_link_libraries(1808_debug ${link_libs})
