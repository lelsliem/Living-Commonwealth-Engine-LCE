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

#include "LCE/Simulation/Decision/Behaviour.h"

namespace
{
    //-------------------------------------------------------------------------
    // Small deterministic jitter so two identical farmers can choose
    // differently — personality from an ID. No global state: seeded by a
    // golden-ratio hash of the entity's own value. This is the fallback
    // when no Rng is provided (0.5.0); with an Rng, the jitter comes from
    // a per-entity child stream instead (see Decide).
    //-------------------------------------------------------------------------
    float Noise(LCE::Simulation::EntityId id) noexcept
    {
        constexpr float kNoise = 0.05f;

        const auto hash = id.Value() * 0x9E3779B97F4A7C15ull;

        const auto units = static_cast<float>((hash >> 32) % 1000u);

        return (units / 1000.0f - 0.5f) * 2.0f * kNoise;
    }

    //-------------------------------------------------------------------------
    // The jitter magnitude — the same ±5% personality band either way.
    //-------------------------------------------------------------------------
    constexpr float kNoise = 0.05f;

    //-------------------------------------------------------------------------
    // Personality jitter. With an Rng: a child stream derived from the
    // entity's ID — same seed + same entity = same jitter, and iteration
    // order can never leak into the stream (the parent is untouched).
    // StableDerive anchors to the seed, never the live state, so the
    // parent advancing between ticks (the adapter's births) can never
    // re-roll a settled mind's noise (0.8.x field finding). Without: the
    // deterministic id-hash fallback.
    //-------------------------------------------------------------------------
    float Jitter(
        LCE::Simulation::EntityId id,
        const LCE::Simulation::Rng* rng) noexcept
    {
        if (rng != nullptr)
        {
            return rng->StableDerive(id.Value()).NextFloat(-kNoise, kNoise);
        }

        return Noise(id);
    }

    //-------------------------------------------------------------------------
    // The most urgent need: the one with the lowest value (most deprived).
    //
    // Personality tie-break (0.8.4): when two needs are within a small
    // band of the most urgent one, they are effectively tied — and list
    // order must not decide the mind. With an Rng, the winner among band
    // members is the one with the highest per-need draw, derived from
    // (seed, entity, need ordinal): same seed + same entity + same needs
    // = same winner, every run, whatever the iteration order — the
    // parent stream never advances. This is the seam a world's traits
    // multiply into: a bold mind's Safety can win its attention over a
    // barely-more-urgent Hunger. Without an Rng the strict lowest wins
    // and ties fall to list order — behaviour unchanged.
    //-------------------------------------------------------------------------
    const LCE::Simulation::Need* MostUrgent(
        const LCE::Simulation::Needs& needs,
        LCE::Simulation::EntityId id,
        const LCE::Simulation::Rng* rng) noexcept
    {
        const LCE::Simulation::Need* best = nullptr;

        for (const auto& need : needs.List)
        {
            if (best == nullptr || need.Value < best->Value)
            {
                best = &need;
            }
        }

        if (best == nullptr || rng == nullptr)
        {
            return best;
        }

        // The band: within this margin of the most urgent need, needs
        // compete for the mind's attention. Small enough that a clearly
        // dominant need still wins outright; the draw decides the rest.
        //
        // The draw keys on the need TYPE, never the list index — two
        // minds with the same needs listed in a different order make
        // the same choice (the QueryWhere discipline).
        constexpr float kTieBand = 0.05f;

        const LCE::Simulation::Need* winner = best;
        float winnerDraw = -1.0f;

        for (const auto& need : needs.List)
        {
            if (need.Value > best->Value + kTieBand)
            {
                continue;
            }

            const auto draw = rng->StableDerive(id.Value())
                .Derive(static_cast<std::uint64_t>(need.Type))
                .NextFloat();

            if (draw > winnerDraw)
            {
                winnerDraw = draw;
                winner = &need;
            }
        }

        return winner;
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

    //-------------------------------------------------------------------------
    // IsUnavailable
    //
    // A world fact declares one kind of interaction unavailable *while it
    // is remembered*. World facts have an invalid Other — nothing to meet,
    // no one to trust — so no relationship is shaped, only a door is shut.
    // The tick forgets facts below the threshold, which is how the market
    // reopens: the fact fades, and trade is possible again.
    //-------------------------------------------------------------------------
    bool IsUnavailable(
        const LCE::Simulation::Memory& memory,
        LCE::Simulation::InteractionKind kind) noexcept
    {
        for (const auto& event : memory.Events)
        {
            if (!event.Other.IsValid() && event.Kind == kind)
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // FindThreat
    //
    // The threat: the Other of the strongest remembered wrong or fight.
    // Returns nullopt when the entity remembers no danger.
    //-------------------------------------------------------------------------
    std::optional<LCE::Simulation::EntityId> FindThreat(
        const LCE::Simulation::EntityRegistry& registry,
        LCE::Simulation::EntityId id)
    {
        const auto memory = registry.GetComponent<LCE::Simulation::Memory>(id);

        if (!memory)
        {
            return std::nullopt;
        }

        std::optional<LCE::Simulation::EntityId> threat;
        float bestWeight = 0.0f;

        for (const auto& event : memory->Events)
        {
            if (!event.Other.IsValid())
            {
                continue;   // world facts name no one
            }

            if (event.Kind != LCE::Simulation::InteractionKind::Wronged
                && event.Kind != LCE::Simulation::InteractionKind::Combat)
            {
                continue;
            }

            if (event.Weight > bestWeight)
            {
                bestWeight = event.Weight;
                threat = event.Other;
            }
        }

        return threat;
    }
}

namespace LCE::Simulation
{
    std::optional<Intent> Decide(
        const EntityRegistry& registry,
        EntityId id,
        const Rng* rng,
        float desperateHunger)
    {
        // No drives, no decision. Only minds act.
        const auto needs = registry.GetComponent<Needs>(id);

        if (!needs)
        {
            return std::nullopt;
        }

        const Need* urgent = MostUrgent(*needs, id, rng);

        if (urgent == nullptr)
        {
            return std::nullopt;
        }

        Intent intent;
        intent.Confidence = (1.0f - urgent->Value) + Jitter(id, rng);

        switch (urgent->Type)
        {
        case NeedType::Hunger:
        {
            // The farmer goes to market: hunger is urgent, memory says the
            // merchant trades, and trust favours them. No script fired.
            // Unless the world is shut: a remembered Trade world fact
            // (invalid Other) blocks the trip while it lasts — except for
            // the desperate (0.7.0 field finding): below the desperate
            // threshold the closed sign is ignored, so a starving mind
            // walks to the shut door anyway and the refusal can happen.
            const auto memory = registry.GetComponent<Memory>(id);
            const bool desperate = urgent->Value < desperateHunger;

            if (!desperate && memory && IsUnavailable(*memory, InteractionKind::Trade))
            {
                intent.Action = ActionType::Explore;   // market closed
            }
            else if (auto target = ChooseTarget(registry, id, InteractionKind::Trade))
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
            // The same world-fact rule: a remembered Social world fact
            // means no one is gathering today.
            const auto memory = registry.GetComponent<Memory>(id);

            if (memory && IsUnavailable(*memory, InteractionKind::Social))
            {
                intent.Action = ActionType::Explore;
            }
            else if (auto target = ChooseTarget(registry, id, InteractionKind::Social))
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
        {
            // Danger awareness: the strongest remembered wrong or fight
            // names the threat to flee. No threat in memory → no decision
            // — you can't flee from nothing.
            if (auto threat = FindThreat(registry, id))
            {
                intent.Action = ActionType::Flee;
                intent.Target = *threat;
                intent.Confidence += 0.2f;   // knows the danger
            }
            else
            {
                return std::nullopt;
            }
            break;
        }

        default:
            // An unknown need type has no decision this tick.
            return std::nullopt;
        }

        return intent;
    }
}
