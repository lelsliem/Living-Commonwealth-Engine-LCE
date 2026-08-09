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
// │     “Behaviour is just needs, memory, and relationships arguing politely.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Behaviour.cpp
//
// Purpose:
//
//      Implements the decision function: how needs, memory, relationships,
//      and goals become one action.
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

#include "LCE/Simulation/Behaviour.h"

namespace
{
    //-------------------------------------------------------------------------
    // Small deterministic jitter so two identical farmers can choose
    // differently — personality from an ID. No global state: seeded by a
    // golden-ratio hash of the entity's own value.
    //-------------------------------------------------------------------------
    float Noise(LCE::Simulation::EntityId id) noexcept
    {
        constexpr float kNoise = 0.05f;

        const auto hash = id.Value() * 0x9E3779B97F4A7C15ull;

        const auto units = static_cast<float>((hash >> 32) % 1000u);

        return (units / 1000.0f - 0.5f) * 2.0f * kNoise;
    }

    //-------------------------------------------------------------------------
    // The most urgent need: the one with the lowest value (most deprived).
    //-------------------------------------------------------------------------
    const LCE::Simulation::Need* MostUrgent(
        const LCE::Simulation::Needs& needs) noexcept
    {
        const LCE::Simulation::Need* best = nullptr;

        for (const auto& need : needs.List)
        {
            if (best == nullptr || need.Value < best->Value)
            {
                best = &need;
            }
        }

        return best;
    }

    //-------------------------------------------------------------------------
    // The best-known Other for a kind of interaction: the remembered entity
    // with the strongest memory weight and the most favourable
    // relationship. Returns nullopt when the entity knows no one of that
    // kind.
    //-------------------------------------------------------------------------
    std::optional<LCE::Simulation::EntityId> ChooseTarget(
        const LCE::Simulation::EntityRegistry& registry,
        LCE::Simulation::EntityId id,
        LCE::Simulation::InteractionKind kind)
    {
        const auto memory = registry.GetComponent<LCE::Simulation::Memory>(id);

        if (!memory)
        {
            return std::nullopt;
        }

        const auto relationships =
            registry.GetComponent<LCE::Simulation::Relationships>(id);

        std::optional<LCE::Simulation::EntityId> best;
        float bestScore = 0.0f;

        for (const auto& event : memory->Events)
        {
            if (event.Kind != kind || !event.Other.IsValid())
            {
                continue;
            }

            float score = event.Weight;   // remembered = it matters

            if (relationships)
            {
                const auto iterator = relationships->ByEntity.find(event.Other);

                if (iterator != relationships->ByEntity.end())
                {
                    score += iterator->second.Trust + iterator->second.Disposition;
                }
            }

            if (score > bestScore)
            {
                bestScore = score;
                best = event.Other;
            }
        }

        return best;
    }
}

namespace LCE::Simulation
{
    std::optional<Intent> Decide(
        const EntityRegistry& registry,
        EntityId id)
    {
        // No drives, no decision. Only minds act.
        const auto needs = registry.GetComponent<Needs>(id);

        if (!needs)
        {
            return std::nullopt;
        }

        const Need* urgent = MostUrgent(*needs);

        if (urgent == nullptr)
        {
            return std::nullopt;
        }

        Intent intent;
        intent.Confidence = (1.0f - urgent->Value) + Noise(id);

        switch (urgent->Type)
        {
        case NeedType::Hunger:
        {
            // The farmer goes to market: hunger is urgent, memory says the
            // merchant trades, and trust favours them. No script fired.
            if (auto target = ChooseTarget(registry, id, InteractionKind::Trade))
            {
                intent.Action = ActionType::MoveTo;
                intent.Target = *target;
                intent.Confidence += 0.2f;   // knows a source
            }
            else
            {
                intent.Action = ActionType::Explore;   // must find food first
            }
            break;
        }

        case NeedType::Fatigue:
            intent.Action = ActionType::Rest;
            break;

        case NeedType::Social:
        {
            if (auto target = ChooseTarget(registry, id, InteractionKind::Social))
            {
                intent.Action = ActionType::Socialize;
                intent.Target = *target;
                intent.Confidence += 0.2f;
            }
            else
            {
                intent.Action = ActionType::Explore;
            }
            break;
        }

        case NeedType::Comfort:
            intent.Action = ActionType::Work;
            break;

        case NeedType::Safety:
        default:
            // No danger awareness yet — Flee arrives with a later stone.
            return std::nullopt;
        }

        return intent;
    }
}
