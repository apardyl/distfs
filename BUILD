package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distfs-lib",
    srcs = [
        "main.cc"
    ],
    deps = [
        "//meta:distfs-meta",
        "//data:distfs-data",
        "//fuse:distfs-fuse",
        "//network:distfs-network",
    ],
)

cc_binary(
    name = "distfs",
    deps = [
        ":distfs-lib",
    ],
)
