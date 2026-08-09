//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SimulationTickTest.cpp
//
// Purpose:
//
//      Verifies one tick advances every system coherently: needs decay,
//      memory fades, relationships drift, goals grow, and minds get fresh
//      intents — without one system corrupting another.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool SimulationTickTest()
    {
        Simulation::EntityRegistry registry;

        const auto farmer = registry.CreateEntity();
        const auto merchant = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            farmer,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.8f, 0.1f } }
            });

        registry.AddComponent<Simulation::Goals>(
            farmer,
            Simulation::Goals{
                Simulation::Goal{ Simulation::GoalType::Prosper, 0.0f }
            });

        Simulation::Remember(
            registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });

        // One tick: everything moves at once.
        Simulation::Update(registry, 1.0);

        const auto needs = registry.GetComponent<Simulation::Needs>(farmer);
        const auto memory = registry.GetComponent<Simulation::Memory>(farmer);
        const auto relationships = registry.GetComponent<Simulation::Relationships>(farmer);
        const auto goals = registry.GetComponent<Simulation::Goals>(farmer);
        const auto intent = registry.GetComponent<Simulation::Intent>(farmer);

        if (!needs || !memory || !relationships || !goals || !intent)
        {
            return false;
        }

        if (needs->List[0].Value > 0.71f)
        {
            return false;   // hunger decayed
        }

        if (memory->Events[0].Weight > 0.81f)
        {
            return false;   // salience faded
        }

        if (relationships->ByEntity.at(merchant).Trust < 0.14f)
        {
            return false;   // the trade shaped the relationship
        }

        if (goals->Active->Urgency < 0.09f)
        {
            return false;   // the goal grew urgent
        }

        if (intent->Action != Simulation::ActionType::MoveTo)
        {
            return false;   // and the mind decided to go to market
        }

        if (intent->Target != merchant)
        {
            return false;
        }

        // A second tick rewrites the intent, and a mind with no decision
        // loses its intent rather than keeping a stale one.
        registry.RemoveComponent<Simulation::Memory>(farmer);
        registry.RemoveComponent<Simulation::Relationships>(farmer);

        Simulation::Update(registry, 1.0);

        const auto freshIntent = registry.GetComponent<Simulation::Intent>(farmer);

        if (!freshIntent)
        {
            return false;
        }

        if (freshIntent->Action != Simulation::ActionType::Explore)
        {
            return false;   // no known source anymore → explore
        }

        return true;
    }
}
