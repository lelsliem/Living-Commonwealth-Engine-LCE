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
// │          “Simulation is the art of giving purpose to the invisible.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EntityId.h
//
// Purpose:
//
//      Defines the tagged identifier used to name simulation entities.
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
#include <functional>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // EntityId
    //
    // A tagged identifier for a simulation entity. It wraps a 64-bit value
    // so an ID can never be confused with an ordinary integer: the compiler
    // rejects DestroyEntity(healthPoints) and accepts DestroyEntity(id).
    //
    // Layout of the value:
    //     low  32 bits = slot index in the registry
    //     high 32 bits = generation (how many times the slot was reused)
    //
    // A generation of 0 with index 0 is the invalid sentinel. A live entity
    // always has generation >= 1, so the sentinel is never alive.
    //-------------------------------------------------------------------------
    class EntityId
    {
    public:
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue = 0;

        //-------------------------------------------------------------------------
        // Constructs an invalid ID (this is also the default).
        //-------------------------------------------------------------------------
        constexpr EntityId() = default;

        //-------------------------------------------------------------------------
        // Constructs an ID from a raw packed value. Prefer Make().
        //-------------------------------------------------------------------------
        explicit constexpr EntityId(ValueType value) noexcept
            : m_Value(value)
        {
        }

        //-------------------------------------------------------------------------
        // The raw packed value (index and generation combined).
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr ValueType Value() const noexcept
        {
            return m_Value;
        }

        //-------------------------------------------------------------------------
        // The slot index this entity occupies in the registry.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr std::uint32_t Index() const noexcept
        {
            return static_cast<std::uint32_t>(m_Value & 0xFFFFFFFFull);
        }

        //-------------------------------------------------------------------------
        // The generation of the slot — bumped each time the slot is reused.
        // A stale ID carries an old generation and therefore never matches.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr std::uint32_t Generation() const noexcept
        {
            return static_cast<std::uint32_t>(m_Value >> 32);
        }

        //-------------------------------------------------------------------------
        // Returns whether this ID could name a live entity.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr bool IsValid() const noexcept
        {
            return m_Value != InvalidValue;
        }

        //-------------------------------------------------------------------------
        // Packs an index and generation into an ID. The registry's factory.
        //-------------------------------------------------------------------------
        static constexpr EntityId Make(
            std::uint32_t index,
            std::uint32_t generation) noexcept
        {
            return EntityId{
                (static_cast<ValueType>(generation) << 32) | index };
        }

        friend constexpr bool operator==(EntityId, EntityId) = default;
        friend constexpr bool operator!=(EntityId, EntityId) = default;

    private:
        ValueType m_Value = InvalidValue;
    };
}

namespace std
{
    //-------------------------------------------------------------------------
    // Lets EntityId be a key in unordered containers: hash the packed value.
    //-------------------------------------------------------------------------
    template <>
    struct hash<LCE::Simulation::EntityId>
    {
        [[nodiscard]]
        size_t operator()(const LCE::Simulation::EntityId& id) const noexcept
        {
            return std::hash<LCE::Simulation::EntityId::ValueType>{}(id.Value());
        }
    };
}
