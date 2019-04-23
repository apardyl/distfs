package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distfs-lib",
    srcs = [
        "main.cc"
    ],
    deps = [
        "//meta:distfs-meta",
    ],
)

cc_binary(
    name = "distfs",
    deps = [
        ":distfs-lib",
    ],
)
