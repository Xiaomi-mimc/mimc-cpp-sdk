sh compile_version_info.sh

mv CMakeLists_Ios.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
echo "start build for arm64 armv7 armv7s..."
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=OS
cmake --build . --config Release
mv Release/libmimc_sdk.a ../libmimc_sdk_arm.a
echo "build for arm64 armv7 armv7s sucess"
cd ..

rm -rf build
mkdir build
cd build
echo "start build for i386..."
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=SIMULATOR
cmake --build . --config Release
mv Release/libmimc_sdk.a ../libmimc_sdk_i386.a
echo "build for arm64 armv7 armv7s sucess"
cd ..



rm -rf build
mkdir build
mv libmimc_sdk_arm.a build/
mv libmimc_sdk_i386.a build/
cd build
echo "start build for x86_64..."
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=SIMULATOR64
cmake --build . --config Release
echo "build for x86_64 success"
mv Release/libmimc_sdk.a libmimc_sdk_x86.a

lipo -create libmimc_sdk_arm.a libmimc_sdk_x86.a libmimc_sdk_i386.a -output libmimc_sdk.a
rm libmimc_sdk_arm.a
rm libmimc_sdk_x86.a
rm libmimc_sdk_i386.a
echo "build for x86_64 i386 arm64 armv7 armv7s success"
cd ..
mv CMakeLists.txt CMakeLists_Ios.txt


cat version.h