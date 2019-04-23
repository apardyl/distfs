workspace(name = "com_pardyl_distfs")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_github_libfuse",
  url = "https://github.com/libfuse/libfuse/releases/download/fuse-3.5.0/fuse-3.5.0.tar.xz",
  sha256 = "75bfee6b730145483d18238b50daccde4c1b8133fa1703367fbf8088d0666bf0",
  build_file = "BUILD.libfuse",
  strip_prefix = "fuse-3.5.0",
)

http_archive(
  name = "com_github_lz4",
  url = "https://github.com/lz4/lz4/archive/v1.9.1.tar.gz",
  sha256 = "f8377c89dad5c9f266edc0be9b73595296ecafd5bfa1000de148096c50052dc4",
  build_file = "BUILD.lz4",
  strip_prefix = "lz4-1.9.1/lib",
)
