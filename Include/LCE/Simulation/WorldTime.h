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
// │       “A day remembered is a day that mattered; a day forgotten is a day lost.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      WorldTime.h
//
// Purpose:
//
//      Defines the world calendar — the day counter that anchors
//      memories to world time, and the seasons derived from it (0.5.0).
//      The substrate 0.7.0 Legacy stands on: entities remember decades,
//      not ticks.
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

#include <cstdint>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // WorldTime
    //
    // The world's calendar, in days. The adapter drives it from the
    // game's clock and passes it to Remember/ReportOutcome, which stamp
    // memories with the day they happened. An input, never global state
    // (ADR-0014).
    //-------------------------------------------------------------------------
    struct WorldTime
    {
        std::uint64_t Day = 0;
    };

    //-------------------------------------------------------------------------
    // Season — the calendar's rhythm. Derived, never stored.
    //-------------------------------------------------------------------------
    enum class Season
    {
        Spring,
        Summer,
        Autumn,
        Winter
    };

    //-------------------------------------------------------------------------
    // The season for a world day: four seasons of 90 days in a 360-day
    // year. Pure — a function of the day alone.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    inline Season SeasonOf(std::uint64_t day) noexcept
    {
        switch ((day / 90) % 4)
        {
        case 0:
            return Season::Spring;
        case 1:
            return Season::Summer;
        case 2:
            return Season::Autumn;
        default:
            return Season::Winter;
        }
    }
}
