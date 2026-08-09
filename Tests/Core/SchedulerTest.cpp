//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SchedulerTest.cpp
//
// Purpose:
//
//      Verifies that the LCE scheduler executes callbacks after their
//      scheduled delay.
//
// Project:
//
//      Living Commonwealth Engine (LCE)
//
// License:
//
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//
//      (c) 2026-present LCE Contributors
//=============================================================================//

#include "LCE/Scheduling/Scheduler.h"

namespace LCE::Tests
{
    bool SchedulerTest()
    {
        Scheduling::Scheduler scheduler;

        bool executed = false;

        scheduler.Schedule(
            Scheduling::Scheduler::Duration{1.0},
            [&executed]()
            {
                executed = true;
            });

        scheduler.Update(
            Scheduling::Scheduler::Duration{0.5});

        if (executed)
        {
            return false;
        }

        scheduler.Update(
            Scheduling::Scheduler::Duration{0.5});

        return executed;
    }
}