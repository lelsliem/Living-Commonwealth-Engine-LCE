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
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    //-------------------------------------------------------------------------
    // The magnitude and direction an outcome gives its kind's effect:
    // success counts fully, partial counts half, failure inverts the
    // positive kinds (a failed trade loses trust).
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // AppendMemory (0.8.0 stone 14a)
    //
    // Pushes one event and, when sim.memory.cap is set, evicts the
    // lowest-weight event (oldest wins ties) once the store exceeds the
    // cap — a mind can only hold so much, and the hot path (ChooseTarget,
    // IsUnavailable, FindThreat, fade) stays bounded by it. Deterministic:
    // ties resolve to the earliest remembered, and the store is append
    // ordered. Cap 0 = unbounded (the default, behavior unchanged).
    // Snapshot restore never goes through here — a loaded world is the
    // truth, not a fresh experience.
    //-------------------------------------------------------------------------
    void AppendMemory(
        LCE::Simulation::Memory& memory,
        LCE::Simulation::MemoryEvent event,
        std::size_t cap)
    {
        memory.Events.push_back(event);

        if (cap == 0 || memory.Events.size() <= cap)
        {
            return;
        }

        std::size_t weakest = 0;

        for (std::size_t i = 1; i < memory.Events.size(); ++i)
        {
            if (memory.Events[i].Weight < memory.Events[weakest].Weight)
            {
                weakest = i;
            }
        }

        memory.Events.erase(
            memory.Events.begin() + static_cast<std::ptrdiff_t>(weakest));
    }

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

    //-------------------------------------------------------------------------
    // SpreadToGroupMates
    //
    // The Society echo (0.6.0 stone 09): trust is earned personally;
    // disposition travels. Every mind sharing a group with the subject
    // feels a fainter version of the subject's feeling about the other —
    // "they wronged my brother" — at GroupInheritance strength. The echo
    // shapes feelings, not memory: memory is personal, and the
    // RelationshipChangedEvent is how a mate learns the news. Crossings
    // publish exactly like any other relationship change.
    //-------------------------------------------------------------------------
    void SpreadToGroupMates(
        LCE::Simulation::EntityRegistry& registry,
        const LCE::Simulation::SimulationTuning& tuning,
        LCE::Simulation::EntityId subject,
        LCE::Simulation::EntityId other,
        float dispositionDelta,
        LCE::Events::EventBus* events,
        std::uint64_t day)
    {
        if (dispositionDelta == 0.0f)
        {
            return;
        }

        const auto subjectGroups =
            registry.GetComponent<LCE::Simulation::Groups>(subject);

        if (!subjectGroups || subjectGroups->Memberships.empty())
        {
            return;
        }

        const auto delta = dispositionDelta * tuning.GroupInheritance;

        if (delta == 0.0f)
        {
            return;
        }

        // Every mind sharing any of the subject's groups hears the echo.
        const auto members = registry.QueryWhere<LCE::Simulation::Groups>(
            [&subjectGroups](
                LCE::Simulation::EntityId,
                const LCE::Simulation::Groups& groups)
            {
                for (const auto memberGroup : groups.Memberships)
                {
                    for (const auto subjectGroup : subjectGroups->Memberships)
                    {
                        if (memberGroup == subjectGroup)
                        {
                            return true;
                        }
                    }
                }

                return false;
            });

        for (const auto member : members)
        {
            // Not the subject, and never the other — a mate who IS the
            // other has no self-relationship to shape.
            if (member == subject || member == other)
            {
                continue;
            }

            auto relationships =
                registry.GetComponent<LCE::Simulation::Relationships>(member);

            if (!relationships)
            {
                registry.AddComponent<LCE::Simulation::Relationships>(
                    member, LCE::Simulation::Relationships{});
                relationships =
                    registry.GetComponent<LCE::Simulation::Relationships>(member);
            }

            auto& relationship = relationships->ByEntity[other];
            const auto before = relationship.Disposition;

            relationship.Disposition += delta;

            PublishCrossings(
                events, tuning, member, other,
                before, relationship.Disposition,
                relationship.Trust, day);
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
        const Rng* rng,
        TickReport* report)
    {
        const auto delta = static_cast<float>(deltaSeconds);

        // Measurement (0.8.0 stone 13): a local report accumulates every
        // pass; only when a caller asked for one does the tick read the
        // clock or copy it out — the default path (nullptr) is untouched.
        TickReport local;

        const auto started = (report != nullptr)
            ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};

        //-------------------------------------------------------------------------
        // Needs decay. Only entities WITH a Needs component are simulated —
        // a rock has no needs and is untouched.
        //
        // Per-mind metabolism (0.5.0): when a seeded Rng is present, each
        // entity's needs decay at its own rate, derived from its ID — the
        // same seed + the same entity yields the same rate every tick, and
        // the parent stream never advances. StableDerive anchors to the
        // seed, never the live state, so a caller advancing the parent
        // between ticks (the adapter's births) can never re-roll a mind's
        // metabolism — a near-tied mind stays settled (0.8.x field
        // finding). This is what breaks the herd: identical minds no
        // longer get hungry on the same clock.
        //
        // Per-NEED metabolism (0.8.4): the rate is derived per need, the
        // need's TYPE folded into the key — a bold mind's Safety can
        // decay differently from its Hunger, which is exactly the seam a
        // world's traits multiply into. Same seed + same entity + same
        // need = same rate, every tick. The key is the need type, never
        // the list index — two minds with the same needs listed in a
        // different order metabolize identically (the QueryWhere
        // discipline). Without an Rng the jitter is exactly 1.0 —
        // behaviour unchanged, so no existing caller is affected.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Needs>(
            [delta, rng, &tuning, &local](EntityId id, Needs& needs)
            {
                ++local.Entities;

                for (auto& need : needs.List)
                {
                    const auto rate = (rng != nullptr)
                        ? rng->StableDerive(id.Value())
                              .Derive(static_cast<std::uint64_t>(need.Type))
                              .NextFloat(
                                  1.0f - tuning.NeedJitter,
                                  1.0f + tuning.NeedJitter)
                        : 1.0f;

                    need.Value -= need.DecayRate * delta * rate;

                    if (need.Value < 0.0f)
                    {
                        need.Value = 0.0f;
                    }
                }
            });

        if (report != nullptr)
        {
            local.NeedsMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
        }

        //-------------------------------------------------------------------------
        // Memory fades. Salience erodes each second; forgotten below the
        // threshold. Erase-while-iterating: safe because we never touch the
        // store inside the loop (same rule as the Scheduler).
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Memory>(
            [delta, &tuning, &local](EntityId, Memory& memory)
            {
                local.MemoryEvents += memory.Events.size();

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

        if (report != nullptr)
        {
            local.MemoryMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
        }

        //-------------------------------------------------------------------------
        // Relationships drift toward neutral — feelings cool over time
        // unless experience refreshes them.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Relationships>(
            [delta, &tuning, &local](EntityId, Relationships& relationships)
            {
                local.Relationships += relationships.ByEntity.size();

                for (auto& entry : relationships.ByEntity)
                {
                    auto& relationship = entry.second;

                    relationship.Disposition +=
                        (0.0f - relationship.Disposition) * tuning.DriftRate * delta;
                    relationship.Trust +=
                        (0.0f - relationship.Trust) * tuning.DriftRate * delta;
                }
            });

        if (report != nullptr)
        {
            local.RelationshipsMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
        }

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

        if (report != nullptr)
        {
            local.GoalsMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
        }

        //-------------------------------------------------------------------------
        // Decide: one Intent per mind. Two-phase on purpose — deciding may
        // AddComponent/RemoveComponent (mutating the Intent store), so we
        // first collect every decision, then apply them. Never mutate a
        // store while iterating it.
        //-------------------------------------------------------------------------
        std::vector<std::pair<EntityId, std::optional<Intent>>> decisions;

        registry.ForEachWithComponent<Needs>(
            [&registry, &decisions, rng, &tuning](EntityId id, const Needs&)
            {
                decisions.emplace_back(
                    id, Decide(registry, id, rng, tuning.HungerDesperate));
            });

        if (report != nullptr)
        {
            local.DecideMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
        }

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

        if (report != nullptr)
        {
            local.TotalMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started).count();
            *report = local;
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

        // Bounded memory (0.8.0 stone 14a): a mind can only hold so much.
        AppendMemory(*memory, stamped, tuning.MemoryCap);

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
        float vicariousDisposition = 0.0f;

        switch (event.Kind)
        {
        case InteractionKind::Trade:
            relationship.Trust += tuning.TrustGain;
            break;

        case InteractionKind::Aid:
        case InteractionKind::Social:
            relationship.Disposition += tuning.DispositionGain;
            vicariousDisposition = tuning.DispositionGain;
            break;

        case InteractionKind::Wronged:
        case InteractionKind::Combat:
            relationship.Disposition -= tuning.DispositionLoss;
            vicariousDisposition = -tuning.DispositionLoss;
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

        // The Society echo (0.6.0 stone 09): the subject's group-mates
        // feel a fainter version of the same feeling.
        SpreadToGroupMates(
            registry, tuning, id, event.Other,
            vicariousDisposition, events, time.Day);
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

        // Bounded memory (0.8.0 stone 14a).
        AppendMemory(*memory, stamped, tuning.MemoryCap);

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
            float vicariousDisposition = 0.0f;

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
                vicariousDisposition =
                    tuning.DispositionGain * ResultScale(outcome.Result);
                break;

            case InteractionKind::Wronged:
            case InteractionKind::Combat:
                // A wrong is a wrong, however it went — full loss.
                relationship.Disposition -= tuning.DispositionLoss;
                vicariousDisposition = -tuning.DispositionLoss;
                break;
            }

            // Bond crossing (0.6.0 stone 08) — same edge-triggered rule
            // as Remember: the moment a line is crossed, not the resting
            // state beside it.
            PublishCrossings(
                events, tuning, id, outcome.Other,
                dispositionBefore, relationship.Disposition,
                relationship.Trust, time.Day);

            // The Society echo (0.6.0 stone 09): group-mates share the
            // feeling, fainter — the outcome that sour a settlement.
            SpreadToGroupMates(
                registry, tuning, id, outcome.Other,
                vicariousDisposition, events, time.Day);
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

    void InheritGroupAttitudes(
        EntityRegistry& registry,
        EntityId id,
        GroupId group)
    {
        if (!registry.IsAlive(id))
        {
            return;
        }

        auto relationships = registry.GetComponent<Relationships>(id);

        if (!relationships)
        {
            registry.AddComponent<Relationships>(id, Relationships{});
            relationships = registry.GetComponent<Relationships>(id);
        }

        // The group's collective knowledge, derived from its members:
        // for every other the group knows, the sum and count of the
        // members' dispositions toward them. Derived, never stored — the
        // same rule as the seasons.
        std::unordered_map<EntityId, std::pair<double, int>> sums;

        const auto members = registry.QueryWhere<Groups>(
            [group, id](EntityId member, const Groups& memberships)
            {
                if (member == id)
                {
                    return false;   // the newcomer is not their own group
                }

                for (const auto candidate : memberships.Memberships)
                {
                    if (candidate == group)
                    {
                        return true;
                    }
                }

                return false;
            });

        for (const auto member : members)
        {
            const auto memberRelationships =
                registry.GetComponent<Relationships>(member);

            if (!memberRelationships)
            {
                continue;
            }

            for (const auto& [other, relationship] : memberRelationships->ByEntity)
            {
                if (other == id)
                {
                    continue;   // no self-knowledge to inherit
                }

                auto& sum = sums[other];
                sum.first += relationship.Disposition;
                ++sum.second;
            }
        }

        // Seed the newcomer with the group's mean disposition. Personal
        // knowledge always beats inherited: an existing relationship is
        // left untouched — divergence has already begun. Trust is never
        // inherited; trust is earned personally. Quiet by design: seeding
        // is not an event (the same rule as drift).
        for (const auto& [other, sum] : sums)
        {
            if (relationships->ByEntity.contains(other))
            {
                continue;
            }

            relationships->ByEntity[other].Disposition =
                static_cast<float>(sum.first / sum.second);
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
        tuning.GroupInheritance = read("sim.group.inheritance", tuning.GroupInheritance);
        tuning.HungerDesperate = read("sim.hunger.desperate", tuning.HungerDesperate);
        tuning.BequestFloor = read("sim.legacy.bequestFloor", tuning.BequestFloor);
        tuning.InheritanceScale = read("sim.legacy.inheritanceScale", tuning.InheritanceScale);

        // A day count, not a rate — its own reader (0.7.0 stone 11).
        // Same rule as every knob: a broken value keeps the default.
        {
            const auto raw = config.Get("sim.legacy.maxAgeDays");

            if (!raw.empty())
            {
                try
                {
                    tuning.LegacyMaxAgeDays = static_cast<std::uint64_t>(
                        std::stoull(std::string(raw)));
                }
                catch (const std::exception&)
                {
                    // not a number — the default stands
                }
            }
        }

        // A count, not a rate — its own reader (0.8.0 stone 14a). 0 = a
        // mind remembers everything (the default, unchanged behavior).
        {
            const auto raw = config.Get("sim.memory.cap");

            if (!raw.empty())
            {
                try
                {
                    tuning.MemoryCap = static_cast<std::size_t>(
                        std::stoull(std::string(raw)));
                }
                catch (const std::exception&)
                {
                    // not a number — the default stands
                }
            }
        }

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

    std::size_t Bequeath(
        EntityRegistry& registry,
        EntityId dying,
        std::span<const EntityId> heirs,
        const SimulationTuning& tuning)
    {
        if (!registry.IsAlive(dying))
        {
            return 0;
        }

        const auto memory = registry.GetComponent<Memory>(dying);

        if (!memory)
        {
            return 0;
        }

        // Deterministic (the QueryWhere discipline): the caller's list
        // order can never leak into results — process heirs ascending.
        std::vector<EntityId> ordered(heirs.begin(), heirs.end());

        std::sort(
            ordered.begin(),
            ordered.end(),
            [](EntityId left, EntityId right)
            {
                return left.Value() < right.Value();
            });

        std::size_t count = 0;

        for (const auto heir : ordered)
        {
            if (!registry.IsAlive(heir) || heir == dying)
            {
                continue;
            }

            auto heirMemory = registry.GetComponent<Memory>(heir);

            if (!heirMemory)
            {
                registry.AddComponent<Memory>(heir, Memory{});
                heirMemory = registry.GetComponent<Memory>(heir);
            }

            for (const auto& event : memory->Events)
            {
                if (event.Weight < tuning.BequestFloor)
                {
                    continue;   // faint enough to die with the owner
                }

                auto inherited = event;
                inherited.Weight = event.Weight * tuning.InheritanceScale;

                // Bounded memory (0.8.0 stone 14a).
                AppendMemory(*heirMemory, inherited, tuning.MemoryCap);
                ++count;
            }
        }

        return count;
    }

    std::size_t InheritMemory(
        EntityRegistry& registry,
        EntityId heir,
        EntityId ancestor,
        const SimulationTuning& tuning,
        WorldTime time,
        bool (*accept)(const MemoryEvent&))
    {
        if (!registry.IsAlive(heir) || !registry.IsAlive(ancestor)
            || heir == ancestor)
        {
            return 0;
        }

        const auto ancestorMemory = registry.GetComponent<Memory>(ancestor);

        if (!ancestorMemory)
        {
            return 0;
        }

        auto heirMemory = registry.GetComponent<Memory>(heir);

        if (!heirMemory)
        {
            registry.AddComponent<Memory>(heir, Memory{});
            heirMemory = registry.GetComponent<Memory>(heir);
        }

        std::size_t count = 0;

        for (const auto& event : ancestorMemory->Events)
        {
            if (accept != nullptr && !accept(event))
            {
                continue;
            }

            // The world's patience (0.7.0): facts older than
            // LegacyMaxAgeDays do not travel. Age is the story's, not
            // the hearer's — unstamped or future-dated facts pass.
            if (tuning.LegacyMaxAgeDays != 0
                && event.Day != 0 && time.Day != 0
                && time.Day > event.Day
                && time.Day - event.Day > tuning.LegacyMaxAgeDays)
            {
                continue;
            }

            auto inherited = event;
            inherited.Weight = event.Weight * tuning.InheritanceScale;

            // Bounded memory (0.8.0 stone 14a).
            AppendMemory(*heirMemory, inherited, tuning.MemoryCap);
            ++count;
        }

        return count;
    }

    std::size_t FixedStep::Advance(
        double frameDelta,
        EntityRegistry& registry,
        const SimulationTuning& tuning,
        LCE::Events::EventBus* events,
        const Rng* rng,
        TickReport* report)
    {
        std::size_t steps = 0;

        Remaining += frameDelta;

        while (Remaining >= Step)
        {
            Update(registry, Step, tuning, events, rng, report);
            Remaining -= Step;
            ++steps;
        }

        return steps;
    }
}
