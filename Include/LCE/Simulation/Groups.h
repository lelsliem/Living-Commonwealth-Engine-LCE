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
// │          "Belonging is the first story every mind tells itself."        │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Groups.h
//
// Purpose:
//
//      Defines the Society substrate (0.6.0 stone 09): opaque group
//      identity and the membership component — the layer between the
//      individual and the world.
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
#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // GroupId
    //
    // An opaque identifier for a group. The world assigns the meaning —
    // a family, a settlement, a faction — and hands the core opaque
    // numbers. Tagged so a group can never be confused with an ordinary
    // integer or an EntityId: the compiler rejects the wrong argument.
    //-------------------------------------------------------------------------
    class GroupId
    {
    public:
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue = 0;

        //-------------------------------------------------------------------------
        // Constructs an invalid group id (this is also the default).
        //-------------------------------------------------------------------------
        constexpr GroupId() = default;

        //-------------------------------------------------------------------------
        // Constructs a group id from a raw value.
        //-------------------------------------------------------------------------
        explicit constexpr GroupId(ValueType value) noexcept
            : m_Value(value)
        {
        }

        //-------------------------------------------------------------------------
        // The raw value.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr ValueType Value() const noexcept
        {
            return m_Value;
        }

        //-------------------------------------------------------------------------
        // Returns whether this id could name a group.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        constexpr bool IsValid() const noexcept
        {
            return m_Value != InvalidValue;
        }

        friend constexpr bool operator==(GroupId, GroupId) = default;
        friend constexpr bool operator!=(GroupId, GroupId) = default;

    private:
        ValueType m_Value = InvalidValue;
    };

    //-------------------------------------------------------------------------
    // Groups
    //
    // The component: which groups this entity belongs to. An entity can
    // belong to several — a settler is of their family, their settlement,
    // and their faction at once.
    //-------------------------------------------------------------------------
    struct Groups
    {
        std::vector<GroupId> Memberships;
    };
}

namespace std
{
    //-------------------------------------------------------------------------
    // Lets GroupId be a key in unordered containers: hash the raw value.
    //-------------------------------------------------------------------------
    template <>
    struct hash<LCE::Simulation::GroupId>
    {
        [[nodiscard]]
        size_t operator()(const LCE::Simulation::GroupId& id) const noexcept
        {
            return std::hash<LCE::Simulation::GroupId::ValueType>{}(id.Value());
        }
    };
}
