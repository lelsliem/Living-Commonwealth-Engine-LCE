//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      TuningTest.cpp
//
// Purpose:
//
//      Verifies SimulationTuning::FromConfiguration — the modder's knob
//      (0.5.0). Known keys override defaults; missing, empty, or
//      unparsable values keep the default; unknown keys are ignored so
//      the adapter may carry its own keys in the same file.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool TuningTest()
    {
        //-------------------------------------------------------------------------
        // 1. Empty configuration → every default, untouched.
        //-------------------------------------------------------------------------
        {
            Config::Configuration empty;

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(empty);

            if (tuning.MemoryFadeRate != 0.2f ||
                tuning.ForgetThreshold != 0.1f ||
                tuning.DriftRate != 0.05f ||
                tuning.GoalUrgencyRate != 0.1f ||
                tuning.TrustGain != 0.15f ||
                tuning.DispositionGain != 0.1f ||
                tuning.DispositionLoss != 0.25f ||
                tuning.HungerDesperate != 0.0f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. Known keys override the defaults.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.memory.fade", "0.05");
            config.Set("sim.memory.forget", "0.05");
            config.Set("sim.drift.rate", "0.01");
            config.Set("sim.goal.urgency", "0.2");
            config.Set("sim.trust.gain", "0.5");
            config.Set("sim.disposition.gain", "0.3");
            config.Set("sim.disposition.loss", "0.1");
            config.Set("sim.hunger.desperate", "0.2");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            if (tuning.MemoryFadeRate != 0.05f ||
                tuning.ForgetThreshold != 0.05f ||
                tuning.DriftRate != 0.01f ||
                tuning.GoalUrgencyRate != 0.2f ||
                tuning.TrustGain != 0.5f ||
                tuning.DispositionGain != 0.3f ||
                tuning.DispositionLoss != 0.1f ||
                tuning.HungerDesperate != 0.2f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. A broken value keeps the default — a bad line must never
        //    break the world.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.memory.fade", "not-a-number");
            config.Set("sim.memory.forget", "");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            if (tuning.MemoryFadeRate != 0.2f || tuning.ForgetThreshold != 0.1f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. Unknown keys are ignored — the adapter may share one file
        //    with its own keys.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("market.open.hour", "9");
            config.Set("mod.quote", "the world is awake");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            if (tuning.MemoryFadeRate != 0.2f || tuning.TrustGain != 0.15f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. The factory output actually reaches the simulation — a
        //    configured slow fade keeps a memory alive past the default.
        //-------------------------------------------------------------------------
        {
            Config::Configuration config;

            config.Set("sim.memory.fade", "0.05");
            config.Set("sim.memory.forget", "0.05");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            Simulation::EntityRegistry registry;

            const auto trader = registry.CreateEntity();
            const auto farmer = registry.CreateEntity();

            Simulation::Remember(
                registry, farmer, { trader, Simulation::InteractionKind::Trade, 1.0f });

            Simulation::Update(registry, 1.0, tuning);

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            // The default fade would leave 0.80; the configured slow fade
            // leaves 0.95. Configuration reached into the simulation.
            if (memory->Events[0].Weight < 0.90f)
            {
                return false;
            }
        }

        return true;
    }
}
