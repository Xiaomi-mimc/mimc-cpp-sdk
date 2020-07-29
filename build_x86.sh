sh compile_version_info.sh

mv CMakeLists_X86.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_X86.txt


cat version.h