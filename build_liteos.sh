sh compile_version_info.sh

mv CMakeLists_Liteos.txt CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
mv CMakeLists.txt CMakeLists_Liteos.txt


cat version.h