#!/bin/sh

cd `dirname $0`/..

mkdir -p build_x86
cd build_x86

cmake -DANDROID_NATIVE_API_LEVEL=android-9 -DANDROID_ABI=x86 -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake $@ ../..

