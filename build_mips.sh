sh compile_version_info.sh

mv CMakeLists_Mips.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Mips.txt
cp ./build/libmimc_sdk.so ../lib
md5sum ./build/libmimc_sdk.so
md5sum ../lib/libmimc_sdk.so

cat version.h