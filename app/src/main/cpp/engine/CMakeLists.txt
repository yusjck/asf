
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libpng
        ${CMAKE_BINARY_DIR}/3rd/libpng
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include
        ${CMAKE_SOURCE_DIR}/3rd/lua
        ${CMAKE_SOURCE_DIR}/base
        ${CMAKE_SOURCE_DIR}/cmdsrv)

add_library(
        engine
        SHARED
        main.cpp
        engineservice.cpp
        luastub.cpp
        luamain.cpp
        lualib_color.cpp
        lualib_event.cpp
        lualib_input.cpp
        lualib_memory.cpp
        lualib_misc.cpp
        lualib_plugin.cpp
        lualib_script.cpp
        lualib_bit.cpp
        lualib_file.cpp
        lualib_int64.cpp
        lualib_struct.cpp
        logfile.cpp
        cmdbridge.cpp
        pluginproxy.cpp)

target_link_libraries(
        engine
        cmdsrv
        control
        base
        cjson
        lua
        png_static
        z)
