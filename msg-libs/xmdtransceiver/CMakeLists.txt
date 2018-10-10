CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(xmdtransceiver)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-g -O2 -fno-omit-frame-pointer -Wall -D_REENTRANT -Wno-deprecated -fpermissive")

set(LOG_INCLUDE_DIR /home/work/codes/mimc/mimc-cpp-sdk/third-party/log4cplus-120/include)
set(ZLIB_INCLUDE_DIR /home/work/codes/mimc/mimc-cpp-sdk/third-party/zlib-128)

include_directories(${PROJECT_SOURCE_DIR}/include ${LOG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
aux_source_directory(${PROJECT_SOURCE_DIR}/src LIB_SRC)
add_library(xmdtransceiver STATIC ${LIB_SRC})
