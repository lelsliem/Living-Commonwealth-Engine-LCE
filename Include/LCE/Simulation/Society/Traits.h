//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                                                                         │
// │                       ██╗      ██████╗███████╗                          │
// │                       ██║     ██╔════╝██╔════╝                          │
// │                       ██║     ██║     █████╗                            │
// │                       ██║     ██║     ██╔══╝                            │
// │                       ███████╗╚██████╗███████╗                          │
// │                       ╚══════╝ ╚═════╝╚══════╝                          │
// │                                                                         │
// │            Building living worlds through simulation.                   │
// │                                                                         │
// │                    "Same seed, different tree."                         │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Traits.h
//
// Purpose:
//
//      Defines the personality substrate (0.6.0 stone 09): a named-float
//      component whose per-entity variation derives deterministically
//      from the seeded RNG — identical base traits, different
//      individuals. The *influence* of a trait is the world's business
//      (its behaviour tables read the component); the core provides the
//      variation, the persistence, and the query.
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
#include "LCE/Simulation/Substrate/Rng.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // TraitValue
    //
    // One named personality number. The name is the world's vocabulary —
    // "boldness", "sociability" — the core only carries it and varies it.
    //-------------------------------------------------------------------------
    struct TraitValue
    {
        std::string Name;
        float Value = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Traits
    //
    // The component: an entity's personality. A co-save component like
    // any other — persisted, queried, never global. The world's decision
    // layer reads it; the core's Decide stays vocabulary-free.
    //-------------------------------------------------------------------------
    struct Traits
    {
        std::vector<TraitValue> List;
    };

    //-------------------------------------------------------------------------
    // JitteredTraits
    //
    // Per-entity variation of a base template: every trait value is
    // scaled by (1 ± spread) around its base, derived deterministically
    // from the entity's ID. With an Rng: a child stream — same seed +
    // same entity = same traits, every run, order-independent (the
    // parent stream never advances). Without: the deterministic id-hash
    // fallback. A spread of zero reproduces the base exactly. Pure — a
    // function of its arguments (ADR-0026).
    //-------------------------------------------------------------------------
    [[nodiscard]]
    inline Traits JitteredTraits(
        const Traits& base,
        EntityId id,
        const Rng* rng,
        float spread = 0.2f)
    {
        Traits result;

        // The entity's own child stream, derived once — every trait is
        // its OWN draw from it, so one entity's traits differ from each
        // other, and the stream advances while the parent never does.
        // (0.8.2 field fix: the old code re-derived the child for each
        // trait and took only its first draw, so every trait of an
        // entity came out identical.)
        std::optional<Rng> child;

        if (rng != nullptr)
        {
            child.emplace(rng->Derive(id.Value()));
        }

        result.List.reserve(base.List.size());

        for (std::size_t i = 0; i < base.List.size(); ++i)
        {
            float noise = 0.0f;

            if (child.has_value())
            {
                noise = child->NextFloat(-spread, spread);
            }
            else
            {
                // The fallback folds the trait's index into the hash so
                // the traits of one entity differ, not just entities.
                constexpr std::uint64_t kGolden = 0x9E3779B97F4A7C15ull;

                const auto hash =
                    (id.Value() * kGolden) ^ (i * kGolden);

                const auto units = static_cast<float>((hash >> 32) % 1000u);

                noise = (units / 1000.0f - 0.5f) * 2.0f * spread;
            }

            result.List.push_back(TraitValue{
                base.List[i].Name,
                base.List[i].Value * (1.0f + noise) });
        }

        return result;
    }
}
