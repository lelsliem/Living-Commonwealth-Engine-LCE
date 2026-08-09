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
// │           “Time is the canvas; simulation is the brush.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Simulation.h
//
// Purpose:
//
//      Defines the simulation tick — the heartbeat of the living world —
//      and the channel through which experience enters (Remember).
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

#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Memory.h"

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // Advances the world by deltaSeconds: decays needs, fades memory,
    // drifts relationships toward neutral, grows goal urgency, then
    // decides one Intent per mind.
    //
    // Stateless: time is an input, never global state (ADR-0014). The
    // adapter calls this each game tick (0.4.0); tests call it directly.
    //-------------------------------------------------------------------------
    void Update(
        EntityRegistry& registry,
        double deltaSeconds);

    //-------------------------------------------------------------------------
    // Records an experience for the entity: appends it to Memory and
    // applies its effect on Relationships (trust grows with fair trade,
    // affection with aid, distrust with wrongs).
    //
    // This is also the channel for world facts: the adapter pushes
    // \"the market is open today\" in as a memory event, and the core
    // reasons over it without ever querying the world.
    //-------------------------------------------------------------------------
    void Remember(
        EntityRegistry& registry,
        EntityId id,
        const MemoryEvent& event);
}
