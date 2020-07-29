cc_library(
    name = "mimc_cpp_sdk",
    copts = [
        "-Os",
        "-fno-omit-frame-pointer",
#        "-fno-rtti",
        "-fno-exceptions",
        "-ffunction-sections",
        "-fdata-sections",
        "-pthread",
        "-fPIC",
        "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
        "-lm",
        "-lcrypto",
        "-pthread",
        "-Wl,--gc-sections",
    ],
    includes = ["include"],
    srcs = glob(["src/*.cpp", "src/*.cc", "src/*.c"]),
    hdrs = glob(["include/**/*.h"]),
    deps = [
        "//msg-libs/xmdtransceiver",
        "//third-party/protobuf-2_5_0",
        "//third-party/json-c",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "mimc_cpp_demo",
    copts = [
         "-Os",
         "-fno-exceptions",
         "-fno-rtti",
         "-ffunction-sections",
         "-fdata-sections",
         "-D_GLIBCXX_USE_NANOSLEEP",
#         "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
	 "-ldl",
    ],
    linkstatic=True,
    includes = ["include"],
    srcs = glob(["demo/mimc_demo.cpp",
    "demo/**/*.h"]),
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "mimc_c_demo",
    copts = [
         "-Os",
         "-fno-exceptions",
         "-ffunction-sections",
         "-fdata-sections",
         "-D_GLIBCXX_USE_NANOSLEEP",
#         "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
	 "-ldl",
    ],
    linkstatic=True,
    srcs = ["demo/c_im_demo.c"],
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "rts_cpp_demo",
    copts = [
         "-Os",
         "-fno-exceptions",
         "-fno-rtti",
         "-ffunction-sections",
         "-fdata-sections",
         "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
    ],
    linkstatic=True,
    includes = ["include"],
    srcs = glob(["demo/rts_demo.cpp",
    "demo/**/*.h"]),
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "rts_c_demo",
    copts = [
         "-Os",
         "-ffunction-sections",
         "-fdata-sections",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
	 "-ldl",
    ],
    linkstatic=True,
    srcs = ["demo/c_rts_demo.c"],
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "rts_echo_demo",
    copts = [
         "-Os",
         "-fno-exceptions",
         "-fno-rtti",
         "-ffunction-sections",
         "-fdata-sections",
         "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
    ],
    linkstatic=True,
    srcs = glob(["demo/rts_echo_demo.cpp",
    "demo/**/*.h"]),
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_test(
    name = "mimc_cpp_test",
    copts = [
        "-Os",
        "-fno-exceptions",
        "-fno-rtti",
        "-ffunction-sections",
        "-fdata-sections", 
        "-I.",
        "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
    ],
    linkstatic=True,
    srcs = glob([
        "test/mimc_test.cpp",
        "test/**/*.h",
		"test/mimc_tokenfetcher.cpp",
    ]),
    deps = [
        "//third-party/gtest-170",
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_test(
    name = "rts_cpp_test",    
    copts = [
        "-Os",
        "-fno-exceptions",
        "-fno-rtti",
        "-ffunction-sections",
        "-fdata-sections",
        "-I.",
        "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
        "-lz",
        "-lssl",
        "-Wl,--gc-sections",
    ],
    linkstatic=True,
    srcs = glob([
       "test/rts_test.cpp",
       "test/**/*.h",
	   "test/mimc_tokenfetcher.cpp",
    ]),
    deps = [
        "//third-party/gtest-170",
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_test(
    name = "rts_performance_test",    
    copts = [
        "-Os",
        "-fno-exceptions",
        "-fno-rtti",
        "-ffunction-sections",
        "-fdata-sections",
        "-I.",
        "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
        "-lz",
        "-lssl",
        "-Wl,--gc-sections",
    ],
    linkstatic=True,
    srcs = glob([
       "test/rts_performance.cpp",
       "test/**/*.h",
	   "test/mimc_tokenfetcher.cpp",
    ]),
    deps = [
        "//third-party/gtest-170",
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_test(
    name = "rts_efficiency_test",    
    copts = [
        "-Os",
        "-fno-exceptions",
        "-fno-rtti",
        "-ffunction-sections",
        "-fdata-sections",
        "-I.",
        "-D_GLIBCXX_USE_NANOSLEEP",
#        "-DSTAGING",
    ],
    linkopts = [
        "-lz",
        "-lssl",
        "-Wl,--gc-sections",
    ],
    linkstatic=True,
    srcs = glob([
       "test/rts_efficiency.cpp",
       "test/**/*.h",
	   "test/mimc_tokenfetcher.cpp",
    ]),
    deps = [
        "//third-party/gtest-170",
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "rts_c_sender",
    copts = [
         "-Os",
         "-ffunction-sections",
         "-fdata-sections",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
         "-ldl",
    ],
    linkstatic=True,
    srcs = ["demo/c_rts_sender.c"],
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

cc_binary(
    name = "rts_c_receiver",
    copts = [
         "-Os",
         "-ffunction-sections",
         "-fdata-sections",
#        "-DSTAGING",
    ],
    linkopts = [
         "-lz",
         "-lssl",
         "-Wl,--gc-sections",
         "-ldl",
    ],
    linkstatic=True,
    srcs = ["demo/c_rts_receiver.c"],
    deps = [
        ":mimc_cpp_sdk",
        "//third-party/curl-7-59-0"
    ]
)

