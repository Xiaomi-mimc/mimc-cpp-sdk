cmake_minimum_required(VERSION 3.8)
PROJECT(gtest-170)

set(CMAKE_CXX_STANTARD 98)
message(STATUS "begin build")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-g -DGTEST_HAS_PTHREAD -g -O2 -Wall -pthread")
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src LIB_SRC)
add_library(gtest-170 STATIC ${LIB_SRC})
target_link_libraries(gtest-170 pthread)
