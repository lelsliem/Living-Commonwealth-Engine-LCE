#==============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#==============================================================================

# LCE Doctor — the 0.5.0 SDK CLI. Point it at a project; it validates
# the SDK contract (build system, LCE wiring, core checkout, toolchain)
# and prints a plain pass/fail log. Its checks live in Checks.h and are
# covered by the core harness's Doctor suite.
add_executable(LCEDoctor
    LCEDoctor/main.cpp
)

target_compile_features(LCEDoctor
    PRIVATE
        cxx_std_23
)

# CMAKE_CURRENT_SOURCE_DIR — not CMAKE_SOURCE_DIR, which is the
# consumer's root when the engine is embedded via FetchContent.
target_include_directories(LCEDoctor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# LCE Bench (0.8.7) — the Scale numbers, measured on this machine: tick
# cost at population, co-save bytes per mind, round-trip, determinism at
# scale, the memory cap. Same seed, same scenarios, same methodology as
# the Scale suite — so the 0.9.0 gate ("performance at scale verified")
# is measurable, not asserted. --sanity runs a tiny scenario for CI.
add_executable(LCE.Bench
    LCEBench/main.cpp
)

target_link_libraries(LCE.Bench
    PRIVATE
        LCE.Core
)

target_compile_features(LCE.Bench
    PRIVATE
        cxx_std_23
)

# LCE Studio (0.8.9) — the observation window. A zero-dependency GUI:
# a tiny HTTP server on 127.0.0.1 (std sockets) and one embedded HTML
# page; the browser is the window. Consumer-only through the public
# API — the same shape the bench proved, with a page. The sim ticks
# on a thread at 20 Hz; the page watches the bus. --selftest runs one
# internal request and exits — CI's smoke that the tool runs.
add_executable(LCE.Studio
    LCEStudio/main.cpp
)

target_link_libraries(LCE.Studio
    PRIVATE
        LCE.Core
)

target_compile_features(LCE.Studio
    PRIVATE
        cxx_std_23
)

if(WIN32)
    # Winsock is the socket shim on Windows; MSVC's #pragma comment
    # links it, but declare it explicitly for other Windows toolchains.
    target_link_libraries(LCE.Studio
        PRIVATE
            ws2_32
    )
endif()
