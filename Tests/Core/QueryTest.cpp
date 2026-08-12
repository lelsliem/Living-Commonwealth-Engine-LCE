//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      QueryTest.cpp
//
// Purpose:
//
//      Verifies the query surface (0.5.0): filtered reads with
//      deterministic iteration order — "everyone hungry", "all settlers
//      who remember the raid" — the determinism hook seeded RNG and
//      save-compat stand on.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Mind/Relationships.h"

namespace LCE::Tests
{
    bool QueryTest()
    {
        //-------------------------------------------------------------------------
        // 1. "Everyone hungry": filtered by component value; entities
        //    without the component are not even candidates.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto hungry1 = registry.CreateEntity();
            const auto fed = registry.CreateEntity();
            const auto hungry2 = registry.CreateEntity();
            const auto rock = registry.CreateEntity();   // no Needs at all

            registry.AddComponent<Simulation::Needs>(
                hungry1,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.2f, 0.1f } }
                });

            registry.AddComponent<Simulation::Needs>(
                fed,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.9f, 0.1f } }
                });

            registry.AddComponent<Simulation::Needs>(
                hungry2,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.1f, 0.1f } }
                });

            const auto hungry = registry.QueryWhere<Simulation::Needs>(
                [](Simulation::EntityId, const Simulation::Needs& needs)
                {
                    for (const auto& need : needs.List)
                    {
                        if (need.Value < 0.5f)
                        {
                            return true;
                        }
                    }

                    return false;
                });

            // Deterministic order: ascending EntityId::Value().
            if (hungry.size() != 2 || hungry[0] != hungry1 || hungry[1] != hungry2)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. "All settlers who remember the raid": a cross-component query
        //    — the predicate reaches the registry through the capture.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto witness = registry.CreateEntity();
            const auto newcomer = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                witness,
                Simulation::Memory{
                    { { Simulation::EntityId{}, Simulation::InteractionKind::Combat, 0.9f } }
                });

            registry.AddComponent<Simulation::Memory>(
                newcomer,
                Simulation::Memory{
                    { { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f } }
                });

            const auto raidWitnesses = registry.QueryWhere<Simulation::Memory>(
                [&registry](Simulation::EntityId id, const Simulation::Memory& memory)
                {
                    if (!registry.IsAlive(id))
                    {
                        return false;
                    }

                    for (const auto& event : memory.Events)
                    {
                        if (event.Kind == Simulation::InteractionKind::Combat)
                        {
                            return true;
                        }
                    }

                    return false;
                });

            if (raidWitnesses.size() != 1 || raidWitnesses[0] != witness)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. Determinism across runs: the same query on the same registry
        //    returns the same order, and that order is ascending ID even
        //    when the store's internal order differs (reuse a slot to
        //    shuffle internal layout).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto a = registry.CreateEntity();
            registry.DestroyEntity(a);
            const auto b = registry.CreateEntity();   // reuses a's slot
            const auto c = registry.CreateEntity();

            registry.AddComponent<Simulation::Needs>(
                b,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.2f, 0.1f } }
                });

            registry.AddComponent<Simulation::Needs>(
                c,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.2f, 0.1f } }
                });

            const auto first = registry.QueryWhere<Simulation::Needs>(
                [](Simulation::EntityId, const Simulation::Needs&)
                {
                    return true;
                });

            const auto second = registry.QueryWhere<Simulation::Needs>(
                [](Simulation::EntityId, const Simulation::Needs&)
                {
                    return true;
                });

            if (first != second)
            {
                return false;
            }

            if (first.size() != 2 || first[0].Value() >= first[1].Value())
            {
                return false;   // ascending ID order, guaranteed
            }
        }

        //-------------------------------------------------------------------------
        // 4. Empty store → empty vector; no match → empty vector.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto id = registry.CreateEntity();

            // No Relationships component has ever been attached.
            const auto none = registry.QueryWhere<Simulation::Relationships>(
                [](Simulation::EntityId, const Simulation::Relationships&)
                {
                    return true;
                });

            if (!none.empty())
            {
                return false;
            }

            registry.AddComponent<Simulation::Needs>(
                id,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.9f, 0.1f } }
                });

            const auto noMatch = registry.QueryWhere<Simulation::Needs>(
                [](Simulation::EntityId, const Simulation::Needs& needs)
                {
                    return needs.List[0].Value < 0.5f;   // nobody is hungry
                });

            if (!noMatch.empty())
            {
                return false;
            }
        }

        return true;
    }
}
