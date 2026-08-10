//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      TestRunner.cpp
//
// Purpose:
//
//      Runs every LCE core suite by name and reports PASS/FAIL for each.
//      The harness is a dev-time tool: it never ships. From 0.4.0 on, the
//      real test is Fallout 4 itself.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LoggingTest.h"
#include "EventBusTest.h"
#include "ClockTest.h"
#include "SchedulerTest.h"
#include "TaskTest.h"
#include "ServiceRegistryTest.h"
#include "EntityRegistryTest.h"
#include "NeedsTest.h"
#include "MemoryTest.h"
#include "RelationshipsTest.h"
#include "GoalsTest.h"
#include "BehaviourTest.h"
#include "SimulationTickTest.h"
#include "SnapshotTest.h"
#include "TuningTest.h"
#include "OutcomeTest.h"
#include "ObservationTest.h"
#include "QueryTest.h"
#include "RngTest.h"

#include <cstdio>

namespace
{
    // Every suite is a bool-returning function: true means passed. The
    // table is data — adding a suite is adding one row.
    struct Suite
    {
        const char* Name;
        bool (*Run)();
    };

    constexpr Suite Suites[] = {
        { "Logging",         LCE::Tests::LoggingTest },
        { "EventBus",        LCE::Tests::EventBusTest },
        { "Clock",           LCE::Tests::ClockTest },
        { "Scheduler",       LCE::Tests::SchedulerTest },
        { "Task",            LCE::Tests::TaskTest },
        { "ServiceRegistry", LCE::Tests::ServiceRegistryTest },
        { "EntityRegistry",  LCE::Tests::EntityRegistryTest },
        { "Needs",           LCE::Tests::NeedsTest },
        { "Memory",          LCE::Tests::MemoryTest },
        { "Relationships",   LCE::Tests::RelationshipsTest },
        { "Goals",           LCE::Tests::GoalsTest },
        { "Behaviour",       LCE::Tests::BehaviourTest },
        { "SimulationTick",  LCE::Tests::SimulationTickTest },
        { "Snapshot",        LCE::Tests::SnapshotTest },
        { "Tuning",          LCE::Tests::TuningTest },
        { "Outcome",         LCE::Tests::OutcomeTest },
        { "Observation",     LCE::Tests::ObservationTest },
        { "Query",           LCE::Tests::QueryTest },
        { "Rng",             LCE::Tests::RngTest },
    };

    constexpr int kSuiteCount =
        static_cast<int>(sizeof(Suites) / sizeof(Suites[0]));
}

int main()
{
    int passed = 0;

    for (const auto& suite : Suites)
    {
        std::printf("[ RUN  ] %s\n", suite.Name);

        if (suite.Run())
        {
            std::printf("[  OK  ] %s\n", suite.Name);
            ++passed;
        }
        else
        {
            std::printf("[ FAIL ] %s\n", suite.Name);
        }
    }

    std::printf("\n%d/%d suites passed.\n", passed, kSuiteCount);

    return passed == kSuiteCount ? 0 : 1;
}
