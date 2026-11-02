cmake_minimum_required(VERSION 3.29)
project(Lab2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)

add_executable(Lab2
        main.cpp
        Utils.h
        Timer.h
        FileGenerator.h
        FileGenerator.cpp
        FileProcessor.cpp
        FileProcessor.h
)

target_link_libraries(Lab2 PRIVATE kernel32)