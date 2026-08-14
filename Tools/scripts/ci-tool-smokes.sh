#!/usr/bin/env bash
#===============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#  ci-tool-smokes.sh — smoke-run the Bench and Studio after a build.
#  The tools live in <build-dir>/Bin on single-config generators (Linux)
#  and <build-dir>/Bin/<config> on multi-config (MSVC) — find them. The
#  name carries the platform's executable suffix (LCE.Bench vs
#  LCE.Bench.exe), so match both and fail loudly rather than let an
#  empty path become a cryptic "command not found".
#
#  Usage:  bash Tools/scripts/ci-tool-smokes.sh <build-dir>
#
#===============================================================================
set -euo pipefail

BUILD_DIR="${1:?usage: ci-tool-smokes.sh <build-dir>}"

BENCH="$(find "$BUILD_DIR/Bin" \( -name 'LCE.Bench' -o -name 'LCE.Bench.exe' \) -type f | head -1)"
if [ -z "$BENCH" ]; then
    echo "FAIL — LCE.Bench not found under $BUILD_DIR/Bin"
    find "$BUILD_DIR/Bin" | head -20
    exit 1
fi
echo "bench: $BENCH"
"$BENCH" --sanity

STUDIO="$(find "$BUILD_DIR/Bin" \( -name 'LCE.Studio' -o -name 'LCE.Studio.exe' \) -type f | head -1)"
if [ -z "$STUDIO" ]; then
    echo "FAIL — LCE.Studio not found under $BUILD_DIR/Bin"
    find "$BUILD_DIR/Bin" | head -20
    exit 1
fi
echo "studio: $STUDIO"
"$STUDIO" --selftest
