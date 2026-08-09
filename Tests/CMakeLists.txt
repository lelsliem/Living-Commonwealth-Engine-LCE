#==============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#==============================================================================

# LCE Core test harness — every subsystem has a suite; runs on every build.
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
)

target_link_libraries(LCE.Core.Tests
    PRIVATE
        LCE.Core
)

target_compile_features(LCE.Core.Tests
    PRIVATE
        cxx_std_23
)