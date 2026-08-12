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
// │     “In the end, every choice is a story waiting to happen.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Behaviour.h
//
// Purpose:
//
//      Defines the behaviour layer: the action an entity can choose
//      (Intent) and the stateless decision function (Decide) that turns
//      needs, memory, relationships, and goals into one intent.
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

#include "LCE/Simulation/Entity/EntityId.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Goals.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Mind/Relationships.h"
#include "LCE/Simulation/Substrate/Rng.h"

#include <optional>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // ActionType
    //
    // The generic actions the core can choose. Acquiring food is expressed
    // as MoveTo a source; Flee means running from a remembered threat.
    //
    // The core never names a game action — MoveTo a target the adapter
    // resolves into \"walk along the road to town\".
    //-------------------------------------------------------------------------
    enum class ActionType
    {
        MoveTo,
        Rest,
        Socialize,
        Explore,
        Work,
        Flee
    };

    //-------------------------------------------------------------------------
    // Intent
    //
    // The behaviour output, used as a component: an entity with a Needs
    // component is a mind, and each tick its Intent is recomputed (or
    // removed if it has no decision). The adapter reads it and executes
    // the action in the game world.
    //-------------------------------------------------------------------------
    struct Intent
    {
        ActionType Action = ActionType::Explore;
        EntityId Target;
        float Confidence = 0.0f;   // how strongly the entity wants this
    };

    //-------------------------------------------------------------------------
    // Reads the entity's components and returns the action it most wants,
    // or nullopt when it has no drives or no decision this tick.
    // Stateless (ADR-0026): a pure function of data. When an Rng is
    // provided (0.5.0), the personality jitter comes from a child stream
    // derived from the entity's ID — same seed + same entity = same
    // jitter, regardless of iteration order. Nullptr keeps the
    // deterministic id-hash noise; existing callers are untouched.
    //
    // desperateHunger (0.7.0 field finding): below this hunger value a
    // remembered Trade world fact no longer blocks the trip — a starving
    // mind pushes the shut door anyway, so an arrival can land on a
    // closed market and the refusal can happen. 0.0 (the default) means
    // no mind is ever desperate: existing behavior, untouched.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    std::optional<Intent> Decide(
        const EntityRegistry& registry,
        EntityId id,
        const Rng* rng = nullptr,
        float desperateHunger = 0.0f);
}
