
cmake_minimum_required(VERSION 3.4.1)

add_library(
        libevdev
        STATIC

        source/libevdev/libevdev.c
        source/libevdev/libevdev-names.c)

include_directories(
        include
        source/include
        source/libevdev)
