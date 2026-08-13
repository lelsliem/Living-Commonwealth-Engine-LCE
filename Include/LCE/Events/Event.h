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
// │          “My code doesn’t have bugs. It just develops random features.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//
//      Event.h
//
// Purpose:
//
//      Defines the base event contract used by the LCE event system.
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

#include <functional>
#include <memory>
#include <typeindex>
#include <vector>

namespace LCE::Events
{
    //-------------------------------------------------------------------------
    // The base contract for events published through LCE.
    //-------------------------------------------------------------------------
    class Event
    {
    public:
        virtual ~Event() = default;
    };
}
