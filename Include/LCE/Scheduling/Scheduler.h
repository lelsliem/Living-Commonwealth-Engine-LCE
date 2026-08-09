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

        void Update(Duration delta) noexcept;

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