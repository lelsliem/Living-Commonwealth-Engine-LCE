//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │
// │                       ██╗      ██████╗███████╗
// │                       ██║     ██╔════╝██╔════╝
// │                       ██║     ██║     █████╗
// │                       ██║     ██║     ██╔══╝
// │                       ███████╗╚██████╗███████╗
// │                       ╚═════╝  ╚═════╝╚══════╝
// │
// │            Building living worlds through simulation.
// │
// │                “My calendar and I are in a toxic relationship — it keeps ghosting my free time."
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Scheduler.cpp
//
// Purpose:
//
//      Implements the LCE scheduler.
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

#include <utility>

namespace LCE::Scheduling
{
    void Scheduler::Update(Duration delta) noexcept
    {
        // Phase one: tick every task and collect the callbacks that are due.
        //
        // The due callbacks are moved out of the vector and invoked only
        // AFTER the pass finishes. A callback may call Schedule() while it
        // runs, and std::vector::push_back can reallocate, invalidating any
        // iterator we still hold. By never touching m_Tasks while a callback
        // executes, the loop stays safe no matter what a callback does.
        // (Remove this separation and run the reentrancy test: that is the
        // classic iterator-invalidation bug.)
        std::vector<Callback> due;

        for (auto iterator = m_Tasks.begin();
             iterator != m_Tasks.end();)
        {
            iterator->Remaining -= delta;

            if (iterator->Remaining <= Duration::zero())
            {
                // Move, don't copy: copying a std::function can allocate.
                due.push_back(std::move(iterator->Function));

                iterator = m_Tasks.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }

        // Phase two: run the due callbacks. A task scheduled by a callback
        // begins counting down on the next Update call — never during the
        // pass that created it.
        for (auto& callback : due)
        {
            callback();
        }
    }

    void Scheduler::Schedule(
        Duration delay,
        Callback callback)
    {
        m_Tasks.push_back({
            delay,
            std::move(callback)
        });
    }
}