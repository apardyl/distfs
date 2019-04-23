workspace(name = "com_pardyl_distfs")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_github_libfuse",
  url = "https://github.com/libfuse/libfuse/releases/download/fuse-3.5.0/fuse-3.5.0.tar.xz",
  sha256 = "75bfee6b730145483d18238b50daccde4c1b8133fa1703367fbf8088d0666bf0",
  build_file = "BUILD.libfuse",
  strip_prefix = "fuse-3.5.0",
)
