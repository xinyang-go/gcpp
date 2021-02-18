cmake_minimum_required(VERSION 3.12)
project(gcpp)

set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif ()

add_compile_options(-Wall -Werror=return-type)
if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_compile_options(-O3 -flto -march=native)
elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    add_compile_options(-O3 -flto -march=native -g)
elseif (CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
    add_compile_options(-Os -flto -march=native)
else ()
    add_compile_options(-O0 -g)
endif ()

include_directories(include)

find_package(Threads)
file(GLOB_RECURSE src src/*.cpp)
add_library(gcpp SHARED ${src})
target_link_libraries(gcpp Threads::Threads)

add_executable(gcpp_example_basic example/basic.cpp)
target_link_libraries(gcpp_example_basic gcpp)

add_executable(gcpp_example_multi_thread example/multi_thread.cpp)
target_link_libraries(gcpp_example_multi_thread gcpp)
