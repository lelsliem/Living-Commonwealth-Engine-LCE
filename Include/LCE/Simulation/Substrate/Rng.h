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
// │      “Chaos with a seed is just a story that repeats itself politely.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Rng.h
//
// Purpose:
//
//      Defines the seeded random number generator — randomness that a
//      save can resume. One 64-bit word is the whole state, so capture
//      and restore are a single number (0.5.0).
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

#include <cstdint>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // Rng
    //
    // A splitmix64 generator: fast, tiny (one uint64 of state), and
    // deterministic — the same seed produces the same sequence forever.
    // The entire state is one word, so State()/SetState() capture and
    // restore the whole stream: persist one number in the co-save and a
    // restored world resumes the exact same randomness.
    //
    // Derive(key) is the determinism trick: it returns a CHILD stream
    // mixed from the current state and the key WITHOUT advancing the
    // parent. Same (state, key) → same child. StableDerive(key) is the
    // per-entity variant the tick uses: it anchors to the SEED, never
    // the live state, so advancing the parent for other draws (births,
    // mediation) can never re-roll a mind's noise — same seed + same
    // entity = same jitter, every run, whatever order the store visits
    // and however far the parent has moved (0.8.x field finding: a
    // near-tied mind re-rolled its intent every frame because the
    // adapter's births advanced the parent between ticks).
    //-------------------------------------------------------------------------
    class Rng
    {
    public:
        explicit Rng(std::uint64_t seed) noexcept;

        //-------------------------------------------------------------------------
        // The next full 64-bit draw, advancing the stream.
        //-------------------------------------------------------------------------
        std::uint64_t Next() noexcept;

        //-------------------------------------------------------------------------
        // A float in [0, 1).
        //-------------------------------------------------------------------------
        float NextFloat() noexcept;

        //-------------------------------------------------------------------------
        // A float in [min, max).
        //-------------------------------------------------------------------------
        float NextFloat(float min, float max) noexcept;

        //-------------------------------------------------------------------------
        // The whole state — one number. Persist it to resume this exact
        // stream after a restore.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        std::uint64_t State() const noexcept;

        //-------------------------------------------------------------------------
        // Resumes the stream from a captured state.
        //-------------------------------------------------------------------------
        void SetState(std::uint64_t state) noexcept;

        //-------------------------------------------------------------------------
        // A child stream for the key, without advancing this one. Same
        // (state, key) → same child, always — order-independent by design.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        Rng Derive(std::uint64_t key) const noexcept;

        //-------------------------------------------------------------------------
        // A child stream for the key anchored to the SEED, never the
        // live state. The parent may advance freely between calls — the
        // child for a key never moves. This is the per-entity noise
        // primitive (needs decay, Decide jitter): personality is a pure
        // function of (seed, entity), so a settled mind stays settled.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        Rng StableDerive(std::uint64_t key) const noexcept;

    private:
        std::uint64_t m_Seed;
        std::uint64_t m_State;
    };
}
