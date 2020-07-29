sh compile_version_info.sh

bazel build :all
bazel run :rts_cpp_test


cat version.h