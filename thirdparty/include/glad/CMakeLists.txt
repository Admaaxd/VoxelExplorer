cmake_minimum_required(VERSION 3.16)
project(Glad)

set(CMAKE_CXX_STANDARD 20)

add_library(glad include/glad/glad.h include/KHR/khrplatform.h src/glad.c)
target_include_directories(glad PUBLIC include/)