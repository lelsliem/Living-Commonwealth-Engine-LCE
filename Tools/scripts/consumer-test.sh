#!/usr/bin/env bash
#===============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#  0.9.1 — the packaging gate. Builds and installs the engine, then proves
#  BOTH embedding paths from Docs/SDK/Embedding.md against the result:
#
#    Path 1 — FetchContent  (the primary path). The consumer is generated
#             by lce-doctor init — the scaffold IS the recipe, so the doc
#             and the tool agree by construction. FETCHCONTENT_SOURCE_DIR_LCE
#             points it at this checkout instead of GitHub (the standard
#             offline override; GIT_TAG is then the pin in spirit only).
#
#    Path 2 — find_package  (the installed path). The Embedding.md recipe
#             verbatim, consuming the freshly installed prefix.
#
#  Each consumer builds and runs, and must print "the loop runs" — a
#  generated project that compiles and links against the released
#  surface IS the proof the recipe is real.
#
#  Usage:  bash Tools/scripts/consumer-test.sh [engine-root] [config]
#
#  CONFIG defaults to Debug. The Release workflow gates on Release so
#  the packaged shape is proven in the configuration it ships in.
#
#  Exit code: 0 when both legs pass, 1 otherwise. Runs on Git Bash
#  (Windows) and bash (Linux/CI).
#
#===============================================================================
set -euo pipefail

ENGINE_ROOT="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
CONFIG="${2:-Debug}"
SCRATCH="$(mktemp -d)"
trap 'rm -rf "$SCRATCH"' EXIT

echo "LCE packaging gate — both Embedding paths"
echo "engine : $ENGINE_ROOT"
echo "scratch: $SCRATCH"
echo

#-------------------------------------------------------------------------------
# The engine build + install. The doctor must exist (tools ON) so Path 1
# can generate its consumer; the install (tools/tests/samples OFF) is
# exactly the released shape the find_package path consumes.
#-------------------------------------------------------------------------------
echo "[1/3] building and installing the engine ..."
# The build type is explicit: single-config generators (and CMake >= 4)
# only emit the export's per-config file when a build type is set —
# without it, install(EXPORT) leaves the package without any
# IMPORTED_LOCATION and find_package consumers fail.
cmake -S "$ENGINE_ROOT" -B "$SCRATCH/engine" \
    -DCMAKE_BUILD_TYPE=$CONFIG \
    -DLCE_BUILD_TESTS=ON \
    -DLCE_BUILD_SAMPLES=OFF \
    -DLCE_BUILD_TOOLS=ON >/dev/null
cmake --build "$SCRATCH/engine" --config "$CONFIG" --parallel >/dev/null
cmake --install "$SCRATCH/engine" --config "$CONFIG" --prefix "$SCRATCH/prefix" >/dev/null

# grep -oP is GNU-only (BusyBox and BSD grep have no PCRE); sed does
# the same extraction everywhere the gate runs.
VERSION="$(sed -n 's/.*VersionString = "\([^"]*\)".*/\1/p' \
    "$ENGINE_ROOT/Include/LCE/Version/Version.h")"

echo "      installed: $VERSION"
echo

#-------------------------------------------------------------------------------
# Path 1 — FetchContent via the doctor scaffold.
#-------------------------------------------------------------------------------
echo "[2/3] Path 1 — FetchContent (lce-doctor init's scaffold) ..."

DOCTOR="$(find "$SCRATCH/engine/Bin" -name 'LCEDoctor*' -type f | head -1)"

if [ -z "$DOCTOR" ]; then
    echo "FAIL — LCEDoctor not found in the engine build"
    exit 1
fi

# The doctor scaffolds ./<name> in the CURRENT directory — cd first,
# pass only the name.
( cd "$SCRATCH" && "$DOCTOR" init consumer >/dev/null )

cmake -S "$SCRATCH/consumer" -B "$SCRATCH/consumer/build" \
    -DFETCHCONTENT_SOURCE_DIR_LCE="$ENGINE_ROOT" >/dev/null
cmake --build "$SCRATCH/consumer/build" --config "$CONFIG" --parallel >/dev/null

# The scaffold names its executable after the project name: "consumer"
# (consumer.exe on Windows, consumer on Linux). The build dir is full
# of consumer.* artifacts — match the binary, never a .recipe/.vcxproj.
CONSUMER="$(find "$SCRATCH/consumer/build" -name 'consumer.exe' -type f | head -1)"

if [ -z "$CONSUMER" ]; then
    CONSUMER="$(find "$SCRATCH/consumer/build" -name 'consumer' -type f | head -1)"
fi

if [ -z "$CONSUMER" ]; then
    echo "FAIL — the scaffolded consumer did not build"
    exit 1
fi

OUTPUT="$(cd "$SCRATCH/consumer" && "$CONSUMER")"

if echo "$OUTPUT" | grep -q "the loop runs"; then
    echo "      OK — scaffold built, linked $VERSION, ran"
else
    echo "FAIL — scaffold output:"
    echo "$OUTPUT"
    exit 1
fi

echo

#-------------------------------------------------------------------------------
# Path 2 — find_package against the installed prefix.
#-------------------------------------------------------------------------------
echo "[3/3] Path 2 — find_package(LCE) against the install ..."

mkdir -p "$SCRATCH/pkg-consumer"

cat > "$SCRATCH/pkg-consumer/CMakeLists.txt" <<'EOF'
# The Embedding.md find_package recipe, verbatim.
cmake_minimum_required(VERSION 3.28)
project(myworld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LCE 0.8 REQUIRED)

add_executable(myworld main.cpp)
target_link_libraries(myworld PRIVATE LCE::Core)
EOF

cp "$SCRATCH/consumer/main.cpp" "$SCRATCH/pkg-consumer/main.cpp"
cp "$SCRATCH/consumer/host.ini" "$SCRATCH/pkg-consumer/host.ini"

cmake -S "$SCRATCH/pkg-consumer" -B "$SCRATCH/pkg-consumer/build" \
    -DCMAKE_PREFIX_PATH="$SCRATCH/prefix" >/dev/null
cmake --build "$SCRATCH/pkg-consumer/build" --config "$CONFIG" --parallel >/dev/null

CONSUMER2="$(find "$SCRATCH/pkg-consumer/build" -name 'myworld.exe' -type f | head -1)"

if [ -z "$CONSUMER2" ]; then
    CONSUMER2="$(find "$SCRATCH/pkg-consumer/build" -name 'myworld' -type f | head -1)"
fi

if [ -z "$CONSUMER2" ]; then
    echo "FAIL — the find_package consumer did not build"
    exit 1
fi

OUTPUT2="$(cd "$SCRATCH/pkg-consumer" && "$CONSUMER2")"

if echo "$OUTPUT2" | grep -q "the loop runs"; then
    echo "      OK — installed package linked, ran"
else
    echo "FAIL — find_package output:"
    echo "$OUTPUT2"
    exit 1
fi

echo
echo "LCE packaging gate: both Embedding paths proven."
