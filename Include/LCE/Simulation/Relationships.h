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
// │       “Social systems: the original source of unpredictable behaviour.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Relationships.h
//
// Purpose:
//
//      Defines the Relationships component — how an entity feels about the
//      others it has met.
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

#include <unordered_map>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // Relationship
    //
    // How one entity feels about another. Disposition is the valence
    // (-1 hate ... +1 love); Trust is how reliable they have proven to be.
    // Both drift toward neutral over time and shift with remembered
    // interactions.
    //-------------------------------------------------------------------------
    struct Relationship
    {
        float Disposition = 0.0f;
        float Trust = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Relationships
    //
    // The component. The farmer chooses the merchant they know and trust —
    // not a stranger.
    //-------------------------------------------------------------------------
    struct Relationships
    {
        std::unordered_map<EntityId, Relationship> ByEntity;
    };
}
