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

        // Desperate hunger (0.7.0 field finding): below sim.hunger.desperate
        // the closed sign is ignored — a starving mind walks to the shut
        // market anyway, so the arrival can land and the refusal happen.
        // A moderate mind still respects the shut door.
        const auto desperate = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            desperate,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.15f, 0.1f } }
            });

        Simulation::Remember(
            registry, desperate, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            registry, desperate,
            { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::SimulationTuning tuning;
        tuning.HungerDesperate = 0.2f;

        Simulation::Update(registry, 0.0, tuning);

        const auto desperateIntent =
            registry.GetComponent<Simulation::Intent>(desperate);

        if (!desperateIntent)
        {
            return false;
        }

        if (desperateIntent->Action != Simulation::ActionType::MoveTo)
        {
            return false;   // desperate hunger pushes the shut door
        }

        if (desperateIntent->Target != merchant)
        {
            return false;   // and it still walks to the remembered trader
        }

        const auto moderate = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            moderate,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
            });

        Simulation::Remember(
            registry, moderate, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            registry, moderate,
            { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::Update(registry, 0.0, tuning);

        const auto moderateIntent =
            registry.GetComponent<Simulation::Intent>(moderate);

        if (!moderateIntent)
        {
            return false;
        }

        if (moderateIntent->Action == Simulation::ActionType::MoveTo)
        {
            return false;   // moderate hunger still respects the shut door
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

        // The personality tie-break (0.8.4): two near-tied needs resolve
        // by a per-need seeded draw, NOT list order. The SAME entity
        // with the SAME needs listed in a DIFFERENT order must make the
        // SAME choice — the draw keys on the need type, never the list
        // index (the QueryWhere discipline).
        {
            Simulation::Rng seed{ 99 };

            const auto mind = registry.CreateEntity();

            // First: Hunger then Fatigue, both near-tied.
            registry.AddComponent<Simulation::Needs>(
                mind,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.30f, 0.1f },
                      Simulation::Need{ Simulation::NeedType::Fatigue, 0.32f, 0.1f } }
                });

            const auto intentHungerFirst =
                Simulation::Decide(registry, mind, &seed);

            if (!intentHungerFirst)
            {
                return false;
            }

            // Second: the same two needs, listed in the other order.
            registry.AddComponent<Simulation::Needs>(
                mind,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Fatigue, 0.32f, 0.1f },
                      Simulation::Need{ Simulation::NeedType::Hunger, 0.30f, 0.1f } }
                });

            const auto intentFatigueFirst =
                Simulation::Decide(registry, mind, &seed);

            if (!intentFatigueFirst)
            {
                return false;
            }

            // Same needs, same entity, same seed, different list order —
            // the same action. List order leaks into nothing.
            if (intentHungerFirst->Action != intentFatigueFirst->Action)
            {
                return false;
            }

            // And the same seed re-rolls identically: determinism holds
            // even for a coin.
            Simulation::Rng seedAgain{ 99 };

            const auto intentRe =
                Simulation::Decide(registry, mind, &seedAgain);

            if (!intentRe || intentRe->Action != intentHungerFirst->Action)
            {
                return false;   // a seeded coin always lands the same way
            }
        }

        return true;
    }
}
