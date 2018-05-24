package(default_visibility = ["//visibility:public"])

cc_library(
	name = "zlib-128",
	includes = [
    "."
  ],
	hdrs = [
    "zlib.h"
  ],
	copts = [
		"-O3",
    "-fPIC",
    "-D_LARGEFILE64_SOURCE=1",
    "-DHAVE_HIDDEN",
    "-DPIC",
	],
	linkstatic = 1,
	linkopts = [
  ],
	hdrs_check = "loose",
	srcs = glob([
		"*.c",
	]),
	deps = [
  ],
)

