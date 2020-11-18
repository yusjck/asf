
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rd/libpng
        ${CMAKE_BINARY_DIR}/3rd/libpng
        ${CMAKE_SOURCE_DIR}/3rd/libevent/include
        ${CMAKE_BINARY_DIR}/3rd/libevent/include
        ${CMAKE_SOURCE_DIR}/base
        ${CMAKE_SOURCE_DIR}/3rd/libevdev/include
        ${CMAKE_SOURCE_DIR}/3rd/libevdev/source/libevdev
        ${CMAKE_SOURCE_DIR}/3rd/minicap-shared/aosp/include)

add_library(
        control
        STATIC
        simpleocr.cpp
        colorassist.cpp
        inputassist.cpp
        minitouch.cpp
        pluginmgr.cpp
        inject.cpp
        processbinder.cpp
        screenreader.cpp
        virtdisplayreader.cpp
        screenreaderbyfb.cpp
        deviceinfo.cpp
        androidcaller.cpp
        controller.cpp)

add_library(minicap SHARED IMPORTED)
set_target_properties(minicap PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/3rd/minicap-shared/aosp/libs/android-19/${ANDROID_ABI}/minicap.so)

target_link_libraries(
        control
        base
        libevdev
        png_static
        z)
