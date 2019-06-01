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

load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar", "pkg_deb")

pkg_tar(
    name = "distfs-pkg",
    extension = "tar.gz",
    srcs = [":distfs"],
    mode = "0755",
)
