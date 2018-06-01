package(default_visibility = ["//visibility:public"])

cc_library(
        name = "curl-7-59-0",
        copts = [
            "-O2",
            "-g",
            "-Wall",
            "-Wno-strict-aliasing",
            "-Wno-pointer-sign",
        ],
        linkstatic = 1,
        linkopts = [],
        includes = ["include"],
        hdrs = glob(["include/curl/*.h"]),
        srcs = [
        "lib/.libs/libcurl.a"
        ],
        deps = [],
)