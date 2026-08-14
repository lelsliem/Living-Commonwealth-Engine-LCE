#!/usr/bin/env bash
#===============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#  ci-build.sh — the shared CI build step. Builds a CMake tree and, on
#  failure, re-emits the error lines as ::error:: workflow annotations —
#  the same text the step log shows, but readable from the Checks API so
#  a build break is diagnosable without signing in.
#
#  Usage:  bash Tools/scripts/ci-build.sh <build-dir> [config]
#
#===============================================================================
set -euo pipefail

BUILD_DIR="${1:?usage: ci-build.sh <build-dir> [config]}"
CONFIG="${2:-Debug}"
LOG="${BUILD_DIR}/build.log"

# pipefail (above) makes the pipeline's status cmake's, not tee's, so a
# failed build reaches the error branch instead of being masked by tee.
if ! cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel 2>&1 | tee "$LOG"; then
    echo "::group::last errors"
    grep -aiE 'error:|fatal error|undefined reference|internal compiler' "$LOG" \
        | tail -20 \
        | sed 's/^/::error::/' || true
    echo "::endgroup::"
    exit 1
fi
