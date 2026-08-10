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
// │      “The world doesn’t wait to be asked — it tells you what happened.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SimulationEvents.h
//
// Purpose:
//
//      Defines the observation events — the push channel through which
//      the simulation tells the outside world what happened, so games
//      react instead of polling (0.5.0).
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

#include "LCE/Events/Event.h"
#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityId.h"
#include "LCE/Simulation/Outcome.h"

#include <cstdint>
#include <string>
#include <utility>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // EntityCreatedEvent
    //
    // Published when the registry creates a genuinely new entity — not
    // when a snapshot restores one. The adapter hears about new minds as
    // they appear, without polling.
    //-------------------------------------------------------------------------
    struct EntityCreatedEvent : public LCE::Events::Event
    {
        explicit EntityCreatedEvent(EntityId id)
            : Id(id)
        {
        }

        EntityId Id;
    };

    //-------------------------------------------------------------------------
    // IntentProducedEvent
    //
    // Published when a mind's decision is recomputed during the tick. The
    // adapter reads the intent and executes it in the game world — push,
    // not poll.
    //-------------------------------------------------------------------------
    struct IntentProducedEvent : public LCE::Events::Event
    {
        IntentProducedEvent(EntityId id, Intent intent)
            : Id(id)
            , Intent(std::move(intent))
        {
        }

        EntityId Id;
        Intent Intent;
    };

    //-------------------------------------------------------------------------
    // OutcomeRecordedEvent
    //
    // Published when an executed intent's outcome is reported back into
    // the simulation. The adapter can react to the result immediately —
    // a robbed settler, a failed trade — or simply let memory carry it.
    //-------------------------------------------------------------------------
    struct OutcomeRecordedEvent : public LCE::Events::Event
    {
        OutcomeRecordedEvent(EntityId id, Outcome outcome)
            : Id(id)
            , Outcome(std::move(outcome))
        {
        }

        EntityId Id;
        Outcome Outcome;
    };

    //-------------------------------------------------------------------------
    // RelationshipChangedEvent
    //
    // Published when a relationship's disposition crosses one of the
    // bond thresholds the world configured (sim.bond.threshold.<name>,
    // 0.6.0 stone 08). Edge-triggered: the event fires the moment the
    // line is crossed — a bond formed, a bond soured — and stays silent
    // while the relationship rests on either side. The threshold's name
    // is the world's vocabulary ("friend", "enemy"); the core knows
    // only that a line the world configured was crossed.
    //-------------------------------------------------------------------------
    struct RelationshipChangedEvent : public LCE::Events::Event
    {
        RelationshipChangedEvent(
            EntityId subject,
            EntityId other,
            float disposition,
            float trust,
            std::string threshold,
            std::uint64_t day)
            : Subject(subject)
            , Other(other)
            , Disposition(disposition)
            , Trust(trust)
            , Threshold(std::move(threshold))
            , Day(day)
        {
        }

        EntityId Subject;
        EntityId Other;
        float Disposition;
        float Trust;
        std::string Threshold;   // the line crossed, by the world's name
        std::uint64_t Day;       // the world day of the crossing
    };
}
