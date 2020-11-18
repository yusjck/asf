
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include)

add_library(
        base
        STATIC
        ipcclient.cpp
        ipcserver.cpp
        ipcconnection.cpp
        msgchannel.cpp
        common.cpp
        lz77.c
        base64.c
        crypt.c
        crc32.c
        fileutil.cpp
        strutil.cpp
        configutil.cpp)

target_link_libraries(
        base
        event_extra
        event_core
        log)
