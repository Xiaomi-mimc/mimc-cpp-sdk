package(default_visibility = ["//visibility:public"])

cc_library(
    name = "json-c",
    includes = ["."],
    hdrs = glob(["json-c/*.h"]),
    copts = [
        "-O2",
        "-g",
        "-Wall",
        "-Werror", 
        "-Wno-error=deprecated-declarations", 
        "-Wextra", 
        "-Wwrite-strings", 
        "-Wno-unused-parameter", 
        "-std=gnu99", 
        "-D_GNU_SOURCE", 
        "-D_REENTRANT",
    ],
    linkopts = [
    ],
#    linkstatic = 1,
    srcs = glob([
        "json-c/*.c",
    ]),
    deps = [],
)
