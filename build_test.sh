sh compile_version_info.sh

mv CMakeLists_Test.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Test.txt


cat version.h