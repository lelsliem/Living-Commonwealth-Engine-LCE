//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      ObservationTest.cpp
//
// Purpose:
//
//      Verifies the observation events (0.5.0) — the push channel that
//      lets games react instead of polling: EntityCreated when the
//      registry makes a genuinely new entity, IntentProduced when the
//      tick re-decides, OutcomeRecorded when an outcome is reported.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"

namespace LCE::Tests
{
    bool ObservationTest()
    {
        //-------------------------------------------------------------------------
        // 1. EntityCreated fires for a genuinely new entity — and a
        //    snapshot restore does NOT flood the bus with creations.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            int created = 0;
            Simulation::EntityId seen{};

            bus.Subscribe(
                std::type_index(typeid(Simulation::EntityCreatedEvent)),
                [&created, &seen](const LCE::Events::Event& event)
                {
                    const auto& e =
                        static_cast<const Simulation::EntityCreatedEvent&>(event);
                    ++created;
                    seen = e.Id;
                });

            registry.SetEventSink(&bus);

            const auto id = registry.CreateEntity();

            if (created != 1 || seen != id)
            {
                return false;
            }

            // A save/load round trip must not re-announce 637 minds as new.
            const auto snapshot = registry.Capture();
            registry.Clear();
            registry.Restore(snapshot);

            if (created != 1)
            {
                return false;   // restore is a private path — no flood
            }
        }

        //-------------------------------------------------------------------------
        // 2. IntentProduced fires when the tick re-decides a mind.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            registry.SetEventSink(&bus);

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Needs>(
                farmer,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
                });

            Simulation::Remember(
                registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });

            int produced = 0;
            Simulation::EntityId producer{};
            Simulation::ActionType action = Simulation::ActionType::Explore;

            bus.Subscribe(
                std::type_index(typeid(Simulation::IntentProducedEvent)),
                [&produced, &producer, &action](const LCE::Events::Event& event)
                {
                    const auto& e =
                        static_cast<const Simulation::IntentProducedEvent&>(event);
                    ++produced;
                    producer = e.Id;
                    action = e.Intent.Action;
                });

            Simulation::Update(registry, 1.0, {}, &bus);

            if (produced != 1 || producer != farmer || action != Simulation::ActionType::MoveTo)
            {
                return false;   // the farmer decided to go to market
            }
        }

        //-------------------------------------------------------------------------
        // 3. OutcomeRecorded fires when a result is reported back.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            registry.SetEventSink(&bus);

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            int recorded = 0;
            Simulation::OutcomeResult result = Simulation::OutcomeResult::Success;

            bus.Subscribe(
                std::type_index(typeid(Simulation::OutcomeRecordedEvent)),
                [&recorded, &result](const LCE::Events::Event& event)
                {
                    const auto& e =
                        static_cast<const Simulation::OutcomeRecordedEvent&>(event);
                    ++recorded;
                    result = e.Outcome.Result;
                });

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Failure },
                {}, &bus);

            if (recorded != 1 || result != Simulation::OutcomeResult::Failure)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. No sink, no bus — nothing publishes, nothing explodes. The
        //    observation channel is optional (ADR-0014: an input, never
        //    global state).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();

            Simulation::Update(registry, 1.0);

            Simulation::ReportOutcome(
                registry, farmer,
                { farmer, Simulation::InteractionKind::Social,
                  Simulation::OutcomeResult::Success });
        }

        return true;
    }
}
