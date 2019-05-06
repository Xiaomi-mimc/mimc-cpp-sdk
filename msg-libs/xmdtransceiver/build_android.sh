mv CMakeLists_Android.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
#cmake -DDEBUG=OFF -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake\
#      -DANDROID_NDK=$ANDROID_NDK_HOME\
#      -DANDROID_ABI=armeabi-v7a\
#      -DANDROID_TOOLCHAIN=clang\
#      -DANDROID_PLATFORM=android-21\
#      -DANDROID_STL=c++_shared\
#      ..
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Android.txt
