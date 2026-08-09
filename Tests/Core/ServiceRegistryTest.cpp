//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      ServiceRegistryTest.cpp
//
// Purpose:
//
//      Verifies the Service Registry stores, retrieves, and replaces services
//      by type without leaking state between unrelated registrations.
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

#include "LCE/Runtime/ServiceRegistry.h"

#include <memory>

namespace
{
    struct TestService
    {
        int Value = 0;
    };

    struct OtherService
    {
    };
}

namespace LCE::Tests
{
    bool ServiceRegistryTest()
    {
        Runtime::ServiceRegistry registry;

        // An unregistered service is neither present nor obtainable.
        if (registry.Has<TestService>())
        {
            return false;
        }

        if (registry.Get<TestService>())
        {
            return false;
        }

        // Registering makes the service obtainable and preserves its state.
        auto first = std::make_shared<TestService>();
        first->Value = 42;

        registry.Register(first);

        if (!registry.Has<TestService>())
        {
            return false;
        }

        const auto retrieved = registry.Get<TestService>();

        if (!retrieved)
        {
            return false;
        }

        if (retrieved != first)
        {
            return false;
        }

        if (retrieved->Value != 42)
        {
            return false;
        }

        // Registering a second instance replaces the first (ADR-0004:
        // replaceable implementations).
        auto second = std::make_shared<TestService>();
        second->Value = 7;

        registry.Register(second);

        if (registry.Get<TestService>() != second)
        {
            return false;
        }

        // Unrelated types do not collide.
        if (registry.Has<OtherService>())
        {
            return false;
        }

        return true;
    }
}
