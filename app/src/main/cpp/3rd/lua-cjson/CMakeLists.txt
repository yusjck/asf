
cmake_minimum_required(VERSION 2.6)

add_definitions("-DENABLE_CJSON_GLOBAL")

include_directories(${CMAKE_SOURCE_DIR}/3rd/lua)

add_library(cjson STATIC lua_cjson.c strbuf.c fpconv.c)
