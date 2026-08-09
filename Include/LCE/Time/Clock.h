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
// │                “The key is in not spending time, but in investing it.” – Stephen R. Covey
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
//
// File:
//
//      Clock.h
//
// Purpose:
//
//      Defines the simulation clock used to measure elapsed LCE runtime time.
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

namespace LCE::Time
{
    class Clock
    {
    public:
        using Duration = std::chrono::duration<double>;

        Clock() noexcept
            : m_StartTime(std::chrono::steady_clock::now())
        {
        }

        ~Clock() = default;

        Duration Elapsed() const noexcept;

    private:
        std::chrono::steady_clock::time_point m_StartTime;
    };
}