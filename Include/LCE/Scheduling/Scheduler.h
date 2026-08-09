//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │
// │                       ██╗      ██████╗███████╗
// │                       ██║     ██╔════╝██╔════╝
// │                       ██║     ██║     █████╗
// │                       ██║     ██║     ██╔══╝
// │                       ███████╗╚██████╗███████╗
// │                       ╚══════╝ ╚═════╝╚══════╝
// │
// │            Building living worlds through simulation.
// │
// │                "A schedule defends from chaos and whim."
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Scheduler.h
//
// Purpose:
//
//      Defines the scheduler used to execute work after a specified delay.
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

#pragma once

#include <chrono>
#include <functional>
#include <vector>

namespace LCE::Scheduling
{
    class Scheduler
    {
    public:
        using Duration = std::chrono::duration<double>;
        using Callback = std::function<void()>;

        //-------------------------------------------------------------------------
        // Advances the scheduler by delta and fires every task whose delay
        // has elapsed.
        //
        // Call once per simulation tick. Due callbacks fire in scheduling
        // order, AFTER the pass finishes: a callback that calls Schedule()
        // has its new task count down from the next Update call. Safe no
        // matter what a callback does.
        //-------------------------------------------------------------------------
        void Update(Duration delta) noexcept;

        //-------------------------------------------------------------------------
        // Schedules callback to run no earlier than delay from now.
        // Order is preserved: equal delays run in scheduling order.
        //-------------------------------------------------------------------------
        void Schedule(
            Duration delay,
            Callback callback);

    private:
        struct ScheduledTask
        {
            Duration Remaining;
            Callback Function;
        };

        std::vector<ScheduledTask> m_Tasks;
    };
}