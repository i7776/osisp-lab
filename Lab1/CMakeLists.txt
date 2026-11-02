cmake_minimum_required(VERSION 3.29)
project(Lab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)
add_executable(Lab1
        main.cpp
        Timer.h
        Matrix.h
        Matrix.cpp
        Threads.cpp
        Threads.h
        Utils.h
)

target_link_libraries(Lab1 PRIVATE kernel32)
