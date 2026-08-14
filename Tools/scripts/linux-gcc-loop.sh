#!/usr/bin/env bash
#===============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#  linux-gcc-loop.sh — reproduce the CI ubuntu-gcc leg locally in a
#  container, so Linux-only GCC diagnostics surface before a red
#  Actions run. Builds the repo's Dockerfile (ubuntu:24.04, g++ 13.3)
#  and runs the exact configure → build → test → tool-smoke sequence
#  the ubuntu-gcc job runs, building into the container's own /build.
#
#  Usage:  bash Tools/scripts/linux-gcc-loop.sh
#
#  Requires Docker (or set DOCKER=podman). The host checkout is mounted
#  read-only; nothing writes back to it.
#
#===============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="lce-linux-gcc:dev"
DOCKER="${DOCKER:-docker}"

"$DOCKER" build -t "$IMAGE" -f "$ROOT/Dockerfile" "$ROOT"

"$DOCKER" run --rm -v "$ROOT":/src:ro -w /src "$IMAGE" bash -lc '
    set -euo pipefail
    cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLCE_BUILD_TESTS=ON \
        -DLCE_BUILD_SAMPLES=ON \
        -DLCE_BUILD_TOOLS=ON
    bash Tools/scripts/ci-build.sh /build Debug
    ctest --test-dir /build -C Debug --output-on-failure
    bash Tools/scripts/ci-tool-smokes.sh /build
'
