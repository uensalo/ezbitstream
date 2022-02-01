cmake_minimum_required(VERSION 3.20)
project(ezbitstream)

set(CMAKE_CXX_STANDARD 14)

add_library(ezbitstream        SHARED ezbitstream.cpp ezbitstream.h)
add_library(ezbitstream_static STATIC ezbitstream.cpp ezbitstream.h)

target_include_directories(
        ezbitstream_static PUBLIC ${CMAKE_SOURCE_DIR}/
)
target_include_directories(
        ezbitstream        PUBLIC ${CMAKE_SOURCE_DIR}/
)

add_executable(exe main.cpp)
target_link_libraries(exe PRIVATE ezbitstream)
