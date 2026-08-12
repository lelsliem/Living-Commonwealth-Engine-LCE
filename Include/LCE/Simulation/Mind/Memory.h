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

#include "LCE/Simulation/Entity/EntityId.h"

#include <cstdint>
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
        Wronged,

        // Weather memory facts (0.5.x): the sky on a given day, one kind
        // per category so a fact is a label — "day 12 was rainy". These
        // are facts, never doors: Decide gates only Trade and Social,
        // and the Other is invalid, so no relationship is shaped.
        // Ordinals are append-only — the adapter co-save writes the raw
        // ordinal, so new kinds go at the end, never in the middle.
        WeatherClear,
        WeatherOvercast,
        WeatherRain,
        WeatherFog,
        WeatherMisty,
        WeatherRadstorm,

        // Death (0.6.0 — the adapter's Stone 1, "the world keeps its
        // books"): the fact that a mind is gone. A fact, never a door —
        // Decide gates only Trade and Social, so a death never blocks a
        // walk or a trade. The Other names who died; the Day stamps when.
        // Ordinal is append-only, like the weather kinds.
        Death
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

        // The world day this was remembered (0.5.0). Stamped by
        // Remember/ReportOutcome when a WorldTime is passed; 0 means
        // unstamped. The age of a fact is now.Day - event.Day — the
        // substrate 0.7.0 Legacy stands on ("entities remember decades").
        std::uint64_t Day = 0;
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
