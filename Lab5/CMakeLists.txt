cmake_minimum_required(VERSION 3.20)
project(Lab5_Network)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# общие исходные файлы
set(COMMON_SOURCES
        common/Protocol.h
        common/SocketUtils.h
        common/SocketUtils.cpp
)

# сервер
add_executable(server
        server/ServerMain.cpp
        server/TextAnalyzer.h
        server/TextAnalyzer.cpp
        ${COMMON_SOURCES}
)
target_link_libraries(server ws2_32)

# клиент
add_executable(client
        client/ClientMain.cpp
        ${COMMON_SOURCES}
)
target_link_libraries(client ws2_32)