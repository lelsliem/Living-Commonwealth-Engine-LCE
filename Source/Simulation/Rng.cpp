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
// │            “Random numbers are just statistics wearing a mask.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Rng.cpp
//
// Purpose:
//
//      Implements the seeded generator — splitmix64, whose one-word
//      state makes save/load a single number (0.5.0).
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

#include "LCE/Simulation/Rng.h"

namespace LCE::Simulation
{
    namespace
    {
        // splitmix64's mixing constants — the avalanche that turns the
        // counter into well-distributed output.
        constexpr std::uint64_t kGolden = 0x9E3779B97F4A7C15ull;
        constexpr std::uint64_t kMixA = 0xBF58476D1CE4E5B9ull;
        constexpr std::uint64_t kMixB = 0x94D049BB133111EBull;

        std::uint64_t Avalanche(std::uint64_t z) noexcept
        {
            z = (z ^ (z >> 30)) * kMixA;
            z = (z ^ (z >> 27)) * kMixB;

            return z ^ (z >> 31);
        }
    }

    Rng::Rng(std::uint64_t seed) noexcept
        : m_Seed(seed), m_State(seed)
    {
    }

    std::uint64_t Rng::Next() noexcept
    {
        m_State += kGolden;

        return Avalanche(m_State);
    }

    float Rng::NextFloat() noexcept
    {
        // The top 24 bits give full single-precision resolution in [0, 1).
        return static_cast<float>(Next() >> 40) * (1.0f / 16777216.0f);
    }

    float Rng::NextFloat(float min, float max) noexcept
    {
        return min + (max - min) * NextFloat();
    }

    std::uint64_t Rng::State() const noexcept
    {
        return m_State;
    }

    void Rng::SetState(std::uint64_t state) noexcept
    {
        m_State = state;
    }

    Rng Rng::Derive(std::uint64_t key) const noexcept
    {
        // Mix the key into the current state and avalanche — a child
        // stream. The parent is untouched, so the tick can derive per
        // entity in any iteration order and the results never change.
        return Rng(Avalanche(m_State + kGolden + key));
    }

    Rng Rng::StableDerive(std::uint64_t key) const noexcept
    {
        // Mix the key into the SEED and avalanche — a child stream that
        // never moves, no matter how far the parent has advanced. The
        // tick's per-entity noise (needs decay, Decide jitter) anchors
        // here, so a caller advancing the parent between ticks (births,
        // mediation) can never re-roll a mind's personality (0.8.x field
        // finding). Same (seed, key) → same child, every run, every tick.
        return Rng(Avalanche(m_Seed + kGolden + key));
    }
}
