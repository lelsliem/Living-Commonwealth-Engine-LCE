//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      BequeathTest.cpp
//
// Purpose:
//
//      Verifies the death-lifecycle stone (0.7.0 stone 10): Bequeath —
//      what an entity bequeaths as it goes. The world names the heirs;
//      the core keeps what salience merits (BequestFloor), fainter
//      (InheritanceScale), their own age intact, and never overwrites
//      the living. Plus: the settlement's feelings toward the dead need
//      no work — they live in survivors' relationship stores and outlive
//      DestroyEntity naturally.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool BequeathTest()
    {
        //-------------------------------------------------------------------------
        // 1. Above the floor, scaled, day kept: the dying entity's facts
        //    at or above BequestFloor reach every heir — fainter, with
        //    their own world day (the story's age survives the transfer).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto dying = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto firstHeir = registry.CreateEntity();
            const auto secondHeir = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                dying,
                Simulation::Memory{
                    {
                        { merchant, Simulation::InteractionKind::Wronged, 1.0f, 100 },
                        { merchant, Simulation::InteractionKind::Trade, 0.3f, 50 },
                    } });

            const Simulation::EntityId heirs[] = { firstHeir, secondHeir };

            const auto count = Simulation::Bequeath(registry, dying, heirs);

            if (count != 2)
            {
                return false;   // the strong fact reached both heirs
            }

            const auto firstMemory =
                registry.GetComponent<Simulation::Memory>(firstHeir);
            const auto secondMemory =
                registry.GetComponent<Simulation::Memory>(secondHeir);

            if (!firstMemory || !secondMemory
                || firstMemory->Events.size() != 1
                || secondMemory->Events.size() != 1)
            {
                return false;
            }

            const auto& inherited = firstMemory->Events[0];

            if (inherited.Kind != Simulation::InteractionKind::Wronged
                || inherited.Other != merchant
                || inherited.Weight != 0.5f      // 1.0 * InheritanceScale
                || inherited.Day != 100)         // the story's own age
            {
                return false;
            }

            const auto& also = secondMemory->Events[0];

            if (also.Other != merchant || also.Day != 100)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. The floor is a line: a fact at exactly the floor passes; a
        //    fact below it stays with the dead.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto dying = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto heir = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                dying,
                Simulation::Memory{
                    {
                        { merchant, Simulation::InteractionKind::Aid, 0.5f },
                        { merchant, Simulation::InteractionKind::Social, 0.49f },
                    } });

            const Simulation::EntityId heirs[] = { heir };

            Simulation::Bequeath(registry, dying, heirs);

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 1
                || heirMemory->Events[0].Kind != Simulation::InteractionKind::Aid)
            {
                return false;   // 0.5 passed, 0.49 stayed
            }
        }

        //-------------------------------------------------------------------------
        // 3. Deterministic: the caller's list order can never leak into
        //    results — scrambled heirs give the same bequest.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry firstWorld;
            Simulation::EntityRegistry secondWorld;

            const auto dyingA = firstWorld.CreateEntity();
            const auto merchantA = firstWorld.CreateEntity();
            const auto childA = firstWorld.CreateEntity();
            const auto childB = firstWorld.CreateEntity();

            firstWorld.AddComponent<Simulation::Memory>(
                dyingA,
                Simulation::Memory{
                    { { merchantA, Simulation::InteractionKind::Wronged, 1.0f, 7 } } });

            const Simulation::EntityId ordered[] = { childA, childB };
            const auto countOrdered = Simulation::Bequeath(firstWorld, dyingA, ordered);

            const auto dyingB = secondWorld.CreateEntity();
            const auto merchantB = secondWorld.CreateEntity();
            const auto childC = secondWorld.CreateEntity();
            const auto childD = secondWorld.CreateEntity();

            secondWorld.AddComponent<Simulation::Memory>(
                dyingB,
                Simulation::Memory{
                    { { merchantB, Simulation::InteractionKind::Wronged, 1.0f, 7 } } });

            const Simulation::EntityId scrambled[] = { childD, childC };
            const auto countScrambled = Simulation::Bequeath(secondWorld, dyingB, scrambled);

            if (countOrdered != countScrambled)
            {
                return false;
            }

            const auto aMemory = firstWorld.GetComponent<Simulation::Memory>(childA);
            const auto bMemory = secondWorld.GetComponent<Simulation::Memory>(childC);

            if (!aMemory || !bMemory || aMemory->Events.size() != 1
                || bMemory->Events.size() != 1
                || aMemory->Events[0].Weight != bMemory->Events[0].Weight
                || aMemory->Events[0].Day != bMemory->Events[0].Day)
            {
                return false;   // same world, same bequest — order-free
            }
        }

        //-------------------------------------------------------------------------
        // 4. Edge safety: no heirs, a dead heir, or the dying naming
        //    itself — none of it breaks, and none of it leaks.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto dying = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto liveHeir = registry.CreateEntity();
            const auto deadHeir = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                dying,
                Simulation::Memory{
                    { { merchant, Simulation::InteractionKind::Trade, 1.0f } } });

            registry.DestroyEntity(deadHeir);

            const Simulation::EntityId mixed[] = { dying, deadHeir, liveHeir };

            const auto count = Simulation::Bequeath(registry, dying, mixed);

            if (count != 1)
            {
                return false;   // only the live, distinct heir received it
            }

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(liveHeir);

            if (!heirMemory || heirMemory->Events.size() != 1)
            {
                return false;
            }

            if (Simulation::Bequeath(
                    registry, dying,
                    std::span<const Simulation::EntityId>{}) != 0)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. Append, never overwrite: the heir's own memories are
        //    untouched — even one of the very same event.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto dying = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto heir = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                dying,
                Simulation::Memory{
                    { { merchant, Simulation::InteractionKind::Trade, 1.0f, 20 } } });

            registry.AddComponent<Simulation::Memory>(
                heir,
                Simulation::Memory{
                    { { merchant, Simulation::InteractionKind::Trade, 1.0f, 20 } } });

            const Simulation::EntityId heirs[] = { heir };

            Simulation::Bequeath(registry, dying, heirs);

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 2)
            {
                return false;   // lived and inherited, side by side
            }

            if (heirMemory->Events[0].Weight != 1.0f)
            {
                return false;   // the lived memory was never touched
            }
        }

        //-------------------------------------------------------------------------
        // 6. The settlement remembers the miller: feelings toward the
        //    dead live in survivors' relationship stores and outlive
        //    DestroyEntity — no core work needed.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto miller = registry.CreateEntity();
            const auto survivor = registry.CreateEntity();

            registry.AddComponent<Simulation::Relationships>(
                survivor,
                Simulation::Relationships{
                    { { miller, { 0.5f, 0.3f } } } });

            registry.DestroyEntity(miller);

            if (registry.IsAlive(miller))
            {
                return false;
            }

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(survivor);

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(miller);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Disposition != 0.5f
                || iterator->second.Trust != 0.3f)
            {
                return false;   // the feeling outlived the owner
            }
        }

        return true;
    }
}
