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
// │       “Simulation: because guessing the future every frame is fun.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Simulation.cpp
//
// Purpose:
//
//      Implements the simulation tick — the heartbeat of the living world.
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

#include "LCE/Simulation/Simulation.h"

#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/SimulationEvents.h"

#include <algorithm>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    //-------------------------------------------------------------------------
    // The magnitude and direction an outcome gives its kind's effect:
    // success counts fully, partial counts half, failure inverts the
    // positive kinds (a failed trade loses trust).
    //-------------------------------------------------------------------------
    float ResultScale(LCE::Simulation::OutcomeResult result) noexcept
    {
        switch (result)
        {
        case LCE::Simulation::OutcomeResult::Success:
            return 1.0f;

        case LCE::Simulation::OutcomeResult::Partial:
            return 0.5f;

        case LCE::Simulation::OutcomeResult::Failure:
            return -1.0f;
        }

        return 0.0f;
    }

    //-------------------------------------------------------------------------
    // ServesGoal
    //
    // The table that turns outcomes into ambition: a trade feeds
    // AcquireFood and Prosper, aid and company feed Socialize, danger
    // feeds ReachSafety. A kind that serves nothing cannot satisfy a
    // goal.
    //-------------------------------------------------------------------------
    bool ServesGoal(
        LCE::Simulation::InteractionKind kind,
        LCE::Simulation::GoalType goal) noexcept
    {
        switch (kind)
        {
        case LCE::Simulation::InteractionKind::Trade:
            return goal == LCE::Simulation::GoalType::AcquireFood
                || goal == LCE::Simulation::GoalType::Prosper;

        case LCE::Simulation::InteractionKind::Aid:
        case LCE::Simulation::InteractionKind::Social:
            return goal == LCE::Simulation::GoalType::Socialize;

        case LCE::Simulation::InteractionKind::Combat:
        case LCE::Simulation::InteractionKind::Wronged:
            return goal == LCE::Simulation::GoalType::ReachSafety;
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // PublishCrossings
    //
    // Edge-triggered bond-threshold events (0.6.0 stone 08): for every
    // threshold in the tuning's watch-list, if the disposition moved
    // across the line during this mutation — below to above, or above to
    // below — publish a RelationshipChangedEvent naming the line.
    // Crossing is strict: resting exactly on a line and drifting away is
    // not a crossing. The world names its own lines; the core only knows
    // a configured line was crossed.
    //-------------------------------------------------------------------------
    void PublishCrossings(
        LCE::Events::EventBus* events,
        const LCE::Simulation::SimulationTuning& tuning,
        LCE::Simulation::EntityId subject,
        LCE::Simulation::EntityId other,
        float before,
        float after,
        float trust,
        std::uint64_t day)
    {
        if (events == nullptr)
        {
            return;
        }

        for (const auto& threshold : tuning.BondThresholds)
        {
            const auto crossedUp =
                before < threshold.Value && after >= threshold.Value;
            const auto crossedDown =
                before > threshold.Value && after <= threshold.Value;

            if (crossedUp || crossedDown)
            {
                events->Publish(LCE::Simulation::RelationshipChangedEvent{
                    subject, other, after, trust, threshold.Name, day });
            }
        }
    }
}

namespace LCE::Simulation
{
    void Update(
        EntityRegistry& registry,
        double deltaSeconds,
        const SimulationTuning& tuning,
        LCE::Events::EventBus* events,
        const Rng* rng)
    {
        const auto delta = static_cast<float>(deltaSeconds);

        //-------------------------------------------------------------------------
        // Needs decay. Only entities WITH a Needs component are simulated —
        // a rock has no needs and is untouched.
        //
        // Per-mind metabolism (0.5.0): when a seeded Rng is present, each
        // entity's needs decay at its own rate, derived from its ID — the
        // same seed + the same entity yields the same rate every tick, and
        // the parent stream never advances. This is what breaks the herd:
        // identical minds no longer get hungry on the same clock. Without
        // an Rng the jitter is exactly 1.0 — behaviour unchanged, so no
        // existing caller is affected.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Needs>(
            [delta, rng, &tuning](EntityId id, Needs& needs)
            {
                const auto rate = (rng != nullptr)
                    ? rng->Derive(id.Value()).NextFloat(
                          1.0f - tuning.NeedJitter,
                          1.0f + tuning.NeedJitter)
                    : 1.0f;

                for (auto& need : needs.List)
                {
                    need.Value -= need.DecayRate * delta * rate;

                    if (need.Value < 0.0f)
                    {
                        need.Value = 0.0f;
                    }
                }
            });

        //-------------------------------------------------------------------------
        // Memory fades. Salience erodes each second; forgotten below the
        // threshold. Erase-while-iterating: safe because we never touch the
        // store inside the loop (same rule as the Scheduler).
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Memory>(
            [delta, &tuning](EntityId, Memory& memory)
            {
                for (auto iterator = memory.Events.begin();
                     iterator != memory.Events.end();)
                {
                    iterator->Weight -= tuning.MemoryFadeRate * delta;

                    if (iterator->Weight <= tuning.ForgetThreshold)
                    {
                        iterator = memory.Events.erase(iterator);
                    }
                    else
                    {
                        ++iterator;
                    }
                }
            });

        //-------------------------------------------------------------------------
        // Relationships drift toward neutral — feelings cool over time
        // unless experience refreshes them.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Relationships>(
            [delta, &tuning](EntityId, Relationships& relationships)
            {
                for (auto& entry : relationships.ByEntity)
                {
                    auto& relationship = entry.second;

                    relationship.Disposition +=
                        (0.0f - relationship.Disposition) * tuning.DriftRate * delta;
                    relationship.Trust +=
                        (0.0f - relationship.Trust) * tuning.DriftRate * delta;
                }
            });

        //-------------------------------------------------------------------------
        // Goals grow urgent while they go unserved.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Goals>(
            [delta, &tuning](EntityId, Goals& goals)
            {
                if (goals.Active)
                {
                    goals.Active->Urgency += tuning.GoalUrgencyRate * delta;
                }
            });

        //-------------------------------------------------------------------------
        // Decide: one Intent per mind. Two-phase on purpose — deciding may
        // AddComponent/RemoveComponent (mutating the Intent store), so we
        // first collect every decision, then apply them. Never mutate a
        // store while iterating it.
        //-------------------------------------------------------------------------
        std::vector<std::pair<EntityId, std::optional<Intent>>> decisions;

        registry.ForEachWithComponent<Needs>(
            [&registry, &decisions, rng](EntityId id, const Needs&)
            {
                decisions.emplace_back(id, Decide(registry, id, rng));
            });

        for (const auto& [id, intent] : decisions)
        {
            if (intent)
            {
                registry.AddComponent<Intent>(id, *intent);

                // Observation (0.5.0): every fresh decision is news — the
                // adapter executes it without polling.
                if (events != nullptr)
                {
                    events->Publish(IntentProducedEvent{ id, *intent });
                }
            }
            else
            {
                registry.RemoveComponent<Intent>(id);
            }
        }
    }

    void Remember(
        EntityRegistry& registry,
        EntityId id,
        const MemoryEvent& event,
        const SimulationTuning& tuning,
        WorldTime time,
        LCE::Events::EventBus* events)
    {
        if (!registry.IsAlive(id))
        {
            return;
        }

        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        // Anchor the memory to world time (0.5.0): stamp the day it
        // happened — unless the caller already set one, which wins (the
        // adapter may report a historical event while passing today).
        auto stamped = event;

        if (stamped.Day == 0 && time.Day != 0)
        {
            stamped.Day = time.Day;
        }

        memory->Events.push_back(stamped);

        // World facts have no other entity; they shape nothing here.
        if (!event.Other.IsValid())
        {
            return;
        }

        auto relationships = registry.GetComponent<Relationships>(id);

        if (!relationships)
        {
            registry.AddComponent<Relationships>(id, Relationships{});
            relationships = registry.GetComponent<Relationships>(id);
        }

        auto& relationship = relationships->ByEntity[event.Other];

        // Memories shape feelings: fair trade earns trust, aid warms,
        // wrongs and fights sour.
        const auto dispositionBefore = relationship.Disposition;

        switch (event.Kind)
        {
        case InteractionKind::Trade:
            relationship.Trust += tuning.TrustGain;
            break;

        case InteractionKind::Aid:
        case InteractionKind::Social:
            relationship.Disposition += tuning.DispositionGain;
            break;

        case InteractionKind::Wronged:
        case InteractionKind::Combat:
            relationship.Disposition -= tuning.DispositionLoss;
            break;
        }

        // Bond crossing (0.6.0 stone 08): an experience moved the
        // disposition across a line the world configured — the moment is
        // news. Drift is deliberately quiet: cooling below a line is a
        // dissolve, not an event; the adapter re-derives bonds from state.
        PublishCrossings(
            events, tuning, id, event.Other,
            dispositionBefore, relationship.Disposition,
            relationship.Trust, time.Day);
    }

    void ReportOutcome(
        EntityRegistry& registry,
        EntityId id,
        const Outcome& outcome,
        const SimulationTuning& tuning,
        LCE::Events::EventBus* events,
        WorldTime time)
    {
        if (!registry.IsAlive(id))
        {
            return;
        }

        //-------------------------------------------------------------------------
        // 1. Record the memory — the experience itself, anchored to the
        //    world day (0.5.0) unless the caller already stamped it.
        //-------------------------------------------------------------------------
        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        auto stamped = MemoryEvent{ outcome.Other, outcome.Kind, outcome.Weight };

        if (stamped.Day == 0 && time.Day != 0)
        {
            stamped.Day = time.Day;
        }

        memory->Events.push_back(stamped);

        //-------------------------------------------------------------------------
        // 2. Relationship effects, scaled by the result. World outcomes
        //    name no one — nothing to trust, nothing to shape.
        //-------------------------------------------------------------------------
        if (outcome.Other.IsValid())
        {
            auto relationships = registry.GetComponent<Relationships>(id);

            if (!relationships)
            {
                registry.AddComponent<Relationships>(id, Relationships{});
                relationships = registry.GetComponent<Relationships>(id);
            }

            auto& relationship = relationships->ByEntity[outcome.Other];

            const auto dispositionBefore = relationship.Disposition;

            switch (outcome.Kind)
            {
            case InteractionKind::Trade:
                // A successful trade proves reliability; a failed one
                // loses trust.
                relationship.Trust += tuning.TrustGain * ResultScale(outcome.Result);
                break;

            case InteractionKind::Aid:
            case InteractionKind::Social:
                // Company and aid warm when they work and cool when
                // they don't.
                relationship.Disposition +=
                    tuning.DispositionGain * ResultScale(outcome.Result);
                break;

            case InteractionKind::Wronged:
            case InteractionKind::Combat:
                // A wrong is a wrong, however it went — full loss.
                relationship.Disposition -= tuning.DispositionLoss;
                break;
            }

            // Bond crossing (0.6.0 stone 08) — same edge-triggered rule
            // as Remember: the moment a line is crossed, not the resting
            // state beside it.
            PublishCrossings(
                events, tuning, id, outcome.Other,
                dispositionBefore, relationship.Disposition,
                relationship.Trust, time.Day);
        }

        //-------------------------------------------------------------------------
        // 3. Serve or frustrate the active goal. Only a real interaction
        //    with someone can satisfy ambition; a world outcome cannot.
        //-------------------------------------------------------------------------
        if (outcome.Other.IsValid())
        {
            auto goals = registry.GetComponent<Goals>(id);

            if (goals && goals->Active
                && ServesGoal(outcome.Kind, goals->Active->Type))
            {
                switch (outcome.Result)
                {
                case OutcomeResult::Success:
                    goals->Active.reset();   // ambition served
                    break;

                case OutcomeResult::Partial:
                    goals->Active->Urgency *= 0.5f;   // half served
                    break;

                case OutcomeResult::Failure:
                    break;   // the tick's growth speaks for itself
                }
            }
        }

        //-------------------------------------------------------------------------
        // 4. Consume the intent — the action concluded. The next tick
        //    decides fresh, with the outcome's memory in place.
        //-------------------------------------------------------------------------
        registry.RemoveComponent<Intent>(id);

        // Observation (0.5.0): the result is news — the adapter can react
        // immediately (a robbed settler, a failed trade) without polling.
        if (events != nullptr)
        {
            events->Publish(OutcomeRecordedEvent{ id, outcome });
        }
    }

    SimulationTuning SimulationTuning::FromConfiguration(
        const LCE::Config::Configuration& config)
    {
        // The modder's knob (0.5.0): each known key overrides the
        // corresponding default. Missing, empty, or unparsable values
        // keep the default — a broken line must never break the world.
        // Unknown keys are ignored, so the adapter can share one file.
        const auto read = [&config](std::string_view key, float fallback)
        {
            const auto raw = config.Get(key);

            if (raw.empty())
            {
                return fallback;
            }

            try
            {
                return std::stof(std::string(raw));
            }
            catch (const std::exception&)
            {
                return fallback;
            }
        };

        SimulationTuning tuning;

        tuning.MemoryFadeRate = read("sim.memory.fade", tuning.MemoryFadeRate);
        tuning.ForgetThreshold = read("sim.memory.forget", tuning.ForgetThreshold);
        tuning.DriftRate = read("sim.drift.rate", tuning.DriftRate);
        tuning.GoalUrgencyRate = read("sim.goal.urgency", tuning.GoalUrgencyRate);
        tuning.TrustGain = read("sim.trust.gain", tuning.TrustGain);
        tuning.DispositionGain = read("sim.disposition.gain", tuning.DispositionGain);
        tuning.DispositionLoss = read("sim.disposition.loss", tuning.DispositionLoss);
        tuning.NeedJitter = read("sim.jitter", tuning.NeedJitter);

        // The bond watch-list (0.6.0 stone 08): every
        // sim.bond.threshold.<name> key adds one line the world drew
        // across disposition. A broken value is ignored — a bad line must
        // never break the world. Names are sorted so that when several
        // lines cross in one mutation, the events arrive in a stable
        // order (determinism, stone 05).
        config.ForEach(
            [&tuning](std::string_view name, std::string_view value)
            {
                constexpr std::string_view prefix = "sim.bond.threshold.";

                if (!name.starts_with(prefix))
                {
                    return;
                }

                try
                {
                    tuning.BondThresholds.push_back(BondThreshold{
                        std::string(name.substr(prefix.size())),
                        std::stof(std::string(value)) });
                }
                catch (const std::exception&)
                {
                    // not a number — the line is ignored
                }
            });

        std::sort(
            tuning.BondThresholds.begin(),
            tuning.BondThresholds.end(),
            [](const BondThreshold& left, const BondThreshold& right)
            {
                return left.Name < right.Name;
            });

        return tuning;
    }
}
