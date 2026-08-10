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

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Simulation.h"

#include <string>

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

        // The market reopens when the fact is forgotten: a strong trade
        // memory (2.0) outlives a weaker closure fact (1.0). While the
        // fact is remembered the villager stays home; once it fades below
        // the forget threshold, trade is possible again.
        const auto villager = registry.CreateEntity();

        registry.AddComponent<Simulation::Needs>(
            villager,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.8f, 0.1f } }
            });

        Simulation::Remember(
            registry, villager, { merchant, Simulation::InteractionKind::Trade, 2.0f });
        Simulation::Remember(
            registry, villager,
            { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::Update(registry, 1.0);

        const auto firstIntent = registry.GetComponent<Simulation::Intent>(villager);

        if (!firstIntent)
        {
            return false;
        }

        if (firstIntent->Action == Simulation::ActionType::MoveTo)
        {
            return false;   // the closure fact must block the trip
        }

        Simulation::Update(registry, 5.0);   // the fact fades to nothing

        const auto reopened = registry.GetComponent<Simulation::Intent>(villager);

        if (!reopened)
        {
            return false;
        }

        if (reopened->Action != Simulation::ActionType::MoveTo)
        {
            return false;   // forgotten fact → the market is open again
        }

        if (reopened->Target != merchant)
        {
            return false;
        }

        // Tuning is an input; the adapter builds it from the Configuration
        // service via the 0.5.0 factory. Prove the pattern here: a
        // slow-fade tuning keeps a memory alive longer than the default
        // would.
        Config::Configuration config;
        config.Set("sim.memory.fade", "0.05");
        config.Set("sim.memory.forget", "0.05");

        const auto slow =
            Simulation::SimulationTuning::FromConfiguration(config);

        const auto hoarder = registry.CreateEntity();

        Simulation::Remember(
            registry, hoarder, { merchant, Simulation::InteractionKind::Trade, 1.0f });

        Simulation::Update(registry, 1.0, slow);

        const auto keptMemory = registry.GetComponent<Simulation::Memory>(hoarder);

        if (!keptMemory)
        {
            return false;
        }

        // The default fade would leave 0.80; the configured slow fade
        // leaves 0.95. Configuration reached into the simulation.
        if (keptMemory->Events[0].Weight < 0.90f)
        {
            return false;
        }

        return true;
    }
}
