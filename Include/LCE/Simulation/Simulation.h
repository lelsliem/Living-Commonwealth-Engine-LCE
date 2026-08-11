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

namespace LCE::Events
{
    class EventBus;   // forward declaration — observation is an input, not a dependency
}

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Groups.h"
#include "LCE/Simulation/Memory.h"
#include "LCE/Simulation/Outcome.h"
#include "LCE/Simulation/WorldTime.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // BondThreshold
    //
    // One line the world drew across disposition (0.6.0 stone 08): a
    // name and the crossing point. "friend" at 0.3, "enemy" at -0.6 —
    // the vocabulary is the world's; the core knows only that a line it
    // was told about was crossed.
    //-------------------------------------------------------------------------
    struct BondThreshold
    {
        std::string Name;
        float Value = 0.0f;
    };

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
        float DispositionGain = 0.1f;    // aid and company warmth
        float DispositionLoss = 0.25f;   // wrongs and fights sour them
        float NeedJitter = 0.15f;        // per-mind metabolism spread (±15%)
        float GroupInheritance = 0.5f;   // how strongly a feeling reaches group-mates
        float HungerDesperate = 0.0f;    // below this hunger the closed sign is ignored

        // 0.8.0 Scale — bounding the hot path (stone 14a).
        std::size_t MemoryCap = 0;       // max events per mind; 0 = unbounded

        // 0.7.0 Legacy — what survives the entity (stones 10-12).
        float BequestFloor = 0.5f;       // salience below which a fact stays with the dead
        float InheritanceScale = 0.5f;   // secondhand stories are fainter than lived experience
        std::uint64_t LegacyMaxAgeDays = 0;  // inheritance age limit; 0 = any age

        // The bond watch-list (0.6.0 stone 08). Empty by default — the
        // world must name its own lines; a default here would invent
        // vocabulary the core has no business owning. When a disposition
        // crosses a listed line, RelationshipChangedEvent is published
        // (edge-triggered: the moment of crossing, then silent).
        std::vector<BondThreshold> BondThresholds;

        //-------------------------------------------------------------------------
        // Builds tuning from the Configuration service — the modder's
        // knob (0.5.0). Known keys ("sim.memory.fade", "sim.drift.rate",
        // "sim.jitter", ...) override the defaults above; a missing or
        // unparsable value keeps the default; unknown keys are ignored so
        // the adapter may carry its own keys in the same file. Pure
        // function — tuning stays an input, never global state (ADR-0014).
        //-------------------------------------------------------------------------
        static SimulationTuning FromConfiguration(
            const LCE::Config::Configuration& config);
    };

    //-------------------------------------------------------------------------
    // TickReport (0.8.0 stone 13)
    //
    // A lightweight, opt-in measurement of one Update call: per-pass
    // counts and wall time, so the cost of a settlement is knowable
    // instead of guessed. The adapter passes a pointer when it wants the
    // numbers; nullptr (the default) means the tick measures nothing —
    // every existing caller is untouched.
    //-------------------------------------------------------------------------
    struct TickReport
    {
        std::uint64_t Entities = 0;        // minds swept by the decay pass
        std::uint64_t MemoryEvents = 0;    // events examined by the fade pass
        std::uint64_t Relationships = 0;   // pairs drifted toward neutral
        double NeedsMs = 0.0;
        double MemoryMs = 0.0;
        double RelationshipsMs = 0.0;
        double GoalsMs = 0.0;
        double DecideMs = 0.0;
        double TotalMs = 0.0;
    };

    //-------------------------------------------------------------------------
    // Advances the world by deltaSeconds: decays needs, fades memory,
    // drifts relationships toward neutral, grows goal urgency, then
    // decides one Intent per mind.
    //
    // Stateless: time and tuning are inputs, never global state
    // (ADR-0014). The adapter calls this each game tick (0.4.0); tests
    // call it directly. When a non-null TickReport* is passed (0.8.0), the
    // per-pass counts and wall time are filled; nullptr measures nothing.
    //-------------------------------------------------------------------------
    void Update(
        EntityRegistry& registry,
        double deltaSeconds,
        const SimulationTuning& tuning = {},
        LCE::Events::EventBus* events = nullptr,
        const Rng* rng = nullptr,
        TickReport* report = nullptr);

    //-------------------------------------------------------------------------
    // FixedStep (0.8.0 stone 14b)
    //
    // The timing-independent tick. The adapter feeds real frame deltas
    // (which vary); the sim advances in whole fixed steps (which don't) —
    // same seed + same steps = same world, whatever the frame rate. This
    // is what makes "a year of sim time passes without drift" provable:
    // variable frame deltas were the #1 drift source, and fixed steps
    // remove them by construction.
    //
    // Update(delta) remains the raw primitive — every existing caller is
    // untouched. This is an opt-in composition helper; the host chooses
    // the cadence (Step, default 0.1s) and feeds it a frame delta each
    // frame. Advance returns how many whole steps ran, so the caller
    // knows how many intents were produced this frame.
    //-------------------------------------------------------------------------
    struct FixedStep
    {
        double Remaining = 0.0;   // partial step carried over
        double Step = 0.1;        // sim cadence, in seconds

        std::size_t Advance(
            double frameDelta,
            EntityRegistry& registry,
            const SimulationTuning& tuning = {},
            LCE::Events::EventBus* events = nullptr,
            const Rng* rng = nullptr,
            TickReport* report = nullptr);
    };

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
        const SimulationTuning& tuning = {},
        WorldTime time = {},
        LCE::Events::EventBus* events = nullptr);

    //-------------------------------------------------------------------------
    // Observation events (0.5.0): when a non-null EventBus is passed, the
    // tick publishes IntentProducedEvent for every fresh decision. Push,
    // not poll — the bus is an input, never global state (ADR-0014).
    //
    // Seeded determinism (0.5.0): when an Rng is passed, Decide draws its
    // personality jitter from per-entity child streams derived from the
    // ID — same seed resumes the exact same world, order-independent.
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Reports how an executed intent actually went — the observe leg of
    // the living loop (0.5.0). The adapter calls this after acting on an
    // Intent; the simulation then:
    //
    //   1. Records the memory (weight carries, fades, reinforces).
    //   2. Scales relationship effects by the result — for the positive
    //      kinds (Trade, Aid, Social) a Success builds trust/affection
    //      while a Failure loses it (the merchant proved unreliable); for
    //      the negative kinds (Wronged, Combat) a wrong is a wrong, full
    //      loss either way.
    //   3. Serves or frustrates the active goal — a Success clears a goal
    //      the kind serves (Trade feeds AcquireFood/Prosper, Social/Aid
    //      feed Socialize, Combat/Wronged feed ReachSafety), a Partial
    //      halves its urgency, a Failure leaves it to the tick's growth.
    //   4. Consumes the intent, so the next tick decides fresh with the
    //      outcome's memory in place.
    //
    // World outcomes (invalid Other) record memory only — no relationship
    // is shaped, no goal is served. Stateless: outcome and tuning are
    // inputs, never global state (ADR-0014).
    //-------------------------------------------------------------------------
    void ReportOutcome(
        EntityRegistry& registry,
        EntityId id,
        const Outcome& outcome,
        const SimulationTuning& tuning = {},
        LCE::Events::EventBus* events = nullptr,
        WorldTime time = {});

    //-------------------------------------------------------------------------
    // InheritGroupAttitudes
    //
    // Seeds a newcomer's feelings from the group's collective experience
    // (0.6.0 stone 09): the newcomer's disposition toward everyone the
    // group collectively knows becomes the group's mean disposition —
    // then their own experiences diverge it. Personal knowledge always
    // beats inherited: an existing relationship is left untouched. Trust
    // is never inherited — trust is earned personally. Quiet by design:
    // seeding is not an event (the same rule as drift). The world calls
    // this after adding the membership; the group's mean is derived from
    // the members' relationship stores, never stored separately.
    //-------------------------------------------------------------------------
    void InheritGroupAttitudes(
        EntityRegistry& registry,
        EntityId id,
        GroupId group);

    //-------------------------------------------------------------------------
    // Bequeath (0.7.0 stone 10 — death lifecycle)
    //
    // What an entity bequeaths as it goes. The world names the heirs
    // (the adapter knows family, household, settlement); the core keeps
    // what salience merits: the dying entity's MemoryEvents at or above
    // tuning.BequestFloor are copied into each heir's Memory, scaled by
    // tuning.InheritanceScale, carrying their original world day — the
    // story's age survives the transfer ("the feud is decades old").
    //
    // Append, never overwrite: the heir's own memories are untouched.
    // Deterministic: heirs are processed in ascending EntityId order,
    // so the caller's list order can never leak into results (the
    // QueryWhere discipline). Returns how many facts were bequeathed;
    // no event — the world called the death, it needs no announcement
    // back.
    //-------------------------------------------------------------------------
    std::size_t Bequeath(
        EntityRegistry& registry,
        EntityId dying,
        std::span<const EntityId> heirs,
        const SimulationTuning& tuning = {});

    //-------------------------------------------------------------------------
    // InheritMemory (0.7.0 stone 11 — generational handoff)
    //
    // Descendants inherit memory, selectively. The world's predicate
    // chooses which facts travel (nullptr = all); the core scales
    // (tuning.InheritanceScale — a story heard is fainter than a life
    // lived) and ages (tuning.LegacyMaxAgeDays filters facts older
    // than the world's patience; 0 keeps everything; unstamped or
    // future-dated facts pass — age is a property of the story, not
    // the hearer). The heir's own memories are never touched.
    //-------------------------------------------------------------------------
    std::size_t InheritMemory(
        EntityRegistry& registry,
        EntityId heir,
        EntityId ancestor,
        const SimulationTuning& tuning = {},
        WorldTime time = {},
        bool (*accept)(const MemoryEvent&) = nullptr);
}
