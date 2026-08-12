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
// │             “Every outcome is a memory waiting to become a lesson.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Outcome.h
//
// Purpose:
//
//      Defines the outcome — the adapter's structured report of how an
//      executed intent actually went, and the raw material of learning.
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
#include "LCE/Simulation/Mind/Memory.h"

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // OutcomeResult
    //
    // How the executed action actually went. The result decides whether
    // the counterparty proved reliable — a successful trade builds trust,
    // a failed one loses it.
    //-------------------------------------------------------------------------
    enum class OutcomeResult
    {
        Success,
        Partial,
        Failure
    };

    //-------------------------------------------------------------------------
    // Outcome
    //
    // The adapter's report on an executed intent: who was involved, what
    // kind of interaction was attempted, and how it went. The simulation
    // turns it into memory, relationship change, goal service, and a fresh
    // decision — the observe leg of decide → act → observe → remember →
    // decide (0.5.0).
    //
    // Other may be invalid for world outcomes ("the road was blocked") —
    // such outcomes record memory only and shape nothing else.
    //-------------------------------------------------------------------------
    struct Outcome
    {
        EntityId Other;
        InteractionKind Kind = InteractionKind::Social;
        OutcomeResult Result = OutcomeResult::Failure;
        float Weight = 1.0f;
    };
}
