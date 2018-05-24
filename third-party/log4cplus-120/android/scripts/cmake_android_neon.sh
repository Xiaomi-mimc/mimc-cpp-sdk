#!/bin/sh
cd `dirname $0`/..

mkdir -p build_neon
cd build_neon

cmake -DANDROID_NATIVE_API_LEVEL=android-9 -DANDROID_ABI="armeabi-v7a with NEON" -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake $@ ../..

