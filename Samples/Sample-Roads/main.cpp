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
// │          “Traffic is love; a road is a memory of it.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.3 SDK · Sample Modules
//
// SAMPLE 10 — Roads. Routes that improve with traffic and degrade
// with neglect — as LegacyStore facts. Zero new engine surface.
//
// A road is a named fact in the world's books; its Weight IS its
// condition. Traffic maintains the road it travels (LeaveLegacy with
// a higher weight); weather and neglect wear every road down (the
// world's own tick). Caravans read the books and prefer the best
// maintained route — and when a storm wrecks the good road, they
// reroute, the new road improves under the traffic, and the old one
// is eventually forgotten from the world's books.
//
// The modder's takeaway: infrastructure is not a system — it is the
// world's books (LegacyStore), maintained by the same facts and
// outcomes as everything else. A road is just a memory with a lot of
// traffic.
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
    // Reads the world's books: the road that is currently best kept.
    const char* BestRoad(const EntityRegistry& registry)
    {
        const auto oldRoad = registry.ReadLegacy("old-road");
        const auto newRoad = registry.ReadLegacy("new-road");

        const float oldW = oldRoad.has_value() ? oldRoad->Weight : 0.0f;
        const float newW = newRoad.has_value() ? newRoad->Weight : 0.0f;

        return newW > oldW ? "new-road" : "old-road";
    }

    const char* RoadState(const EntityRegistry& registry, const char* name)
    {
        const auto road = registry.ReadLegacy(name);

        if (!road.has_value())
        {
            return "forgotten";
        }

        if (road->Weight >= 0.8f)
        {
            return "well kept";
        }

        if (road->Weight >= 0.4f)
        {
            return "passable";
        }

        return "degrading";
    }
}

int main()
{
    std::printf("Roads — a road is a memory with a lot of traffic.\n\n");

    LCE::Config::Configuration config;
    config.Set("sim.memory.fade", "0.10");
    config.Set("sim.drift.rate", "0.005");

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;

    // The world's books: two roads. The old road is well kept (1.0);
    // the new road is young and rough (0.5). Their weight is their
    // condition — a legacy fact, readable by anyone.
    registry.LeaveLegacy(LegacyFact{ EntityId{}, 0, "old-road", 1.0f });
    registry.LeaveLegacy(LegacyFact{ EntityId{}, 0, "new-road", 0.5f });

    const auto caravanOne = registry.CreateEntity();   // minds
    const auto caravanTwo = registry.CreateEntity();
    const auto market = registry.CreateEntity();       // a place

    for (const auto id : { caravanOne, caravanTwo })
    {
        registry.AddComponent<Needs>(id, Needs{
            { Need{ NeedType::Hunger, 0.30f, 0.10f } }
        });

        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        memory->Events.push_back(
            MemoryEvent{ market, InteractionKind::Trade, 1.0f });
    }

    WorldTime time{};
    int lastActionDay = -1;

    std::printf("day 0: old-road is well kept (1.0); "
        "new-road is rough (0.5).\n");

    for (int tick = 0; tick < 120; ++tick)   // five days
    {
        const int hour = tick % 24;
        time.Day = static_cast<std::uint64_t>(tick / 24);

        // 06:00 — the world's road works: weather wears every road
        // down; the road that was used yesterday keeps its condition.
        // The day a road was last used is its stamp — neglect is
        // measured from it, and the stamp is only moved by use.
        if (hour == 6 && time.Day != 0)
        {
            for (const char* road : { "old-road", "new-road" })
            {
                if (const auto fact = registry.ReadLegacy(road))
                {
                    // Used yesterday: kept. Otherwise: worn by 0.1.
                    // The stamp is preserved — only the caravan's use
                    // moves it, so an unused road keeps decaying. A
                    // road worn below 0.1 falls out of the world's
                    // books entirely.
                    if (fact->Day != time.Day - 1)
                    {
                        const float worn = fact->Weight - 0.1f;

                        if (worn <= 0.1f)
                        {
                            registry.ForgetLegacy(road);
                        }
                        else
                        {
                            registry.LeaveLegacy(LegacyFact{
                                EntityId{}, fact->Day, road, worn });
                        }
                    }
                }
            }
        }

        // Day 3: a storm wrecks the old road — a hard 1.0 off its
        // condition, enough to drop it below the young road. The
        // world's books change; the caravans will read it and reroute.
        // The stamp is not moved — a wrecked road that no one travels
        // keeps decaying.
        if (time.Day == 3 && hour == 6)
        {
            if (const auto old = registry.ReadLegacy("old-road"))
            {
                const float wrecked = old->Weight - 1.0f;

                if (wrecked <= 0.1f)
                {
                    registry.ForgetLegacy("old-road");
                }
                else
                {
                    registry.LeaveLegacy(LegacyFact{
                        EntityId{}, old->Day, "old-road", wrecked });
                }
            }
        }

        // 08:00 — the caravans head out. Each reads the books and takes
        // the best kept road; the journey is an outcome at the market.
        if (hour == 8 && static_cast<int>(time.Day) != lastActionDay)
        {
            lastActionDay = static_cast<int>(time.Day);
            const auto day = static_cast<unsigned long long>(time.Day);

            const auto road = BestRoad(registry);

            // The road that was travelled is maintained (traffic is its
            // upkeep) and remembers the day it was used.
            if (const auto fact = registry.ReadLegacy(road))
            {
                registry.LeaveLegacy(LegacyFact{
                    EntityId{}, time.Day, road, fact->Weight + 0.05f });
            }

            std::printf("day %llu: both caravans take the %s (%s) and "
                "trade at market\n",
                day, road,
                RoadState(registry, road));

            for (const auto id : { caravanOne, caravanTwo })
            {
                ReportOutcome(registry, id,
                    { market, InteractionKind::Trade, OutcomeResult::Success },
                    tuning, nullptr, time);

                registry.GetComponent<Needs>(id)->List[0].Value = 0.90f;
            }

            std::printf("        old-road: %s (%.2f) | new-road: %s (%.2f)\n",
                RoadState(registry, "old-road"),
                registry.ReadLegacy("old-road")
                    ? registry.ReadLegacy("old-road")->Weight : 0.0f,
                RoadState(registry, "new-road"),
                registry.ReadLegacy("new-road")
                    ? registry.ReadLegacy("new-road")->Weight : 0.0f);
        }

        Update(registry, 1.0, tuning);
    }

    // What the books say at the end: one road carried the traffic and
    // survived; the other was forgotten from the world's books.
    std::printf("\nthe books at day 5:\n");

    for (const char* road : { "old-road", "new-road" })
    {
        if (const auto fact = registry.ReadLegacy(road))
        {
            std::printf("  %s: %s (%.2f, last used day %llu)\n",
                road, RoadState(registry, road), fact->Weight,
                static_cast<unsigned long long>(fact->Day));
        }
        else
        {
            std::printf("  %s: forgotten — no one travelled it\n", road);
        }
    }

    std::printf("\nthe caravans didn't build the road;\nthey just "
        "remembered to use it.\n");

    return 0;
}
