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
// │        "Every great system is a collection of services that agree."
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      ServiceRegistry.h
//
// Purpose:
//
//      Defines the container through which LCE services are registered and
//      obtained. Subsystems are given what they need; they never reach out
//      to find it (ADR-0015).
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

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace LCE::Runtime
{
    class ServiceRegistry
    {
    public:
        ServiceRegistry() = default;
        ~ServiceRegistry() = default;

        ServiceRegistry(const ServiceRegistry&) = delete;
        ServiceRegistry& operator=(const ServiceRegistry&) = delete;

        //-------------------------------------------------------------------------
        // Registers a service under its static type T.
        //
        // Overwrites any previous registration — implementations are
        // replaceable (ADR-0004).
        //-------------------------------------------------------------------------
        template <typename T>
        void Register(std::shared_ptr<T> service);

        //-------------------------------------------------------------------------
        // Returns whether a service of type T is registered.
        //-------------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        bool Has() const noexcept;

        //-------------------------------------------------------------------------
        // Returns the registered service of type T, or an empty pointer if
        // none is registered.
        //-------------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        std::shared_ptr<T> Get() const noexcept;

    private:
        std::unordered_map<
            std::type_index,
            std::shared_ptr<void>> m_Services;
    };

    //-------------------------------------------------------------------------
    // Implementation
    //
    // The registry is header-only: the type-erased storage requires the
    // template definitions to be visible at the call site. There is no
    // separate source file to keep in sync.
    //-------------------------------------------------------------------------

    template <typename T>
    void ServiceRegistry::Register(std::shared_ptr<T> service)
    {
        m_Services[std::type_index(typeid(T))] = std::move(service);
    }

    template <typename T>
    bool ServiceRegistry::Has() const noexcept
    {
        return m_Services.contains(std::type_index(typeid(T)));
    }

    template <typename T>
    std::shared_ptr<T> ServiceRegistry::Get() const noexcept
    {
        const auto iterator = m_Services.find(std::type_index(typeid(T)));

        if (iterator == m_Services.end())
        {
            return {};
        }

        return std::static_pointer_cast<T>(iterator->second);
    }
}
