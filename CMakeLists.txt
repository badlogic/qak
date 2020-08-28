cmake_minimum_required(VERSION 3.15)
project(qak)

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 -D_CRT_SECURE_NO_WARNINGS")

    if (CMAKE_BUILD_TYPE MATCHES Release)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif ()

    if (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
    endif ()

    message("MSVC CXX Flags: ${CMAKE_CXX_FLAGS}")
    message("MSVC CXX Flags Release: ${CMAKE_CXX_FLAGS_RELEASE}")
else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")

    if (CMAKE_BUILD_TYPE MATCHES Release)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-exceptions -fno-rtti")
    endif ()
    message("Clang CXX Flags: ${CMAKE_CXX_FLAGS}")
    message("Clang CXX Flags Release: ${CMAKE_CXX_FLAGS_RELEASE}")
endif ()

set(CMAKE_CXX_STANDARD 11)

# C++
include_directories(src)
file(GLOB INCLUDES "src/*.h")
file(GLOB SOURCES "src/*.cpp")
add_library(qak-lib ${INCLUDE} ${SOURCES})
set_target_properties(qak-lib PROPERTIES OUTPUT_NAME "qak")

add_executable(qak ${INCLUDES} "src/apps/qak.cpp")
target_link_libraries(qak LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_memory ${INCLUDES} "src/apps/test_memory.cpp")
target_link_libraries(test_memory LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_map ${INCLUDES} "src/apps/test_map.cpp")
target_link_libraries(test_map LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_tokenizer ${INCLUDES} "src/apps/test_tokenizer.cpp")
target_link_libraries(test_tokenizer LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_parser ${INCLUDES} "src/apps/test_parser.cpp")
target_link_libraries(test_parser LINK_PUBLIC qak-lib)

include_directories(src/apps)
add_executable(test_c_api ${INCLUDES} "src/apps/test_c_api.c")
target_link_libraries(test_c_api LINK_PUBLIC qak-lib)

# C
file(GLOB INCLUDES_C "src/c/*.h")
file(GLOB SOURCES_C "src/c/*.c")
add_library(qak-lib-c ${INCLUDE_C} ${SOURCES_C})
set_target_properties(qak-lib-c PROPERTIES OUTPUT_NAME "qak-c")
set_property(TARGET qak-lib-c PROPERTY C_STANDARD 11)

include_directories(src/apps)
add_executable(test_parser_c ${INCLUDES} "src/apps/test_parser.c")
target_link_libraries(test_parser_c LINK_PUBLIC qak-lib-c)

include_directories(src/apps)
add_executable(test_tokenizer_c ${INCLUDES} "src/apps/test_tokenizer.c")
target_link_libraries(test_tokenizer_c LINK_PUBLIC qak-lib-c)

include_directories(src/apps)
add_executable(test_allocation ${INCLUDES} "src/apps/test_allocation.c")
target_link_libraries(test_allocation LINK_PUBLIC qak-lib-c)