//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐ //
// │                                                                         │ //
// │                       ██╗      ██████╗███████╗                          │ //
// │                       ██║     ██╔════╝██╔════╝                          │ //
// │                       ██║     ██║     █████╗                            │ //
// │                       ██║     ██║     ██╔══╝                            │ //
// │                       ███████╗╚██████╗███████╗                          │ //
// │                       ╚══════╝ ╚═════╝╚══════╝                          │ //
// │                                                                         │ //
// │            Building living worlds through simulation.                   │ //
// │                                                                         │ //
// │           "A name outlives the voice that carried it."                  │ //
// │                                                                         │ //
// └─────────────────────────────────────────────────────────────────────────┘ //
//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Legacy.h
//
// Purpose:
//
//      Defines the legacy store (0.7.0 stone 12) — the promise that
//      outlives its maker. Registry-level facts keyed by name, permanent
//      until the world forgets them. The core holds opaque named data
//      with an owner and a day; it knows nothing of bridges or pledges
//      (the GroupId boundary — the world assigns the meaning).
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

#include "LCE/Simulation/EntityId.h"
#include "LCE/Simulation/RegistrySnapshot.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // LegacyFact
    //
    // One record the world left behind. Owner goes stale on death — the
    // fact persists while the ID may not. Day is when it was left; the
    // age of a legacy is now.Day - fact.Day, like a memory's.
    //-------------------------------------------------------------------------
    struct LegacyFact
    {
        EntityId Owner;
        std::uint64_t Day = 0;
        std::string Name;
        float Weight = 1.0f;
    };

    //-------------------------------------------------------------------------
    // LegacyStore
    //
    // The registry-level store, deliberately shaped like the component
    // stores: keyed by name, serialized through the world's registered
    // serializer, wiped by Clear() while the serializer survives.
    //-------------------------------------------------------------------------
    class LegacyStore
    {
    public:
        // Leaves (or replaces) the fact with that name.
        void Leave(LegacyFact fact)
        {
            m_Facts[fact.Name] = std::move(fact);
        }

        // Returns the fact with that name, or nothing.
        [[nodiscard]]
        std::optional<LegacyFact> Read(std::string_view name) const
        {
            const auto iterator = m_Facts.find(std::string(name));

            if (iterator == m_Facts.end())
            {
                return std::nullopt;
            }

            return iterator->second;
        }

        // Retires the fact with that name — the world's explicit decay.
        void Forget(std::string_view name)
        {
            m_Facts.erase(std::string(name));
        }

        [[nodiscard]]
        bool Empty() const noexcept
        {
            return m_Facts.empty();
        }

        // The world registers how the name map becomes bytes and back —
        // the same contract as the component stores (0.4.0). Required for
        // legacies to ride the co-save.
        void SetSerializer(
            ComponentSerializer<std::unordered_map<std::string, LegacyFact>> serializer)
        {
            m_Serializer = std::move(serializer);
        }

        // The whole store as one blob; absent when empty or unserialized.
        [[nodiscard]]
        std::optional<ComponentBlob> Serialize() const
        {
            if (m_Facts.empty() || !m_Serializer)
            {
                return std::nullopt;
            }

            return m_Serializer->Serialize(m_Facts);
        }

        // Replaces the store from a blob captured earlier.
        void Deserialize(const ComponentBlob& blob)
        {
            if (!m_Serializer)
            {
                return;
            }

            m_Facts = m_Serializer->Deserialize(blob);
        }

        // Wipes every fact. The serializer survives — registered once at
        // init, kept for the next game's Restore (like Clear on stores).
        void Clear() noexcept
        {
            m_Facts.clear();
        }

    private:
        std::unordered_map<std::string, LegacyFact> m_Facts;
        std::optional<
            ComponentSerializer<std::unordered_map<std::string, LegacyFact>>>
            m_Serializer;
    };
}
