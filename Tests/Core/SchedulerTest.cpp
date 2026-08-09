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

        if (!executed)
        {
            return false;
        }

        // Reentrancy: a callback that schedules new work must not corrupt
        // the Update loop. The new task counts down on the next Update,
        // never during the pass that created it.
        Scheduling::Scheduler reentrant;

        int secondRun = 0;

        reentrant.Schedule(
            Scheduling::Scheduler::Duration{1.0},
            [&reentrant, &secondRun]()
            {
                reentrant.Schedule(
                    Scheduling::Scheduler::Duration{0.0},
                    [&secondRun]()
                    {
                        ++secondRun;
                    });
            });

        reentrant.Update(
            Scheduling::Scheduler::Duration{1.0});

        if (secondRun != 0)
        {
            return false;   // must not fire during the same pass
        }

        reentrant.Update(
            Scheduling::Scheduler::Duration{0.0});

        if (secondRun != 1)
        {
            return false;   // fires on the next tick
        }

        return true;
    }
}