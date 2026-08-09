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
// │            “What we remember shapes what we become — engines included.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Memory.h
//
// Purpose:
//
//      Defines the Memory component — what an entity has experienced, and
//      the raw material of its decisions.
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

#include "LCE/Simulation/EntityId.h"

#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // InteractionKind
    //
    // The generic categories of experience the core reasons over. The
    // adapter reports events; the simulation gives them meaning.
    //-------------------------------------------------------------------------
    enum class InteractionKind
    {
        Trade,
        Combat,
        Aid,
        Social,
        Wronged
    };

    //-------------------------------------------------------------------------
    // MemoryEvent
    //
    // One remembered experience. Weight is salience: how much it matters
    // and how strongly it is remembered. It fades with time and is
    // forgotten below a threshold; reinforcement restores it.
    //
    // Other may be invalid for world facts (\"the market is open today\").
    //-------------------------------------------------------------------------
    struct MemoryEvent
    {
        EntityId Other;
        InteractionKind Kind = InteractionKind::Social;
        float Weight = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Memory
    //
    // The component. No memory of the merchant, no reason to choose their
    // stall.
    //-------------------------------------------------------------------------
    struct Memory
    {
        std::vector<MemoryEvent> Events;
    };
}
