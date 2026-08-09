//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      BehaviourTest.cpp
//
// Purpose:
//
//      The milestone's proof: a hungry farmer who knows and trusts a
//      merchant decides to go to market. No quest script fired anywhere.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool BehaviourTest()
    {
        Simulation::EntityRegistry registry;

        const auto farmer = registry.CreateEntity();
        const auto merchant = registry.CreateEntity();

        // The farmer is hungry, remembers fair trade with the merchant,
        // and trusts them.
        registry.AddComponent<Simulation::Needs>(
            farmer,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
            });

        Simulation::Remember(
            registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::Update(registry, 1.0);

        const auto intent = registry.GetComponent<Simulation::Intent>(farmer);

        // They're hungry, they know the merchant, they understand the road.
        if (!intent)
        {
            return false;
        }

        if (intent->Action != Simulation::ActionType::MoveTo)
        {
            return false;
        }

        if (intent->Target != merchant)
        {
            return false;
        }

        // A hungry farmer with NO memory of any source explores instead.
        const auto wanderer = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            wanderer,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
            });

        Simulation::Update(registry, 0.0);

        const auto wanderIntent = registry.GetComponent<Simulation::Intent>(wanderer);

        if (!wanderIntent)
        {
            return false;
        }

        if (wanderIntent->Action != Simulation::ActionType::Explore)
        {
            return false;
        }

        // A tired farmer rests.
        const auto tired = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            tired,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Fatigue, 0.2f, 0.1f } }
            });

        Simulation::Update(registry, 0.0);

        const auto tiredIntent = registry.GetComponent<Simulation::Intent>(tired);

        if (!tiredIntent)
        {
            return false;
        }

        if (tiredIntent->Action != Simulation::ActionType::Rest)
        {
            return false;
        }

        return true;
    }
}
