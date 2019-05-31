package(default_visibility = ["//visibility:public"])

load("@build_stack_rules_proto//cpp:cpp_grpc_library.bzl", "cpp_grpc_library")

proto_library(
    name = "distfs-proto",
    srcs = glob(["**/*.proto"]),
)

cpp_grpc_library(
    name = "distfs-rpc-lib",
    deps = [":distfs-proto"],
)
