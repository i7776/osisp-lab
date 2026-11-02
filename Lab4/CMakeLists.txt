cmake_minimum_required(VERSION 3.29)
project(Lab_ReadersWriters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)

add_executable(Lab_ReadersWriters
        main.cpp
        SharedData.h
        Threads.h
        Threads.cpp
)

target_link_libraries(Lab_ReadersWriters PRIVATE kernel32)