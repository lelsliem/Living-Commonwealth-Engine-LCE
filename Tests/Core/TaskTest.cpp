//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      TaskTest.cpp
//
// Purpose:
//
//      Verifies that the LCE task system executes its callback when invoked
//      and safely tolerates an empty callback.
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

#include "LCE/Tasks/Task.h"

namespace LCE::Tests
{
    bool TaskTest()
    {
        int callCount = 0;

        Tasks::Task task(
            [&callCount]()
            {
                ++callCount;
            });

        task.Execute();

        if (callCount != 1)
        {
            return false;
        }

        // An empty callback must not crash.
        Tasks::Task empty(Tasks::Task::Callback{});

        empty.Execute();

        return true;
    }
}
