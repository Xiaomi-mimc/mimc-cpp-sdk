package(default_visibility = ["//visibility:public"])

cc_library(
	name = "jsoncpp",
	includes = [
    "include",
  ],
	hdrs = glob([
    "include/json/*.h"
  ]),
	copts = [
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
		"-Wno-overflow",
		"-pedantic",
		"-Werror=strict-aliasing",
		"-O3",
		"-DNDEBUG",
	],
	linkstatic = 1,
	linkopts = [
  ],
	hdrs_check = "loose",
	srcs = [
		"src/json_reader.cpp",
		"src/json_value.cpp",
		"src/json_writer.cpp",
	],
	deps = [
  ],
)

