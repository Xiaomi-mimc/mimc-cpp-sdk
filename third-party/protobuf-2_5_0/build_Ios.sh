mv CMakeLists_Ios.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
echo "start build for arm64 armv7 armv7s..."
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=OS
cmake --build . --config Release
mv Release/libprotobuf.a ../libprotobuf_arm.a
echo "build for arm64 armv7 armv7s sucess"
cd ..

rm -rf build
mkdir build
mv libprotobuf_arm.a build/
cd build
echo "start build for x86_64..."
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=SIMULATOR64
cmake --build . --config Release
echo "build for x86_64 success"
mv Release/libprotobuf.a libprotobuf_x86.a

lipo -create libprotobuf_arm.a libprotobuf_x86.a -output libprotobuf.a
rm libprotobuf_arm.a
rm libprotobuf_x86.a
echo "build for x86_64 arm64 armv7 armv7s success"
cd ..
mv CMakeLists.txt CMakeLists_Ios.txt
