cmake_minimum_required(VERSION 3.15)
project(qak)


if(MSVC)
    message("MSCV detected")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c89")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wnon-virtual-dtor -pedantic -std=c++03 -fno-exceptions -fno-rtti")
endif()

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
file(GLOB INCLUDES "src/*.h")
file(GLOB SOURCES "src/*.cpp")
add_library(qak-lib ${INCLUDE} ${SOURCES})
set_target_properties(qak-lib PROPERTIES OUTPUT_NAME "qak")

add_executable(qak ${INCLUDES} "src/apps/qak.cpp" src/apps/test.h)
target_link_libraries(qak LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_tokenizer ${INCLUDES} "src/apps/test_tokenizer.cpp")
target_link_libraries(test_tokenizer LINK_PUBLIC qak-lib)