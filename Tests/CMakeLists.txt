#==============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#==============================================================================

# LCE Core test harness — every subsystem has a suite; runs on every build.
# ctest wiring (0.8.6): CI runs the suites through ctest, so the harness
# path is the runner's, not a hard-coded binary location. enable_testing()
# lives at the PROJECT ROOT (see CMakeLists.txt) — the top-level
# CTestTestfile.cmake is what ctest --test-dir Build reads.
add_test(NAME LCE.Core.Tests COMMAND LCE.Core.Tests)

add_executable(LCE.Core.Tests
    Core/TestRunner.cpp
    Core/LoggingTest.cpp
    Core/EventBusTest.cpp
    Core/ClockTest.cpp
    Core/SchedulerTest.cpp
    Core/TaskTest.cpp
    Core/ServiceRegistryTest.cpp
    Core/EntityRegistryTest.cpp
    Core/NeedsTest.cpp
    Core/MemoryTest.cpp
    Core/RelationshipsTest.cpp
    Core/GoalsTest.cpp
    Core/BehaviourTest.cpp
    Core/SimulationTickTest.cpp
    Core/SnapshotTest.cpp
    Core/TuningTest.cpp
    Core/OutcomeTest.cpp
    Core/ObservationTest.cpp
    Core/QueryTest.cpp
    Core/RngTest.cpp
    Core/WorldCalendarTest.cpp
    Core/JitterTest.cpp
    Core/DoctorTest.cpp
    Core/BondThresholdTest.cpp
    Core/GroupsTest.cpp
    Core/TraitsTest.cpp
    Core/BequeathTest.cpp
    Core/InheritanceTest.cpp
    Core/WorldLegacyTest.cpp
    Core/ScaleTest.cpp
    Core/SoakTest.cpp
    Core/HeaderMapTest.cpp
    Core/SurfaceTest.cpp
)

target_link_libraries(LCE.Core.Tests
    PRIVATE
        LCE.Core
)

target_compile_features(LCE.Core.Tests
    PRIVATE
        cxx_std_23
)

# The Doctor suite tests LCE Doctor's checks — the tools live beside
# the core, so the harness reaches them by include path. Relative to
# THIS file, never CMAKE_SOURCE_DIR: when the engine is embedded via
# FetchContent, CMAKE_SOURCE_DIR is the CONSUMER's root — the harness
# must resolve against its own tree wherever it is built.
target_include_directories(LCE.Core.Tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Tools
)

# HeaderMapTest must find the engine root without relying on the
# process working directory (ctest runs the binary from the build
# tree, not the source tree). The source root is compiled in; the
# suite's walk-up-from-cwd discovery remains as a fallback for ad-hoc
# runs.
target_compile_definitions(LCE.Core.Tests
    PRIVATE
        LCE_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)
