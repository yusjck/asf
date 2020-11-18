
cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(3rd/libpng)
add_subdirectory(3rd/libevent)
add_subdirectory(3rd/lua)
add_subdirectory(3rd/lua-cjson)
add_subdirectory(3rd/libevdev)

add_subdirectory(base)
add_subdirectory(control)
add_subdirectory(cmdsrv)
add_subdirectory(engine)
add_subdirectory(cmdhost)
add_subdirectory(procspy)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include
        ${CMAKE_SOURCE_DIR}/base)

add_library(
        native-lib
        SHARED
        enginebinder.cpp
        androidcallservice.cpp
        native-lib.cpp)

target_link_libraries(
        native-lib
        base)
