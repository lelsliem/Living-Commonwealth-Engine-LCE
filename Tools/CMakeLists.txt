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

target_include_directories(LCEDoctor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Tools
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
