package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distfs-data",
    srcs = glob(["**/*.cc"]),
    hdrs = glob(["**/*.h"]),
    deps = [
        "//common:distfs-common",
        "@com_github_lz4//:lz4",
    ],
)
