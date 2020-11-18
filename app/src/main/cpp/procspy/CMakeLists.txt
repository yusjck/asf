
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include
        ${CMAKE_SOURCE_DIR}/base)

add_library(
        procspy
        SHARED
        memassist.cpp
        main.cpp)

target_link_libraries(
        procspy
        base)
