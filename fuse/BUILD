package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distfs-fuse",
    srcs = glob(["**/*.cc"]),
    hdrs = glob(["**/*.h"]),
    deps = [
        "//common:distfs-common",
        "//meta:distfs-meta",
        "@com_github_libfuse//:libfuse",
    ],
)
