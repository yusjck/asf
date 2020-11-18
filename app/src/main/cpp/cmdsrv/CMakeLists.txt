
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include
        ${CMAKE_SOURCE_DIR}/base
        ${CMAKE_SOURCE_DIR}/control)

add_library(
        cmdsrv
        STATIC
        cmdcontext.cpp
        cmdserver.cpp
        cmddispatcher.cpp)

target_link_libraries(
        cmdsrv
        base
        control)
