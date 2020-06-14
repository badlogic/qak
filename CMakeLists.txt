cmake_minimum_required(VERSION 3.15)
project(qak)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
file(GLOB INCLUDES "src/*.h")
file(GLOB SOURCES "src/*.cpp")

add_executable(qak ${INCLUDES} ${SOURCES})