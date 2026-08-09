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
//      Runs the LCE core subsystem tests.
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

#include <iostream>

int main()
{
    bool success = true;

    success &= LCE::Tests::LoggingTest();
    success &= LCE::Tests::EventBusTest();
    success &= LCE::Tests::ClockTest();
    success &= LCE::Tests::SchedulerTest();
    success &= LCE::Tests::TaskTest();
    success &= LCE::Tests::ServiceRegistryTest();
    success &= LCE::Tests::EntityRegistryTest();

    if (success)
    {
        std::cout << "All LCE Core tests passed.\n";
        return 0;
    }

    std::cout << "LCE Core tests failed.\n";
    return 1;
}