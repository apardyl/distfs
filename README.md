# distfs - distributed file system for diskless nodes

## Build:
This project uses [Bazel](https://bazel.build/) as the build system. No need to install dependencies - Bazel will download and build them for you.

To build distfs run `bazel build :distfs` in the main project directory. The result is a single binary file located in `bazel-out/k8-fastbuild/bin/distfs`.

## Usage
Create a new distfs file system from a directory:
```
    distfs build <path to distfs cache> <path to source directory>
```
Server-only mode:
```
    distfs serve <path to distfs cache> <port to listen on>
```
Mount distfs:
```
    distfs mount <path to distfs cache> <cache size in 4MB block number> <port to listen on> <known distfs node> <mount path>
```

