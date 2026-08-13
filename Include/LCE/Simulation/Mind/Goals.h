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
// │            “A world is more than its map — it’s its behaviour over time.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Goals.h
//
// Purpose:
//
//      Defines the Goals component — the long horizon. Needs are urgent
//      and short-term; goals are ambitions that persist.
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

#include <optional>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // GoalType
    //
    // The generic ambitions the core understands. Richer goal planning is
    // deliberately deferred; 0.3.0 keeps one active ambition per entity.
    //-------------------------------------------------------------------------
    enum class GoalType
    {
        AcquireFood,
        ReachSafety,
        Socialize,
        Prosper
    };

    //-------------------------------------------------------------------------
    // Goal
    //
    // Urgency grows while the goal goes unserved. Decide does NOT read
    // Goals directly — it stays a pure function of needs (Behaviour.h);
    // goals shape behaviour through the world's planning layer, which
    // reads Goals::Active and biases needs before the tick (the same
    // channel Weather and Disease use). ReportOutcome serves and
    // frustrates goals; the ambition itself is the world's to own.
    //-------------------------------------------------------------------------
    struct Goal
    {
        GoalType Type = GoalType::AcquireFood;
        float Urgency = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Goals
    //
    // Minimal by decision: at most one active goal per entity.
    //-------------------------------------------------------------------------
    struct Goals
    {
        std::optional<Goal> Active;
    };
}
