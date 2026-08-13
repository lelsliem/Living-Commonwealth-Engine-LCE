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
// │           “Everyone talks about the weather; only a mind remembers it.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.2 SDK · Sample Modules
//
// SAMPLE 6 — Weather. A sky that behaves: seasons on the calendar and
// weather as day-stamped facts. Zero new engine surface.
//
// The Market sample pushed weather facts; this sample shows what they
// DO. The core knows nothing of rain or radstorms — the weather kinds
// are just labels the world chose. The world reads the remembered
// sky and translates it into need: a clear day leaves hunger the
// loudest voice, so the farmer walks to market; a radstorm makes
// safety urgent, and the farmer — who remembers the raider camp —
// flees. The sky never tells the mind what to do; it changes which
// need is urgent, and the decision follows.
//
// The modder's takeaway: weather is not a system, it is a fact and a
// calendar. Push the day's sky as a memory, let the season name
// itself, and let needs do the deciding.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Decision/Behaviour.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Decision/Outcome.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/Substrate/WorldTime.h"

#include <cstdio>

using namespace LCE::Simulation;

namespace
{
    const char* SeasonName(Season season)
    {
        switch (season)
        {
        case Season::Spring: return "spring";
        case Season::Summer: return "summer";
        case Season::Autumn: return "autumn";
        case Season::Winter: return "winter";
        }

        return "?";
    }

    // The world's weather rule — the core never sees it. A radstorm
    // every fourth day; a rainy day two days after each storm; clear
    // otherwise. Pure world knowledge, like a market's hours.
    InteractionKind SkyOf(std::uint64_t day)
    {
        if (day % 4 == 0)
        {
            return InteractionKind::WeatherRadstorm;
        }

        if (day % 4 == 2)
        {
            return InteractionKind::WeatherRain;
        }

        return InteractionKind::WeatherClear;
    }
}

int main()
{
    std::printf("Weather — a sky that behaves.\n\n");

    LCE::Config::Configuration config;
    config.Set("sim.memory.fade", "0.02");

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;

    const auto farmer = registry.CreateEntity();   // a mind
    const auto market = registry.CreateEntity();   // a place
    const auto raiders = registry.CreateEntity();  // a remembered danger

    // The farmer is hungry and safe, knows the market, and remembers
    // the raiders (a Combat fact — the threat FindThreat reads when
    // safety turns urgent). Knowledge is weight 4.0, not 1.0: durable
    // facts that survive the week's fade.
    registry.AddComponent<Needs>(farmer, Needs{
        { Need{ NeedType::Hunger, 0.40f, 0.10f },
          Need{ NeedType::Safety, 0.90f, 0.05f } }
    });

    auto farmerMemory = registry.GetComponent<Memory>(farmer);

    if (!farmerMemory)
    {
        registry.AddComponent<Memory>(farmer, Memory{});
        farmerMemory = registry.GetComponent<Memory>(farmer);
    }

    farmerMemory->Events.push_back(
        MemoryEvent{ market, InteractionKind::Trade, 4.0f });
    farmerMemory->Events.push_back(
        MemoryEvent{ raiders, InteractionKind::Combat, 4.0f });

    WorldTime time{};
    int lastActionDay = -1;

    // Six days, starting at day 88 so the year crosses spring into
    // summer mid-run — the seasons are derived from the day alone.
    for (int tick = 0; tick < 144; ++tick)
    {
        const int hour = tick % 24;
        time.Day = static_cast<std::uint64_t>(88 + tick / 24);

        // 06:00 — the sky speaks, as a fact: the day's weather, stamped
        // with the day it happened.
        if (hour == 6)
        {
            farmerMemory->Events.push_back(MemoryEvent{
                EntityId{}, SkyOf(time.Day), 2.0f, time.Day });

            // The world reads the sky and translates it to need — the
            // adapter's job, never the core's. A radstorm makes safety
            // the loudest voice; any other sky leaves hunger the one
            // that speaks. The core only ever sees the need, never the
            // sky. Both needs are set: the urgent one low, the other
            // sated — the world holds the reins each morning.
            auto needs = registry.GetComponent<Needs>(farmer);

            if (SkyOf(time.Day) == InteractionKind::WeatherRadstorm)
            {
                needs->List[1].Value = 0.20f;   // fear: safety urgent
                needs->List[0].Value = 0.90f;   // hunger sated
            }
            else
            {
                needs->List[0].Value = 0.30f;   // a day's work: hunger
                needs->List[1].Value = 0.90f;   // and all is safe
            }
        }

        // The tick: needs decay, the weather fact fades, the farmer
        // decides — the core reasons over facts it never named.
        Update(registry, 1.0, tuning);

        const auto intent = registry.GetComponent<Intent>(farmer);

        if (!intent || hour < 8
            || static_cast<int>(time.Day) == lastActionDay)
        {
            continue;
        }

        if (intent->Action == ActionType::Flee)
        {
            lastActionDay = static_cast<int>(time.Day);

            std::printf("day %llu %02d:00 (%s) — a radstorm; "
                "the farmer flees the raiders.\n",
                static_cast<unsigned long long>(time.Day), hour,
                SeasonName(SeasonOf(time.Day)));

            registry.GetComponent<Needs>(farmer)->List[1].Value = 0.90f;
        }
        else if (intent->Action == ActionType::MoveTo)
        {
            lastActionDay = static_cast<int>(time.Day);

            std::printf("day %llu %02d:00 (%s) — the farmer walks to "
                "market and trades.\n",
                static_cast<unsigned long long>(time.Day), hour,
                SeasonName(SeasonOf(time.Day)));

            ReportOutcome(registry, farmer,
                { market, InteractionKind::Trade, OutcomeResult::Success },
                tuning, nullptr, time);

            registry.GetComponent<Needs>(farmer)->List[0].Value = 0.90f;
        }
    }

    // The sky, as the farmer remembers it — day-stamped, fading.
    std::printf("\nthe farmer remembers the sky:\n");

    for (const auto& event : farmerMemory->Events)
    {
        if (event.Kind >= InteractionKind::WeatherClear
            && event.Kind <= InteractionKind::WeatherRadstorm)
        {
            std::printf("  day %llu: %s (weight %.2f)\n",
                static_cast<unsigned long long>(event.Day),
                event.Kind == InteractionKind::WeatherRadstorm
                    ? "radstorm"
                    : event.Kind == InteractionKind::WeatherRain
                        ? "rain" : "clear",
                event.Weight);
        }
    }

    std::printf("\nthe year turns, four seasons of 90 days, derived from "
        "the day:\n");
    std::printf("  day 88: %s, day 90: %s\n",
        SeasonName(SeasonOf(88)), SeasonName(SeasonOf(90)));

    return 0;
}
