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
// │            “Needs: because standing still is boring.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Needs.h
//
// Purpose:
//
//      Defines the Needs component — the urgent drives that decay over
//      time and push an entity toward action.
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

#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // NeedType
    //
    // The generic drives the core reasons over. The adapter maps them to
    // game mechanics — the core never knows what "hunger" means in-game.
    //-------------------------------------------------------------------------
    enum class NeedType
    {
        Hunger,
        Fatigue,
        Social,
        Safety,
        Comfort
    };

    //-------------------------------------------------------------------------
    // Need
    //
    // A single drive. Value decays toward 0 at DecayRate per second of
    // simulation time; the lower the value, the more urgent the need.
    //-------------------------------------------------------------------------
    struct Need
    {
        NeedType Type = NeedType::Hunger;
        float Value = 1.0f;       // 1 = satisfied ... 0 = deprived
        float DecayRate = 0.0f;   // per second of simulation time
    };

    //-------------------------------------------------------------------------
    // Needs
    //
    // The component. An entity with Needs is a mind: the simulation tick
    // decays its needs and decides an Intent for it. An entity without
    // Needs is untouched — a rock has no needs.
    //-------------------------------------------------------------------------
    struct Needs
    {
        std::vector<Need> List;
    };
}
