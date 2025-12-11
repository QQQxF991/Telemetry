cmake_minimum_required(VERSION 3.10)
project(telemetry_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(SOURCES
    main.cpp
    binary_message.cpp
    servers.cpp
)

set(HEADERS
    structs.hpp
    binary_message.hpp
)

add_executable(telemetry_server ${SOURCES} ${HEADERS})

target_link_libraries(telemetry_server Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(telemetry_server PRIVATE 
        -Wall
        -Wextra
        -Werror
        -O2
        -pthread
    )
endif()