cmake_minimum_required(VERSION 3.20)
project(ezbitstream)

set(CMAKE_CXX_STANDARD 14)

add_library(ezbitstream SHARED
        bitstream8.cpp
        bitstream16.cpp
        bitstream32.cpp
        bitstream64.cpp
        bitstream8.h
        bitstream16.h
        bitstream32.h
        bitstream64.h
        ezbitstream.h
        tables.h)

add_library(ezbitstream_static STATIC
        bitstream8.cpp
        bitstream16.cpp
        bitstream32.cpp
        bitstream64.cpp
        bitstream8.h
        bitstream16.h
        bitstream32.h
        bitstream64.h
        ezbitstream.h
        tables.h)

target_include_directories(
        ezbitstream_static PUBLIC ${CMAKE_SOURCE_DIR}/
)
target_include_directories(
        ezbitstream        PUBLIC ${CMAKE_SOURCE_DIR}/
)
