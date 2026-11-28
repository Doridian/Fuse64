# CI's Linux build environment (.github/workflows/build.yml, the linux job):
# Ubuntu 22.04 with apt.llvm.org's clang 18 and lld, SDL2 and libepoxy, for
# trying CI's builds here (port/tools/snapshot.py --podman; docs/PORT.md,
# "The snapshot"):
#   podman build -t fuse64-ci-linux -f port/tools/ci/linux.Containerfile port/tools/ci
#   podman build --arch arm64 -t fuse64-ci-linux-arm64 -f port/tools/ci/linux.Containerfile port/tools/ci
# Keep it the same as the linux job's "Install tools".
FROM docker.io/library/ubuntu:26.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y cmake ninja-build pkg-config \
        libsdl2-dev libepoxy-dev file python3 curl lsb-release wget software-properties-common gnupg
RUN curl -sSLfo /llvm.sh https://apt.llvm.org/llvm.sh && bash /llvm.sh 18
ENV PATH=/usr/lib/llvm-18/bin:$PATH
