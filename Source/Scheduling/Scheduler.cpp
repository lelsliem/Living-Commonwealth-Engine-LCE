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
        for (auto iterator = m_Tasks.begin();
             iterator != m_Tasks.end();)
        {
            iterator->Remaining -= delta;

            if (iterator->Remaining <= Duration::zero())
            {
                auto callback = std::move(iterator->Function);

                iterator = m_Tasks.erase(iterator);

                callback();
            }
            else
            {
                ++iterator;
            }
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