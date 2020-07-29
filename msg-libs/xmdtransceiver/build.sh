#!/bin/sh
set -x

PROJ_HOME=`pwd`

# local compile
#cd ${PROJ_HOME}
#TARGET=iotdcm_stable_test
#touch WORKSPACE
#bazel build :${TARGET}

# cross compile
mkdir bin_test
cd bin_test
cmake -DPLATFORM=$1 ..

make

cd ..
rm -rf bin_test
