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
}

namespace LCE::Simulation
{
    void Update(
        EntityRegistry& registry,
        double deltaSeconds,
        const SimulationTuning& tuning)
    {
        const auto delta = static_cast<float>(deltaSeconds);

        //-------------------------------------------------------------------------
        // Needs decay. Only entities WITH a Needs component are simulated —
        // a rock has no needs and is untouched.
        //-------------------------------------------------------------------------
        registry.ForEachWithComponent<Needs>(
            [delta](EntityId, Needs& needs)
            {
                for (auto& need : needs.List)
                {
                    need.Value -= need.DecayRate * delta;

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
            [&registry, &decisions](EntityId id, const Needs&)
            {
                decisions.emplace_back(id, Decide(registry, id));
            });

        for (const auto& [id, intent] : decisions)
        {
            if (intent)
            {
                registry.AddComponent<Intent>(id, *intent);
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
        const SimulationTuning& tuning)
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

        memory->Events.push_back(event);

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
    }

    void ReportOutcome(
        EntityRegistry& registry,
        EntityId id,
        const Outcome& outcome,
        const SimulationTuning& tuning)
    {
        if (!registry.IsAlive(id))
        {
            return;
        }

        //-------------------------------------------------------------------------
        // 1. Record the memory — the experience itself.
        //-------------------------------------------------------------------------
        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        memory->Events.push_back({ outcome.Other, outcome.Kind, outcome.Weight });

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

        return tuning;
    }
}
