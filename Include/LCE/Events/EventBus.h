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
// │          "The best way to predict the future is to create it."          │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//
//      EventBus.h
//
// Purpose:
//
//      Defines the public interface for publishing and subscribing to LCE
//      events.
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

#include "LCE/Events/Event.h"

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace LCE::Events
{
    class EventBus
    {
    public:
        using EventHandler = std::function<void(const Event&)>;

        EventBus() = default;
        ~EventBus() = default;

        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

        void Subscribe(
            std::type_index eventType,
            EventHandler handler);

        void Publish(const Event& event);

    private:
        std::unordered_map<
            std::type_index,
            std::vector<EventHandler>> m_Handlers;
    };
}