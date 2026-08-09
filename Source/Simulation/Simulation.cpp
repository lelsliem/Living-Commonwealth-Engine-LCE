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

#include <utility>
#include <vector>

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
}
