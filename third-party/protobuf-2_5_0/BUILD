package(default_visibility = ["//visibility:public"])

cc_library(
    name = "protobuf-2_5_0",
    includes = ["src", "./"],
    srcs = ["src/.libs/libprotobuf-lite.a"],
    hdrs = glob(["src/google/protobuf/**/*.h"]),
    copts = [
        "-Os",
        "-fno-exceptions",
        "-ffunction-sections",
        "-fdata-sections",
    ],
)
