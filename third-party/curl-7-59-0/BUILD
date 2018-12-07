package(default_visibility = ["//visibility:public"])

cc_library(
        name = "curl-7-59-0",
        copts = [
            "-DHAVE_CONFIG_H",
            "-DBUILDING_LIBCURL",
            "-Os",
            "-fno-exceptions",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wall",
        ],
        linkstatic = 1,
        linkopts = [
            "-lrt",
        ],
        includes = ["include", "lib"],
        hdrs = glob([
            "include/curl/*.h",
            "lib/*.h",
            "lib/vauth/*.h",
            "lib/vtls/*.h",
            "src/*.h",
            ]),
        srcs = glob([
            "lib/*.c",
            "lib/vauth/*.c",
            "lib/vtls/*.c",
            "src/*.c",
        ]),
        deps = [],
)
