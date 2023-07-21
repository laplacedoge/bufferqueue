cmake_minimum_required(VERSION 3.15)

project(bque LANGUAGES C)

include_directories(${CMAKE_SOURCE_DIR})

add_library(bque STATIC bufferqueue.c)

add_subdirectory(example)
