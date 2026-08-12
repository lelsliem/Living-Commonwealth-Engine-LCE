//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EntityRegistryTest.cpp
//
// Purpose:
//
//      Verifies the Entity Registry: creation, destruction, generational
//      reuse, and component attachment — and that stale IDs can never
//      alias a live entity.
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

#include "LCE/Simulation/Entity/EntityRegistry.h"

#include <string>

namespace
{
    struct Health
    {
        int Current = 0;
        int Maximum = 0;
    };

    struct Name
    {
        std::string Text;
    };
}

namespace LCE::Tests
{
    bool EntityRegistryTest()
    {
        Simulation::EntityRegistry registry;

        // A default-constructed ID is invalid and never alive.
        if (Simulation::EntityId{}.IsValid())
        {
            return false;
        }

        if (registry.IsAlive(Simulation::EntityId{}))
        {
            return false;
        }

        // Creating an entity yields a valid, alive, non-zero ID.
        const auto first = registry.CreateEntity();

        if (!first.IsValid())
        {
            return false;
        }

        if (first.Value() == Simulation::EntityId::InvalidValue)
        {
            return false;
        }

        if (!registry.IsAlive(first))
        {
            return false;
        }

        // Two creations yield distinct IDs.
        const auto second = registry.CreateEntity();

        if (first == second)
        {
            return false;
        }

        // Components: add, then get back the same instance with its value.
        registry.AddComponent<Health>(first, Health{ 50, 100 });

        if (!registry.HasComponent<Health>(first))
        {
            return false;
        }

        const auto health = registry.GetComponent<Health>(first);

        if (!health)
        {
            return false;
        }

        if (health->Current != 50 || health->Maximum != 100)
        {
            return false;
        }

        // A missing component yields an empty pointer.
        if (registry.GetComponent<Health>(second))
        {
            return false;
        }

        // Unrelated component types do not collide.
        registry.AddComponent<Name>(first, Name{ "Codsworth" });

        if (!registry.HasComponent<Name>(first))
        {
            return false;
        }

        if (!registry.HasComponent<Health>(first))
        {
            return false;
        }

        if (registry.GetComponent<Name>(first)->Text != "Codsworth")
        {
            return false;
        }

        // RemoveComponent: gone from Has and Get.
        registry.RemoveComponent<Health>(first);

        if (registry.HasComponent<Health>(first))
        {
            return false;
        }

        if (registry.GetComponent<Health>(first))
        {
            return false;
        }

        // Destroying an entity drops every component it owns.
        registry.AddComponent<Health>(first, Health{ 1, 1 });

        registry.DestroyEntity(first);

        if (registry.IsAlive(first))
        {
            return false;
        }

        if (registry.GetComponent<Health>(first))
        {
            return false;
        }

        if (registry.GetComponent<Name>(first))
        {
            return false;
        }

        // Destroying one entity must not touch another.
        if (!registry.IsAlive(second))
        {
            return false;
        }

        // The destroyed slot is reused with a bumped generation: the new
        // entity differs from the old, the old ID is stale, the new is live.
        const auto reused = registry.CreateEntity();

        if (reused == first)
        {
            return false;
        }

        if (registry.IsAlive(first))
        {
            return false;   // stale ID must never alias the reused slot
        }

        if (!registry.IsAlive(reused))
        {
            return false;
        }

        // The reused entity is fully independent: its own components work.
        registry.AddComponent<Health>(reused, Health{ 7, 7 });

        if (registry.GetComponent<Health>(reused)->Current != 7)
        {
            return false;
        }

        // Component access on a destroyed ID is a no-op.
        registry.AddComponent<Health>(first, Health{ 99, 99 });

        if (registry.HasComponent<Health>(first))
        {
            return false;
        }

        // ForEachWithComponent visits exactly the entities that have the
        // type — how systems find their subjects.
        registry.AddComponent<Name>(second, Name{ "Piper" });

        int visited = 0;
        Simulation::EntityId visitedId;

        registry.ForEachWithComponent<Name>(
            [&visited, &visitedId](Simulation::EntityId id, Name&)
            {
                ++visited;
                visitedId = id;
            });

        if (visited != 1)
        {
            return false;
        }

        if (visitedId != second)
        {
            return false;
        }

        return true;
    }
}
