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

        // The market is closed: a world fact (invalid Other, Trade kind)
        // blocks the trip while it is remembered. The farmer knows the
        // merchant, but today there is no trade — explore instead.
        const auto closed = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            closed,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
            });

        Simulation::Remember(
            registry, closed, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            registry, closed,
            { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::Update(registry, 0.0);

        const auto closedIntent = registry.GetComponent<Simulation::Intent>(closed);

        if (!closedIntent)
        {
            return false;
        }

        if (closedIntent->Action == Simulation::ActionType::MoveTo)
        {
            return false;   // the world fact must have blocked the trip
        }

        // Danger awareness: safety is urgent and a fight is remembered —
        // the mind flees the one it fought.
        const auto wary = registry.CreateEntity();
        const auto bandit = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            wary,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Safety, 0.2f, 0.1f } }
            });

        Simulation::Remember(
            registry, wary, { bandit, Simulation::InteractionKind::Combat, 1.0f });

        Simulation::Update(registry, 0.0);

        const auto fleeIntent = registry.GetComponent<Simulation::Intent>(wary);

        if (!fleeIntent)
        {
            return false;
        }

        if (fleeIntent->Action != Simulation::ActionType::Flee)
        {
            return false;
        }

        if (fleeIntent->Target != bandit)
        {
            return false;   // fled from the one remembered as dangerous
        }

        // Safety with no remembered threat → no decision at all: you
        // can't flee from nothing.
        const auto restless = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            restless,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Safety, 0.2f, 0.1f } }
            });

        Simulation::Update(registry, 0.0);

        if (registry.GetComponent<Simulation::Intent>(restless))
        {
            return false;   // no threat, no intent
        }

        return true;
    }
}
