//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      NeedsTest.cpp
//
// Purpose:
//
//      Verifies needs decay over simulation time and that entities without
//      a Needs component are untouched by the tick.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool NeedsTest()
    {
        Simulation::EntityRegistry registry;

        const auto farmer = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            farmer,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.8f, 0.1f } }
            });

        // One second of simulation: hunger decays by its rate (0.1/s).
        Simulation::Update(registry, 1.0);

        const auto needs = registry.GetComponent<Simulation::Needs>(farmer);

        if (!needs)
        {
            return false;
        }

        const float hunger = needs->List[0].Value;

        if (hunger < 0.69f || hunger > 0.71f)
        {
            return false;
        }

        // Decay clamps at zero — a starving entity cannot go below nothing.
        Simulation::Update(registry, 100.0);

        if (registry.GetComponent<Simulation::Needs>(farmer)->List[0].Value != 0.0f)
        {
            return false;
        }

        // An entity with no Needs component is not simulated at all.
        const auto rock = registry.CreateEntity();

        Simulation::Update(registry, 1.0);

        if (registry.GetComponent<Simulation::Needs>(rock))
        {
            return false;
        }

        return true;
    }
}
