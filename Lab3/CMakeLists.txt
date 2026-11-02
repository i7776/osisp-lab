cmake_minimum_required(VERSION 3.29)
project(Lab3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)

add_executable(Lab3
        main.cpp
        Protocol.h
        PipeHelper.h
        Utils.h
        Collector.h
        Collector.cpp
        Validator.h
        Validator.cpp
        Processor.h
        Processor.cpp
        Generator.h
        Generator.cpp
)

target_link_libraries(Lab3 PRIVATE kernel32)