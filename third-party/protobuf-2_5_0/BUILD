package(default_visibility = ["//visibility:public"])

cc_library(
    name = "protobuf-2_5_0",
    includes = ["src", "."],
	hdrs = glob(["src/google/protobuf/**/*.h", "*.h"]),
	copts = [
		"-Os",
		"-fno-exceptions",
		"-ffunction-sections",
		"-fdata-sections",
		"-Wall",
		"-Wno-sign-compare",
		"-Wno-unused-local-typedefs",
	],
	srcs = [
		"src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc",
		"src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc",
		"src/google/protobuf/stubs/common.cc",
		"src/google/protobuf/stubs/once.cc",
		"src/google/protobuf/stubs/hash.h",
		"src/google/protobuf/stubs/map-util.h",
		"src/google/protobuf/stubs/stl_util.h",
		"src/google/protobuf/stubs/stringprintf.cc",
		"src/google/protobuf/stubs/stringprintf.h",
		"src/google/protobuf/extension_set.cc",
		"src/google/protobuf/generated_message_util.cc",
		"src/google/protobuf/message_lite.cc",
		"src/google/protobuf/repeated_field.cc",
		"src/google/protobuf/wire_format_lite.cc",
		"src/google/protobuf/io/coded_stream.cc",
		"src/google/protobuf/io/coded_stream_inl.h",
		"src/google/protobuf/io/zero_copy_stream.cc",
		"src/google/protobuf/io/zero_copy_stream_impl_lite.cc",
	],
deps = [
	],
)
