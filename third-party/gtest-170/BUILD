package(default_visibility = ["//visibility:public"])

cc_library(
	name = "gtest-170",
	includes = ["include"],
	hdrs = glob(["include/gtest/*.h"]),
	copts = [
		"-DGTEST_HAS_PTHREAD=1",
		"-g",
		"-O2",
		"-Wall",
		"-Wextra",
		"-Wno-missing-field-initializers",
		"-pthread",
		"-I./third-party/gtest-170",
	],
	linkopts = ["-pthread"],
	linkstatic = 1,
	hdrs_check = "loose",
	srcs = glob(
		["src/*.cc"], 
		["src/gtest-all.cc"],
	),
	deps = [],
)
