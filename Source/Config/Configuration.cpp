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
// │               “All that we are is the result of what we have thought.” — Buddha
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Configuration.cpp
//
// Purpose:
//
//      Implements the LCE runtime configuration system.
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

#include "LCE/Config/Configuration.h"

namespace LCE::Config
{
    void Configuration::Set(std::string_view name, std::string_view value)
    {
        m_Values[std::string(name)] = std::string(value);
    }

    bool Configuration::Has(std::string_view name) const noexcept
    {
        return m_Values.find(std::string(name)) != m_Values.end();
    }

    std::string_view Configuration::Get(std::string_view name) const noexcept
    {
        const auto iterator = m_Values.find(std::string(name));

        if (iterator == m_Values.end())
        {
            return {};
        }

        return iterator->second;
    }
}