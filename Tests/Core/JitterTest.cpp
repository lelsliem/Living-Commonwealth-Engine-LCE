//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      JitterTest.cpp
//
// Purpose:
//
//      Verifies per-mind metabolism (0.5.0): under a seeded Rng, identical
//      minds decay at their own rates — the herd is broken, not by script,
//      but by the same determinism that save/load stands on. Same seed +
//      same entity = same rate, every run; no Rng = behaviour unchanged;
//      sim.jitter is the modder's knob for how strongly minds diverge.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <cmath>

namespace LCE::Tests
{
    namespace
    {
        // Two identical minds: same hunger, same decay rate, same start.
        // The only difference is which entity they are. The start and rate
        // are chosen so ten ticks of decay can never floor the value to
        // zero — clamping would hide the divergence the test exists to see.
        Simulation::EntityId AddFarmer(Simulation::EntityRegistry& registry)
        {
            const auto id = registry.CreateEntity();

            registry.AddComponent<Simulation::Needs>(
                id,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.9f, 0.05f } }
                });

            return id;
        }
    }

    bool JitterTest()
    {
        //-------------------------------------------------------------------------
        // 1. The herd breaks: two identical minds under one seed decay at
        //    different rates, so their hunger diverges over the ticks.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmerA = AddFarmer(registry);
            const auto farmerB = AddFarmer(registry);

            Simulation::Rng rng{ 2026 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(registry, 1.0, {}, nullptr, &rng);
            }

            const auto needA = registry.GetComponent<Simulation::Needs>(farmerA);
            const auto needB = registry.GetComponent<Simulation::Needs>(farmerB);

            if (!needA || !needB || needA->List.empty() || needB->List.empty())
            {
                return false;
            }

            // Both decayed (the sim ran), but not in lockstep — different
            // entities, different metabolisms.
            const auto hungerA = needA->List[0].Value;
            const auto hungerB = needB->List[0].Value;

            if (hungerA >= 0.9f || hungerB >= 0.9f)
            {
                return false;
            }

            if (hungerA == hungerB)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. Determinism holds: two identical worlds under the same seed
        //    produce bit-identical decay — the jitter is a pure function
        //    of (seed, entity), so save/load reproducibility is untouched.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry worldA;
            const auto farmerA = AddFarmer(worldA);
            Simulation::Rng seedA{ 4242 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(worldA, 1.0, {}, nullptr, &seedA);
            }

            Simulation::EntityRegistry worldB;
            const auto farmerB = AddFarmer(worldB);
            Simulation::Rng seedB{ 4242 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(worldB, 1.0, {}, nullptr, &seedB);
            }

            const auto needA = worldA.GetComponent<Simulation::Needs>(farmerA);
            const auto needB = worldB.GetComponent<Simulation::Needs>(farmerB);

            if (!needA || !needB || needA->List.empty() || needB->List.empty())
            {
                return false;
            }

            if (needA->List[0].Value != needB->List[0].Value)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. No Rng, no jitter: without a seed the decay is exactly as it
        //    always was — identical minds stay identical. No existing
        //    caller is affected by the new behaviour.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmerA = AddFarmer(registry);
            const auto farmerB = AddFarmer(registry);

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(registry, 1.0);
            }

            const auto needA = registry.GetComponent<Simulation::Needs>(farmerA);
            const auto needB = registry.GetComponent<Simulation::Needs>(farmerB);

            if (!needA || !needB || needA->List.empty() || needB->List.empty())
            {
                return false;
            }

            if (needA->List[0].Value != needB->List[0].Value)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. The knob works: sim.jitter = 0 turns the spread off even with
        //    an Rng present — the modder can dial the herd-breaking up or
        //    down, or off entirely.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.jitter", "0");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            if (tuning.NeedJitter != 0.0f)
            {
                return false;
            }

            Simulation::EntityRegistry registry;

            const auto farmerA = AddFarmer(registry);
            const auto farmerB = AddFarmer(registry);

            Simulation::Rng rng{ 777 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(registry, 1.0, tuning, nullptr, &rng);
            }

            const auto needA = registry.GetComponent<Simulation::Needs>(farmerA);
            const auto needB = registry.GetComponent<Simulation::Needs>(farmerB);

            if (!needA || !needB || needA->List.empty() || needB->List.empty())
            {
                return false;
            }

            if (needA->List[0].Value != needB->List[0].Value)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. The default is sane: the knob ships at ±15%, and a configured
        //    wider spread makes minds diverge further over the same ticks.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.jitter", "0.5");

            const auto wide = Simulation::SimulationTuning::FromConfiguration(config);

            Simulation::EntityRegistry wideWorld;
            const auto wideA = AddFarmer(wideWorld);
            const auto wideB = AddFarmer(wideWorld);

            Simulation::Rng wideRng{ 555 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(wideWorld, 1.0, wide, nullptr, &wideRng);
            }

            Simulation::EntityRegistry narrowWorld;
            const auto narrowA = AddFarmer(narrowWorld);
            const auto narrowB = AddFarmer(narrowWorld);

            Simulation::Rng narrowRng{ 555 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(narrowWorld, 1.0, {}, nullptr, &narrowRng);
            }

            const auto wideNeedA = wideWorld.GetComponent<Simulation::Needs>(wideA);
            const auto wideNeedB = wideWorld.GetComponent<Simulation::Needs>(wideB);
            const auto narrowNeedA = narrowWorld.GetComponent<Simulation::Needs>(narrowA);
            const auto narrowNeedB = narrowWorld.GetComponent<Simulation::Needs>(narrowB);

            if (!wideNeedA || !wideNeedB || !narrowNeedA || !narrowNeedB
                || wideNeedA->List.empty() || wideNeedB->List.empty()
                || narrowNeedA->List.empty() || narrowNeedB->List.empty())
            {
                return false;
            }

            const auto wideGap = std::abs(wideNeedA->List[0].Value - wideNeedB->List[0].Value);
            const auto narrowGap = std::abs(narrowNeedA->List[0].Value - narrowNeedB->List[0].Value);

            if (wideGap <= narrowGap)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 6. The parent may advance between ticks — the world doesn't
        //    care. The adapter draws births from the same Rng it passes
        //    to Update, so the parent's live state moves between ticks.
        //    Per-entity noise must anchor to the seed, never the live
        //    state: a settled mind keeps its metabolism and its decision
        //    (0.8.x field finding — a near-tied Rest/Explore mind re-
        //    rolled its intent every frame). Two same-seed worlds, one
        //    whose parent advances between ticks, stay identical.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry quietWorld;
            const auto quietA = AddFarmer(quietWorld);
            const auto quietB = AddFarmer(quietWorld);

            Simulation::Rng quietRng{ 31337 };

            for (int tick = 0; tick < 10; ++tick)
            {
                Simulation::Update(quietWorld, 1.0, {}, nullptr, &quietRng);
            }

            Simulation::EntityRegistry busyWorld;
            const auto busyA = AddFarmer(busyWorld);
            const auto busyB = AddFarmer(busyWorld);

            Simulation::Rng busyRng{ 31337 };

            for (int tick = 0; tick < 10; ++tick)
            {
                // The adapter's pattern: births draw from the same Rng
                // between ticks, moving the parent's live state.
                busyRng.Next();
                busyRng.Next();
                busyRng.Next();

                Simulation::Update(busyWorld, 1.0, {}, nullptr, &busyRng);
            }

            const auto quietNeedA = quietWorld.GetComponent<Simulation::Needs>(quietA);
            const auto quietNeedB = quietWorld.GetComponent<Simulation::Needs>(quietB);
            const auto busyNeedA = busyWorld.GetComponent<Simulation::Needs>(busyA);
            const auto busyNeedB = busyWorld.GetComponent<Simulation::Needs>(busyB);

            if (!quietNeedA || !quietNeedB || !busyNeedA || !busyNeedB
                || quietNeedA->List.empty() || quietNeedB->List.empty()
                || busyNeedA->List.empty() || busyNeedB->List.empty())
            {
                return false;
            }

            // Bit-identical decay: the parent's advancement never leaked
            // into anyone's metabolism.
            if (quietNeedA->List[0].Value != busyNeedA->List[0].Value
                || quietNeedB->List[0].Value != busyNeedB->List[0].Value)
            {
                return false;
            }

            // And the decisions match too — no re-rolled confidence from
            // a moved parent.
            const auto quietIntentA = quietWorld.GetComponent<Simulation::Intent>(quietA);
            const auto busyIntentA = busyWorld.GetComponent<Simulation::Intent>(busyA);

            if (quietIntentA == nullptr || busyIntentA == nullptr)
            {
                return false;
            }

            if (quietIntentA->Action != busyIntentA->Action
                || quietIntentA->Confidence != busyIntentA->Confidence)
            {
                return false;
            }
        }

        return true;
    }
}
