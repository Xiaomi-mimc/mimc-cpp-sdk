package(default_visibility = ["//visibility:public"])

cc_library(
	name = "gflags-20",
	includes = ["src"],
	hdrs = glob(["src/gflags/*.h"]),
	copts = [
		"-Wall",
		"-Wwrite-strings",
		"-Woverloaded-virtual",
		"-Wno-sign-compare",
		"-Wno-unused-local-typedefs",
		"-DNDEBUG",
		"-pthread",
	],
	linkopts = [
		"-pthread",
	],
	linkstatic = 1,
	hdrs_check = "loose",
	srcs = glob([
		"src/flags/*.h", 
		"src/mutex.h",
		"src/util.h",
		"src/gflags.cc",
		"src/gflags_reporting.cc",
		"src/gflags_completions.cc",
	]),
	deps = [],
)
