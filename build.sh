cd third-party/curl-7-59-0
./configure --disable-ldap --disable-ldaps
make

cd ../..
bazel build :all
bazel run :rts_cpp_test

git checkout third-party
