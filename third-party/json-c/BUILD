package(default_visibility = ["//visibility:public"])

cc_library(
    name = "json-c",
    includes = ["."],
    hdrs = glob(["json-c/*.h"]),
    copts = [
        "-Os",
        "-Wall",
        "-Werror", 
        "-Wno-error=deprecated-declarations",
        "-Wextra", 
        "-Wwrite-strings", 
        "-Wno-implicit-fallthrough",
        "-Wno-unused-parameter", 
        "-std=gnu99", 
        "-D_GNU_SOURCE", 
        "-D_REENTRANT",
        "-fno-exceptions",
        "-ffunction-sections",
        "-fdata-sections",
    ],
    linkopts = [
        "-Wl,--gc-sections",
    ],
    srcs = glob([
        "json-c/*.c",
    ]),
    deps = [],
)
