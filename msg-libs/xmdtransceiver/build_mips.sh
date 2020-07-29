mv CMakeLists_Mips.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Mips.txt
cp ./build/libxmdtransceiver.so ../../../lib
md5sum ./build/libxmdtransceiver.so
md5sum ../../../lib/libxmdtransceiver.so
