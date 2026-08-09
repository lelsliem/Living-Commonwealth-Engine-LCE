//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      MemoryTest.cpp
//
// Purpose:
//
//      Verifies experiences are recorded, salience fades over time, and
//      events are forgotten below the threshold.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool MemoryTest()
    {
        Simulation::EntityRegistry registry;

        const auto farmer = registry.CreateEntity();
        const auto merchant = registry.CreateEntity();

        // An experience is recorded.
        Simulation::Remember(
            registry,
            farmer,
            { merchant, Simulation::InteractionKind::Trade, 1.0f });

        const auto memory = registry.GetComponent<Simulation::Memory>(farmer);

        if (!memory)
        {
            return false;
        }

        if (memory->Events.size() != 1)
        {
            return false;
        }

        if (memory->Events[0].Other != merchant)
        {
            return false;
        }

        // Salience fades (0.2/s): after two seconds it has lost 0.4.
        Simulation::Update(registry, 2.0);

        const float weight = memory->Events[0].Weight;

        if (weight < 0.59f || weight > 0.61f)
        {
            return false;
        }

        // Enough time erases the memory entirely (threshold 0.1).
        Simulation::Update(registry, 3.0);

        if (!memory->Events.empty())
        {
            return false;
        }

        // A fresh experience returns (reinforcement).
        Simulation::Remember(
            registry,
            farmer,
            { merchant, Simulation::InteractionKind::Trade, 1.0f });

        if (memory->Events.size() != 1)
        {
            return false;
        }

        if (memory->Events[0].Weight < 0.99f || memory->Events[0].Weight > 1.01f)
        {
            return false;
        }

        return true;
    }
}
