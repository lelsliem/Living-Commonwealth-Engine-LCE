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
// │               "My life’s like a bad configuration file — full of defaults I never agreed to."
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Configuration.h
//
// Purpose:
//
//      Defines the runtime configuration interface used by LCE.
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

#include <string>
#include <string_view>
#include <unordered_map>

namespace LCE::Config
{
    class Configuration
    {
    public:
        void Set(
            std::string_view name,
            std::string_view value);

        [[nodiscard]]
        bool Has(std::string_view name) const noexcept;

        [[nodiscard]]
        std::string_view Get(std::string_view name) const noexcept;

        //-------------------------------------------------------------------------
        // Visits every (name, value) pair in the configuration. The
        // caller filters what it knows — FromConfiguration reads its
        // keys this way (0.6.0 stone 08: the bond watch-list is a
        // family of sim.bond.threshold.* keys with world-chosen names).
        //-------------------------------------------------------------------------
        template <typename Visitor>
        void ForEach(Visitor&& visitor) const
        {
            for (const auto& [name, value] : m_Values)
            {
                visitor(std::string_view(name), std::string_view(value));
            }
        }

    private:
        std::unordered_map<std::string, std::string> m_Values;
    };
}