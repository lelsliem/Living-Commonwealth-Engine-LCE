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
