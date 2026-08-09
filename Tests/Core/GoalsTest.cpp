//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      GoalsTest.cpp
//
// Purpose:
//
//      Verifies goal urgency grows while the goal goes unserved.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool GoalsTest()
    {
        Simulation::EntityRegistry registry;

        const auto settler = registry.CreateEntity();

        registry.AddComponent<Simulation::Goals>(
            settler,
            Simulation::Goals{
                Simulation::Goal{ Simulation::GoalType::Prosper, 0.0f }
            });

        // Urgency grows at 0.1 per second.
        Simulation::Update(registry, 1.0);

        const auto goals = registry.GetComponent<Simulation::Goals>(settler);

        if (!goals || !goals->Active)
        {
            return false;
        }

        const float first = goals->Active->Urgency;

        if (first < 0.09f || first > 0.11f)
        {
            return false;
        }

        // Unserved goals keep growing.
        Simulation::Update(registry, 4.0);

        const float later = goals->Active->Urgency;

        if (later < 0.49f || later > 0.51f)
        {
            return false;
        }

        return true;
    }
}
