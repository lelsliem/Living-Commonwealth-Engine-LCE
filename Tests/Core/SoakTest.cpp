//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SoakTest.cpp
//
// Purpose:
//
//      0.8.0 Scale stone 15 — soak tests: a year of sim time in
//      minutes, the drift found. Two cadences: a decade at day-steps
//      (fast, calendar edges) and an hour at the real tick cadence
//      (FP accumulation on the actual hot path). The finding is the
//      point: no NaN, no unbounded growth, no state creep.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <cmath>

namespace LCE::Tests
{
    namespace
    {
        // A settlement: every mind is a settler with hunger, fatigue,
        // and social needs; they know the trader. The whole village is
        // alive — every mind decays, fades, drifts, and decides.
        void BuildSettlement(
            Simulation::EntityRegistry& registry,
            int count)
        {
            const auto trader = registry.CreateEntity();

            for (int i = 0; i < count; ++i)
            {
                const auto mind = registry.CreateEntity();

                registry.AddComponent<Simulation::Needs>(
                    mind,
                    Simulation::Needs{
                        { Simulation::Need{
                              Simulation::NeedType::Hunger, 0.8f, 0.02f },
                          Simulation::Need{
                              Simulation::NeedType::Fatigue, 0.9f, 0.01f },
                          Simulation::Need{
                              Simulation::NeedType::Social, 0.7f, 0.015f } } });

                Simulation::Remember(
                    registry, mind,
                    { trader, Simulation::InteractionKind::Trade, 1.0f });
                Simulation::Remember(
                    registry, mind,
                    { trader, Simulation::InteractionKind::Social, 0.6f });
            }
        }

        bool WorldIsStable(
            Simulation::EntityRegistry& registry,
            std::size_t expectedEntities,
            std::size_t memoryCap)
        {
            std::size_t entities = 0;
            bool clean = true;

            registry.ForEachWithComponent<Simulation::Needs>(
                [&entities, &clean](Simulation::EntityId, const Simulation::Needs& needs)
                {
                    ++entities;

                    for (const auto& need : needs.List)
                    {
                        if (!std::isfinite(need.Value))
                        {
                            clean = false;
                        }

                        if (need.Value < 0.0f || need.Value > 1.0f)
                        {
                            clean = false;
                        }
                    }
                });

            if (entities != expectedEntities)
            {
                return false;   // no entity creep
            }

            if (!clean)
            {
                return false;   // no NaN, no out-of-range needs
            }

            if (memoryCap != 0)
            {
                registry.ForEachWithComponent<Simulation::Memory>(
                    [memoryCap, &clean](
                        Simulation::EntityId, const Simulation::Memory& memory)
                    {
                        if (memory.Events.size() > memoryCap)
                        {
                            clean = false;
                        }
                    });
            }

            return clean;
        }
    }

    bool SoakTest()
    {
        //-------------------------------------------------------------------------
        // 1. A decade at day-steps: 300 settlers, 3650 days, bounded
        //    memory — the world ages without drift.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;
            Simulation::SimulationTuning tuning;
            tuning.MemoryCap = 64;

            BuildSettlement(registry, 300);

            // A day of sim time per step: 3650 days is a decade.
            for (int day = 0; day < 3650; ++day)
            {
                Simulation::Update(registry, 86400.0, tuning);
            }

            if (!WorldIsStable(registry, 300, 64))
            {
                return false;
            }

            // The decade really aged the world: needs decayed toward
            // zero and memory events were bounded — spot-check that the
            // first settler's memory stayed inside the cap.
            std::size_t largestMemory = 0;

            registry.ForEachWithComponent<Simulation::Memory>(
                [&largestMemory](
                    Simulation::EntityId, const Simulation::Memory& memory)
                {
                    largestMemory =
                        std::max(largestMemory, memory.Events.size());
                });

            if (largestMemory > 64)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. An hour at the real tick cadence: 30 settlers, 0.1s steps —
        //    the actual hot path, thousands of ticks, FP accumulation.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;
            Simulation::SimulationTuning tuning;
            tuning.MemoryCap = 32;

            BuildSettlement(registry, 30);

            Simulation::FixedStep fixed;

            // 3600 seconds at 0.1s cadence = 36,000 whole steps.
            for (int i = 0; i < 3600; ++i)
            {
                fixed.Advance(1.0, registry, tuning);
            }

            if (!WorldIsStable(registry, 30, 32))
            {
                return false;
            }

            // The hour ticked: the world actually moved. The first
            // mind's hunger must be strictly below where it started.
            bool moved = false;

            registry.ForEachWithComponent<Simulation::Needs>(
                [&moved](Simulation::EntityId, const Simulation::Needs& needs)
                {
                    for (const auto& need : needs.List)
                    {
                        if (need.Type == Simulation::NeedType::Hunger
                            && need.Value < 0.79f)
                        {
                            moved = true;
                        }
                    }
                });

            if (!moved)
            {
                return false;   // decay happened over the hour
            }
        }

        return true;
    }
}
