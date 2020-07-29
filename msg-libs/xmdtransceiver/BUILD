package(default_visibility = ["//visibility:public"])

version = "1.0.0.0"

cc_library(
	name = "xmdtransceiver",
	includes = ["include"],
	hdrs = glob([
		"include/*.h",
		"include/*.hpp",
	]),
	copts = [
		"-g",
		"-gdwarf-2",
		"-O2",
		"-fno-omit-frame-pointer",
		"-Wall",
		"-D_REENTRANT",
		"-Wno-deprecated",
		"-DVERSION=" + version,
		"-DBUILD_DATE=UNKNOWN", # TODO: fix this
	],
	linkopts = [
		"-pthread",
                "-lm",
                "-ldl",
	],
	linkstatic = 1,
	srcs = glob([
		"src/*.cpp",
                "src/*.c",
	]),
	deps = [
	],
)

cc_test(
  name = "xmdtransceiver_test",
  srcs = glob([
		"test/test*.cpp",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
    "//third-party/gtest-170",
	":xmdtransceiver",
  ],
) # cc_test:xmdtransceiver_test

cc_binary(
  name = "xmdtransceiver_server",
  srcs = glob([
		"test/server/*.*",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
	":xmdtransceiver",
  ],
) # cc_binary:xmdtransceiver_server

cc_binary(
  name = "xmdtransceiver_client",
  srcs = glob([
		"test/client/*.*",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
	":xmdtransceiver",
  ],
) # cc_binary:xmdtransceiver_client

cc_binary(
  name = "StressSender2",
  srcs = glob([
		"test/stress/Sender2.cpp",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
	":xmdtransceiver",
  ],
) # cc_binary:xmdtransceiver_server

cc_binary(
  name = "StressSender",
  srcs = glob([
		"test/stress/Sender.cpp",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
	":xmdtransceiver",
  ],
) # cc_binary:xmdtransceiver_server

cc_binary(
  name = "StressReceiver",
  srcs = glob([
		"test/stress/Receiver.cpp",
  ]),
  copts = [
	"-O2",
	"-Wall",
	"-pthread",
	"-D_REENTRANT",
	"-Wno-unused-variable",
	"-Wno-deprecated-declarations",
  ],
  deps = [
	":xmdtransceiver",
  ],
) # cc_binary:xmdtransceiver_server

