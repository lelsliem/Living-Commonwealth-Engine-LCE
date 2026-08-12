//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      BondThresholdTest.cpp
//
// Purpose:
//
//      Verifies the bond-threshold watch-list and the
//      RelationshipChangedEvent (0.6.0 stone 08): sim.bond.threshold.*
//      tuning keys name the lines the world drew across disposition, and
//      the event fires edge-triggered when an experience crosses one —
//      bond formation and bond souring are news, resting is not.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"
#include "LCE/Simulation/Substrate/WorldTime.h"

#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace LCE::Tests
{
    //-------------------------------------------------------------------------
    // A small capture for the events the suite asserts on: the sequence of
    // threshold names, the dispositions at each crossing, and the count.
    //-------------------------------------------------------------------------
    namespace
    {
        struct Crossings
        {
            int Count = 0;
            std::vector<std::string> Names;
            std::vector<float> Dispositions;
            std::vector<std::uint64_t> Days;

            void Subscribe(LCE::Events::EventBus& bus)
            {
                bus.Subscribe(
                    std::type_index(typeid(Simulation::RelationshipChangedEvent)),
                    [this](const LCE::Events::Event& event)
                    {
                        const auto& e =
                            static_cast<const Simulation::RelationshipChangedEvent&>(event);
                        ++Count;
                        Names.push_back(e.Threshold);
                        Dispositions.push_back(e.Disposition);
                        Days.push_back(e.Day);
                    });
            }
        };

        Simulation::SimulationTuning WithBond(
            const char* name,
            float value)
        {
            Config::Configuration config;

            config.Set(std::string("sim.bond.threshold.") + name,
                std::to_string(value));

            return Simulation::SimulationTuning::FromConfiguration(config);
        }
    }

    bool BondThresholdTest()
    {
        //-------------------------------------------------------------------------
        // 1. No thresholds configured → no events. Trust changes alone
        //    never cross a line — the watch-list watches disposition.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            Crossings crossings;
            crossings.Subscribe(bus);

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Success },
                {}, &bus);

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Aid,
                  Simulation::OutcomeResult::Success },
                {}, &bus);

            if (crossings.Count != 0)
            {
                return false;   // no lines drawn — no crossings possible
            }
        }

        //-------------------------------------------------------------------------
        // 2. An up-cross fires, with the full payload: subject, other,
        //    disposition, threshold name, and the world day.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            Crossings crossings;
            crossings.Subscribe(bus);

            // Three successful aids: 0.0 → 0.1 → 0.2 → 0.3. The third
            // crosses the friend line exactly.
            for (int i = 0; i < 3; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { merchant, Simulation::InteractionKind::Aid,
                      Simulation::OutcomeResult::Success },
                    tuning, &bus, Simulation::WorldTime{ 42 });
            }

            if (crossings.Count != 1
                || crossings.Names[0] != "friend"
                || crossings.Dispositions[0] < 0.29f
                || crossings.Dispositions[0] > 0.31f
                || crossings.Days[0] != 42)
            {
                return false;   // the crossing, named, at world day 42
            }
        }

        //-------------------------------------------------------------------------
        // 3. A down-cross fires too — a feud is news, not just a bond.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto raider = registry.CreateEntity();

            const auto tuning = WithBond("enemy", -0.6f);

            Crossings crossings;
            crossings.Subscribe(bus);

            // Three wrongs: 0.0 → -0.25 → -0.5 → -0.75. The third crosses
            // the enemy line.
            for (int i = 0; i < 3; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { raider, Simulation::InteractionKind::Wronged,
                      Simulation::OutcomeResult::Success },
                    tuning, &bus);
            }

            if (crossings.Count != 1
                || crossings.Names[0] != "enemy"
                || crossings.Dispositions[0] > -0.74f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. Edge-triggered, not level-triggered: one up-cross, silence
        //    while resting above the line, one down-cross when the bond
        //    sours. The count is crossings, not status.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            Crossings crossings;
            crossings.Subscribe(bus);

            // Up: 0.0 → 0.1 → 0.2 → 0.3 (crosses on the third).
            for (int i = 0; i < 3; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { merchant, Simulation::InteractionKind::Aid,
                      Simulation::OutcomeResult::Success },
                    tuning, &bus);
            }

            // Above the line: 0.4, then 0.5 — no re-fire.
            for (int i = 0; i < 2; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { merchant, Simulation::InteractionKind::Aid,
                      Simulation::OutcomeResult::Success },
                    tuning, &bus);
            }

            // Down: 0.5 → 0.25 — the friend line is crossed downward.
            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Wronged,
                  Simulation::OutcomeResult::Success },
                tuning, &bus);

            if (crossings.Count != 2
                || crossings.Names[0] != "friend"
                || crossings.Names[1] != "friend"
                || crossings.Dispositions[0] < 0.29f   // the up-cross
                || crossings.Dispositions[1] > 0.26f)  // the down-cross
            {
                return false;   // two moments, not a running status
            }
        }

        //-------------------------------------------------------------------------
        // 5. One mutation crossing two lines fires both events — in the
        //    stable, name-sorted order the tuning promises.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            Config::Configuration config;

            config.Set("sim.bond.threshold.friend", "0.3");
            config.Set("sim.bond.threshold.close", "0.35");
            config.Set("sim.disposition.gain", "0.5");   // one aid jumps 0.5

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            Crossings crossings;
            crossings.Subscribe(bus);

            // 0.0 → 0.5 in one step: crosses both 0.3 and 0.35.
            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Aid,
                  Simulation::OutcomeResult::Success },
                tuning, &bus);

            if (crossings.Count != 2
                || crossings.Names[0] != "close"     // sorted by name:
                || crossings.Names[1] != "friend")   // close < friend
            {
                return false;   // deterministic event order
            }
        }

        //-------------------------------------------------------------------------
        // 6. The tuning parse: sim.bond.threshold.* keys become the
        //    watch-list, sorted by name; broken values and unknown keys
        //    are ignored — a bad line must never break the world.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.bond.threshold.friend", "0.3");
            config.Set("sim.bond.threshold.enemy", "-0.6");
            config.Set("sim.bond.threshold.broken", "not-a-number");
            config.Set("market.open.hour", "9");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            if (tuning.BondThresholds.size() != 2
                || tuning.BondThresholds[0].Name != "enemy"
                || tuning.BondThresholds[0].Value != -0.6f
                || tuning.BondThresholds[1].Name != "friend"
                || tuning.BondThresholds[1].Value != 0.3f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 7. Remember emits too — an experience remembered is an
        //    experience, with the same crossing rule.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto neighbour = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            Crossings crossings;
            crossings.Subscribe(bus);

            // Three remembered socials: 0.0 → 0.1 → 0.2 → 0.3.
            for (int i = 0; i < 3; ++i)
            {
                Simulation::Remember(
                    registry, farmer,
                    { neighbour, Simulation::InteractionKind::Social, 1.0f },
                    tuning, {}, &bus);
            }

            if (crossings.Count != 1
                || crossings.Names[0] != "friend")
            {
                return false;   // Remember is a mutation like any other
            }
        }

        //-------------------------------------------------------------------------
        // 8. Drift is quiet: cooling below the line over time is a
        //    dissolve, not an event — the adapter re-derives bonds from
        //    the relationship state it always has.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            Crossings crossings;
            crossings.Subscribe(bus);

            // Up to 0.5: one crossing on the way.
            for (int i = 0; i < 5; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { merchant, Simulation::InteractionKind::Aid,
                      Simulation::OutcomeResult::Success },
                    tuning, &bus);
            }

            // Drift pulls 0.5 → 0.25 over ten seconds — below the line —
            // and nothing fires.
            Simulation::Update(registry, 10.0, tuning, &bus);

            if (crossings.Count != 1)
            {
                return false;   // one crossing; the dissolve stays silent
            }
        }

        //-------------------------------------------------------------------------
        // 9. No bus → nothing publishes, nothing explodes. The
        //    observation channel stays optional (ADR-0014).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            for (int i = 0; i < 3; ++i)
            {
                Simulation::ReportOutcome(
                    registry, farmer,
                    { merchant, Simulation::InteractionKind::Aid,
                      Simulation::OutcomeResult::Success },
                    tuning);
            }
        }

        //-------------------------------------------------------------------------
        // 10. A world fact (invalid Other) shapes no relationship — it
        //     can never cross a bond line.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();

            const auto tuning = WithBond("friend", 0.3f);

            Crossings crossings;
            crossings.Subscribe(bus);

            Simulation::Remember(
                registry, farmer,
                { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f },
                tuning, {}, &bus);

            if (crossings.Count != 0)
            {
                return false;   // nobody to bond with — no event
            }
        }

        return true;
    }
}
