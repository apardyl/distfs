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
    ],
)

cc_binary(
    name = "distfs",
    deps = [
        ":distfs-lib",
    ],
)
