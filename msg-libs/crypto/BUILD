package(default_visibility = ["//visibility:public"])

version = "1.0.0.0"

cc_library(
    name = "crypto",
    includes = [
        "include",
        "include/crypto"
    ],
    copts = [
        "-g",
        "-gdwarf-2",
        "-O0",
        "-Wall",
    ],
    nocopts = "-fPIC",
    linkstatic = 1,
    srcs = glob([
        "src/*.cpp",
        "include/*.h"
    ]),
    deps = [],
)

cc_test(
    name = "test",
    includes = ["include"],
    copts = [
        "-g",
        "-O2",
        "-Wall",
    ],
    nocopts = "-fPIC",
    linkstatic = 1,
    srcs = glob(["test/*_test.cpp"]),
    deps = [
        ":crypto",
        "//msg-libs/passport:xiaomi-ppsdk",
        "//third-party/gtest-170",
    ],
)
