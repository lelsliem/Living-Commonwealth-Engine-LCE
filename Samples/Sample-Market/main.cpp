//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │
// │                       ██╗      ██████╗███████╗
// │                       ██║     ██╔════╝██╔════╝
// │                       ██║     ██║     █████╗
// │                       ██║     ██║     ██╔══╝
// │                       ███████╗╚██████╗███████╗
// │                       ╚══════╝ ╚═════╝╚══════╝
// │
// │            Building living worlds through simulation.
// │
// │   “The market doesn't close by decree — it closes because the door is a memory that fades.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · Sample Modules
//
// SAMPLE 3 — The Market. World facts, the calendar, and weather.
//
// The Farmer had one mind. The Village had many. This sample gives
// the WORLD a voice: the market's hours are a remembered fact that
// fades open again at dawn, and the sky's weather is remembered by
// the day it happened. No scripts — doors and weather are memories,
// and memories are what the simulation is made of.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Needs.h"
#include "LCE/Simulation/Outcome.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/WorldTime.h"

#include <cstdio>

using namespace LCE::Simulation;

int main()
{
    std::printf("The Market — doors and weather are memories.\n\n");

    // Tuning: memory fades fast enough that a weight-1.0 fact dies in
    // ~11 ticks — close the door at 20:00, and it fades open by 08:00.
    LCE::Config::Configuration config;
    config.Set("sim.memory.fade", "0.08");

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;
    const auto farmer = registry.CreateEntity();
    const auto market = registry.CreateEntity();

    registry.AddComponent<Needs>(farmer, Needs{
        { Need{ NeedType::Hunger, 0.30f, 0.05f } }
    });

    // The clock: the host drives WorldTime from its own calendar. The
    // toy world also throttles: an intent is a hint, not a command —
    // the farmer walks to market at most once a day (a real adapter
    // may refuse any walk for any reason).
    WorldTime time{};
    int lastWalkDay = -1;

    for (int tick = 0; tick < 48; ++tick)   // two days
    {
        const int hour = tick % 24;
        time.Day = static_cast<std::uint64_t>(tick / 24);

        // The market's door, as a fact. Open: the farmer remembers the
        // stall. Closed (20:00, once): { invalid, Trade } shuts the
        // door while remembered; it fades on its own and the door
        // reopens — a memory died, not a script ran.
        auto memory = registry.GetComponent<Memory>(farmer);

        if (!memory)
        {
            registry.AddComponent<Memory>(farmer, Memory{});
            memory = registry.GetComponent<Memory>(farmer);
        }

        const bool open = hour >= 8 && hour < 20;

        if (open)
        {
            memory->Events.push_back(
                MemoryEvent{ market, InteractionKind::Trade, 1.0f, time.Day });
        }
        else if (hour == 20)
        {
            memory->Events.push_back(
                MemoryEvent{ EntityId{}, InteractionKind::Trade, 1.0f, time.Day });
        }

        // The sky, once a day at noon: remembered by its day. The
        // farmer's memory says "day 1 was rainy" — and tomorrow the
        // sim can reason about it (0.7.0 Legacy's substrate). Weight
        // 3.0, not 1.0: a storm is salient, so it outlives the door
        // fact — salience is exactly what Weight means.
        if (hour == 12)
        {
            const bool rainy = (time.Day % 2) == 1;   // alternate days
            memory->Events.push_back(MemoryEvent{
                EntityId{},
                rainy ? InteractionKind::WeatherRain
                      : InteractionKind::WeatherClear,
                3.0f, time.Day });
        }

        // The tick.
        Update(registry, 1.0, tuning);

        // What the farmer decided, told plainly.
        const auto intent = registry.GetComponent<Intent>(farmer);

        if (intent && intent->Action == ActionType::MoveTo
            && static_cast<int>(time.Day) != lastWalkDay)
        {
            lastWalkDay = static_cast<int>(time.Day);

            std::printf("day %llu %02d:00 — the farmer walks to the market.\n",
                static_cast<unsigned long long>(time.Day), hour);

            // The market has hours and the sky has weather: both are
            // just memories the world pushed in.
            ReportOutcome(registry, farmer,
                { market, InteractionKind::Trade, OutcomeResult::Success },
                tuning, nullptr, time);

            registry.GetComponent<Needs>(farmer)->List[0].Value = 0.90f;
        }
    }

    // The farmer's weather memories — day-stamped, still in the mind.
    const auto memory = registry.GetComponent<Memory>(farmer);

    std::printf("\nThe farmer remembers the sky:\n");

    for (const auto& event : memory->Events)
    {
        if (event.Kind == InteractionKind::WeatherRain
            || event.Kind == InteractionKind::WeatherClear)
        {
            std::printf("  day %llu: %s\n",
                static_cast<unsigned long long>(event.Day),
                event.Kind == InteractionKind::WeatherRain ? "rainy" : "clear");
        }
    }

    std::printf("\nThe seasons roll past, four of 90 days:\n");
    std::printf("  day 0: %s, day 90: %s, day 180: %s, day 270: %s\n",
        SeasonOf(0) == Season::Spring ? "spring" : "?",
        SeasonOf(90) == Season::Summer ? "summer" : "?",
        SeasonOf(180) == Season::Autumn ? "autumn" : "?",
        SeasonOf(270) == Season::Winter ? "winter" : "?");

    return 0;
}
