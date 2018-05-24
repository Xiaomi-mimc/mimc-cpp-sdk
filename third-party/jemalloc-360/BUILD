package(default_visibility = ["//visibility:public"])

cc_library(
	name = "jemalloc-360",
	includes = ["include"],
	hdrs = glob(["include/jemalloc/*.h"]),
	copts = [
		"-D_GNU_SOURCE",
		"-D_REENTRANT",
		"-std=gnu99",
		"-Wall",
		"-pipe",
		"-g3",
		"-fvisibility=hidden",
		"-O3",
		"-funroll-loops",
		"-Wno-unused-but-set-variable",
		"-Wno-unused-function",
		"-Wno-maybe-uninitialized",
		"-Wno-unused-variable",
	],
	linkopts = ["-pthread"],
	linkstatic = 1,
	hdrs_check = "loose",
	srcs = glob(
		[
			"include/jemalloc/*.h",
			"include/jemalloc/internal/*.h",
			"src/*.c",
		], 
		["src/zone.c"],
	),
	deps = [],
)

cc_library(
	name = "test_lib",
	includes = ["test/include"],
	copts = [
		"-DJEMALLOC_INTEGRATION_TEST",
		"-std=gnu99",
	],
	linkstatic = 1,
	srcs = glob(["test/src/*.c"]),
	deps = [":jemalloc-360"],
)

cc_test(
	name = "test_allocated",
	copts = [
		"-DJEMALLOC_INTEGRATION_TEST",
		"-std=gnu99",
	],
	srcs = ["test/integration/allocated.c"],
	deps = [":test_lib"],
)

cc_test(
	name = "test_allocm",
	copts = [
		"-DJEMALLOC_INTEGRATION_TEST",
		"-std=gnu99",
	],
	srcs = ["test/integration/allocm.c"],
	deps = [":test_lib"],
)

cc_test(
	name = "test_mallocx",
	copts = [
		"-DJEMALLOC_INTEGRATION_TEST",
		"-std=gnu99",
	],
	srcs = ["test/integration/mallocx.c"],
	deps = [":test_lib"],
)

cc_test(
	name = "test_thread_arena",
	copts = [
		"-DJEMALLOC_INTEGRATION_TEST",
		"-std=gnu99",
	],
	srcs = ["test/integration/thread_arena.c"],
	deps = [":test_lib"],
)

