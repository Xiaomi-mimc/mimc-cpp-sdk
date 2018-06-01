cc_library(
    name = "mimc_cpp_sdk",
    copts = [
        "-g",
        "-pthread",
#        "-DSTAGING",
    ],
    linkopts = [
        "-lm",
        "-lcrypto",
        "-pthread",
    ],
    includes = ["include"],
    srcs = glob(["src/*.cpp", "src/*.cc"]),
    hdrs = glob(["include/*.h"]),
    deps = [
    	"//third-party/jsoncpp",
        "//third-party/zlib-128",
        "//third-party/log4cplus-120",
        "//msg-libs/crypto",
        "//third-party/protobuf-250",
    ]
)

cc_binary(
    name = "mimc_cpp_demo",
    copts = [
        "-g",
        "-O3",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl"
    ],
    srcs = ["demo/mimc_demo.cpp"],
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_test(
    name = "mimc_cpp_test",
    copts = [
        "-g",
    ],
    linkopts = [
         "-lz",
         "-lssl"
    ],
    srcs = glob([
        "test/mimc_test.cpp"
    ]),
    deps = [
        "//msg-libs/gtestx",
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)
