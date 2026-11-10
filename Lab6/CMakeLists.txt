cmake_minimum_required(VERSION 3.20)
project(Lab6_System_Disks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(Lab6_Disks
        src/main.cpp
        src/DiskAnalyzer.cpp
        src/Utils.cpp
        include/DiskAnalyzer.h
        include/Utils.h
)