package(default_visibility = ["//visibility:public"])

cc_library(
	name = "gtest-170",
	copts = [
		"-DGTEST_HAS_PTHREAD=1",
		"-Os",
		"-ffunction-sections",
		"-fdata-sections",
		"-Wall",
		"-Wextra",
		"-Wno-missing-field-initializers",
		"-pthread",
	],
	linkopts = [
		"-pthread", 
		"-Wl,--gc-sections",
	],
#	linkstatic = 1,
	includes = ["include", "."],
	hdrs = glob(["include/gtest/*.h", "include/gtest/internal/*.h", "src/*.h"]),
	srcs = glob(["src/*.cc"]),
	deps = [],
)
