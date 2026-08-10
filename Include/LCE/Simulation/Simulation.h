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

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Memory.h"

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // SimulationTuning
    //
    // Every tuning constant of the living world, in one place. Defaults
    // are the 0.3.0 tuning; the 0.4.0 adapter will build this from the
    // Configuration service. Tuning is an *input*, never global state
    // (ADR-0014) — the tick stays a pure function of its arguments.
    //-------------------------------------------------------------------------
    struct SimulationTuning
    {
        float MemoryFadeRate = 0.2f;     // salience lost per second
        float ForgetThreshold = 0.1f;    // forgotten below this weight
        float DriftRate = 0.05f;         // feelings drift toward neutral
        float GoalUrgencyRate = 0.1f;    // urgency gained per second
        float TrustGain = 0.15f;         // a fair trade proves reliability
        float DispositionGain = 0.1f;    // aid and company warm feelings
        float DispositionLoss = 0.25f;   // wrongs and fights sour them

        //-------------------------------------------------------------------------
        // Builds tuning from the Configuration service — the modder's
        // knob (0.5.0). Known keys ("sim.memory.fade", "sim.drift.rate",
        // ...) override the defaults above; a missing or unparsable value
        // keeps the default; unknown keys are ignored so the adapter may
        // carry its own keys in the same file. Pure function — tuning
        // stays an input, never global state (ADR-0014).
        //-------------------------------------------------------------------------
        static SimulationTuning FromConfiguration(
            const LCE::Config::Configuration& config);
    };

    //-------------------------------------------------------------------------
    // Advances the world by deltaSeconds: decays needs, fades memory,
    // drifts relationships toward neutral, grows goal urgency, then
    // decides one Intent per mind.
    //
    // Stateless: time and tuning are inputs, never global state
    // (ADR-0014). The adapter calls this each game tick (0.4.0); tests
    // call it directly.
    //-------------------------------------------------------------------------
    void Update(
        EntityRegistry& registry,
        double deltaSeconds,
        const SimulationTuning& tuning = {});

    //-------------------------------------------------------------------------
    // Records an experience for the entity: appends it to Memory and
    // applies its effect on Relationships (trust grows with fair trade,
    // affection with aid, distrust with wrongs).
    //
    // This is also the channel for world facts: the adapter pushes a
    // memory event with an *invalid* Other — \"the market is closed
    // today\" is { invalid, Trade, weight }. While such a fact is
    // remembered, interactions of that kind are unavailable to the mind;
    // when the fact fades below the forget threshold, the market reopens.
    // The core reasons over facts without ever querying the world.
    //-------------------------------------------------------------------------
    void Remember(
        EntityRegistry& registry,
        EntityId id,
        const MemoryEvent& event,
        const SimulationTuning& tuning = {});
}
