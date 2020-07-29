sh compile_version_info.sh

mv CMakeLists_Linux.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Linux.txt


cat version.h