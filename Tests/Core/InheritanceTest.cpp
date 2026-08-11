//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      InheritanceTest.cpp
//
// Purpose:
//
//      Verifies the generational-handoff stone (0.7.0 stone 11):
//      InheritMemory — descendants inherit memory, selectively. The
//      world's predicate selects, the core scales and ages, and the
//      heir's own memories are never touched. The integration block is
//      the roadmap's proof: a settler's grandson carries the feud to
//      the market and refuses the merchant — the memory outlived its
//      owner, on two channels composing (the story on the memory
//      channel, the grudge on the group echo).
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool InheritanceTest()
    {
        //-------------------------------------------------------------------------
        // 1. The story travels: facts copied into the heir's memory,
        //    scaled, their own day kept, the count returned.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto ancestor = registry.CreateEntity();
            const auto heir = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                ancestor,
                Simulation::Memory{
                    {
                        { merchant, Simulation::InteractionKind::Wronged, 1.0f, 300 },
                        { {}, Simulation::InteractionKind::WeatherRain, 1.0f, 100 },
                    } });

            const auto count = Simulation::InheritMemory(registry, heir, ancestor);

            if (count != 2)
            {
                return false;
            }

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 2)
            {
                return false;
            }

            const auto& feud = heirMemory->Events[0];

            if (feud.Other != merchant
                || feud.Kind != Simulation::InteractionKind::Wronged
                || feud.Weight != 0.5f      // 1.0 * InheritanceScale
                || feud.Day != 300)         // the story's own age
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. The world's patience: LegacyMaxAgeDays filters old facts;
        //    unstamped facts pass; 0 keeps everything.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto ancestor = registry.CreateEntity();
            const auto heir = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                ancestor,
                Simulation::Memory{
                    {
                        { merchant, Simulation::InteractionKind::Trade, 1.0f, 10 },
                        { merchant, Simulation::InteractionKind::Social, 1.0f, 200 },
                        { merchant, Simulation::InteractionKind::Aid, 1.0f, 0 },
                    } });

            Simulation::SimulationTuning tuning;
            tuning.LegacyMaxAgeDays = 100;

            const auto count = Simulation::InheritMemory(
                registry, heir, ancestor, tuning, Simulation::WorldTime{ 300 });

            if (count != 2)
            {
                return false;   // the 10-day-old and the unstamped pass
            }

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 2)
            {
                return false;   // the day-200 and unstamped facts travelled
            }

            // The kept pair: day 200 (100 days old — at the line, not
            // over it) and the unstamped day 0. The day-10 fact — 290
            // days old — stayed behind.
            for (const auto& event : heirMemory->Events)
            {
                if (event.Day == 10)
                {
                    return false;
                }
            }

            const auto all = Simulation::InheritMemory(
                registry, heir, ancestor, Simulation::SimulationTuning{});

            if (all != 3)
            {
                return false;   // maxAge 0 keeps everything
            }
        }

        //-------------------------------------------------------------------------
        // 3. The world's selectivity: the predicate chooses what travels
        //    (the same vocabulary-free seam as QueryWhere).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto ancestor = registry.CreateEntity();
            const auto heir = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                ancestor,
                Simulation::Memory{
                    {
                        { merchant, Simulation::InteractionKind::Trade, 1.0f },
                        { merchant, Simulation::InteractionKind::Wronged, 1.0f },
                        { merchant, Simulation::InteractionKind::Aid, 1.0f },
                    } });

            const auto count = Simulation::InheritMemory(
                registry, heir, ancestor,
                Simulation::SimulationTuning{},
                Simulation::WorldTime{},
                [](const Simulation::MemoryEvent& event)
                {
                    return event.Kind == Simulation::InteractionKind::Wronged
                        || event.Kind == Simulation::InteractionKind::Combat;
                });

            if (count != 1)
            {
                return false;   // only the feud travelled
            }

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 1
                || heirMemory->Events[0].Kind != Simulation::InteractionKind::Wronged)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. Personal memory is never touched: the heir's own facts stay
        //    exactly as they were.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto ancestor = registry.CreateEntity();
            const auto heir = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                ancestor,
                Simulation::Memory{
                    { { merchant, Simulation::InteractionKind::Wronged, 1.0f } } });

            registry.AddComponent<Simulation::Memory>(
                heir,
                Simulation::Memory{
                    { { merchant, Simulation::InteractionKind::Trade, 1.0f, 5 } } });

            Simulation::InheritMemory(registry, heir, ancestor);

            const auto heirMemory =
                registry.GetComponent<Simulation::Memory>(heir);

            if (!heirMemory || heirMemory->Events.size() != 2
                || heirMemory->Events[0].Kind != Simulation::InteractionKind::Trade
                || heirMemory->Events[0].Weight != 1.0f
                || heirMemory->Events[0].Day != 5)
            {
                return false;   // the lived memory is intact
            }
        }

        //-------------------------------------------------------------------------
        // 5. Edge safety: same entity, a missing memory, a dead ancestor
        //    — none of it breaks, none of it travels.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto alive = registry.CreateEntity();
            const auto dead = registry.CreateEntity();

            registry.DestroyEntity(dead);

            if (Simulation::InheritMemory(registry, alive, alive) != 0)
            {
                return false;
            }

            if (Simulation::InheritMemory(registry, alive, dead) != 0)
            {
                return false;
            }

            if (Simulation::InheritMemory(registry, dead, alive) != 0)
            {
                return false;
            }

            const auto ghost = registry.CreateEntity();

            if (Simulation::InheritMemory(registry, ghost, alive) != 0)
            {
                return false;   // the ancestor has no memory to give
            }
        }

        //-------------------------------------------------------------------------
        // 6. The proof: a settler's grandson carries the feud to the
        //    market and refuses the merchant. The story travels on the
        //    memory channel (InheritMemory); the grudge rides the group
        //    echo (InheritGroupAttitudes); Decide refuses the merchant
        //    who wronged the family and walks to the other stall.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto ancestor = registry.CreateEntity();
            const auto parent = registry.CreateEntity();
            const auto merchantA = registry.CreateEntity();
            const auto merchantB = registry.CreateEntity();

            // The family: one wrong to one settler turns them all cold.
            registry.AddComponent<Simulation::Groups>(
                ancestor, Simulation::Groups{ { Simulation::GroupId{ 5 } } });
            registry.AddComponent<Simulation::Groups>(
                parent, Simulation::Groups{ { Simulation::GroupId{ 5 } } });

            // The ancestor knew both stalls — traded with A from olden
            // days (day 50), traded fairly with B (day 200), and was
            // wronged by A (day 100). The echo carries the coldness.
            Simulation::Remember(
                registry, ancestor,
                { merchantA, Simulation::InteractionKind::Trade, 1.0f },
                Simulation::SimulationTuning{}, Simulation::WorldTime{ 50 });
            Simulation::Remember(
                registry, ancestor,
                { merchantA, Simulation::InteractionKind::Wronged, 1.0f },
                Simulation::SimulationTuning{}, Simulation::WorldTime{ 100 });
            Simulation::Remember(
                registry, ancestor,
                { merchantB, Simulation::InteractionKind::Trade, 1.0f },
                Simulation::SimulationTuning{}, Simulation::WorldTime{ 200 });

            // The grandchild: a mind (hungry), born into the family.
            const auto grandchild = registry.CreateEntity();

            registry.AddComponent<Simulation::Needs>(
                grandchild,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } } });
            registry.AddComponent<Simulation::Groups>(
                grandchild, Simulation::Groups{ { Simulation::GroupId{ 5 } } });

            // The grudge travels on the group channel: the family's mean
            // disposition toward A is cold; B was never wronged.
            Simulation::InheritGroupAttitudes(
                registry, grandchild, Simulation::GroupId{ 5 });

            // The story travels on the memory channel.
            Simulation::InheritMemory(registry, grandchild, ancestor);

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(grandchild);

            if (!relationships)
            {
                return false;
            }

            const auto towardA = relationships->ByEntity.find(merchantA);

            if (towardA == relationships->ByEntity.end()
                || towardA->second.Disposition >= 0.0f)
            {
                return false;   // the family's grudge reached the grandchild
            }

            // And the decision: hungry, knowing both stalls, the
            // grandchild refuses the merchant who wronged the family and
            // walks to B. No script — the same money-test mechanics, one
            // generation down.
            const auto intent = Simulation::Decide(registry, grandchild);

            if (!intent || intent->Target != merchantB)
            {
                return false;   // the feud outlived its owner
            }
        }

        return true;
    }
}
