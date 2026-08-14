#===============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#  The local Linux+GCC dev loop. Matches the CI ubuntu-gcc leg
#  (ubuntu:24.04 ships the runner's g++ 13.3), so Linux-only GCC
#  diagnostics — like GCC 13+'s -Wchanges-meaning error — surface
#  locally before a red Actions run.
#
#  Use via:  bash Tools/scripts/linux-gcc-loop.sh
#
#===============================================================================

FROM ubuntu:24.04

# build-essential brings g++ (13.3 on 24.04); g++-13 is named for clarity
# so the image advertises the exact toolchain it reproduces.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        g++-13 \
        cmake \
        ninja-build \
        ca-certificates \
        git \
    && rm -rf /var/lib/apt/lists/*
