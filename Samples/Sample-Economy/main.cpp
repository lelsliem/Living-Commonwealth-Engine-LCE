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
// │          “Prices are memories with numbers on them.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.2 SDK · Sample Modules
//
// SAMPLE 4 — The Economy. Dynamic pricing, supply chains, trade
// routes, and market events — zero new engine surface.
//
// The Farmer had one mind. The Village had many. The Market gave the
// world a voice. This sample shows that a whole ECONOMY needs no new
// systems either: the price of bread is what the market remembers
// about last harvest. No ledger, no money, no price field — the price
// is a pure function of remembered facts. A delivery pushes it down,
// a blight pushes it up, and when the memory fades the price fades
// back. The route to market is trust, remembered. The supply chain is
// a chain of memories.
//
// The modder's takeaway: if you can push a fact and report an
// outcome, you can build a living economy — the substrate does the
// fading, the trusting, and the deciding.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Decision/Behaviour.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Mind/Relationships.h"
#include "LCE/Simulation/Decision/Outcome.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/Substrate/WorldTime.h"

#include <cstdio>

using namespace LCE::Simulation;

namespace
{
    // One number from the market's own memory — there is no price
    // field anywhere in this program. Remembered deliveries are
    // supply (cheaper); remembered blights are scarcity (dearer);
    // what fades, stops counting.
    int PriceOf(const Memory& book)
    {
        float supply = 0.0f;
        float scarcity = 0.0f;

        for (const auto& event : book.Events)
        {
            if (event.Kind != InteractionKind::Trade)
            {
                continue;
            }

            if (event.Other.IsValid())
            {
                supply += event.Weight;      // the farm delivered
            }
            else
            {
                scarcity += event.Weight;    // the sky failed
            }
        }

        const float price = 10.0f + scarcity - supply;

        if (price < 3.0f)
        {
            return 3;
        }

        if (price > 20.0f)
        {
            return 20;
        }

        return static_cast<int>(price);
    }
}

int main()
{
    std::printf("The Economy — prices are memories.\n\n");

    // Tuning: facts fade slowly enough that a delivery shows for a
    // couple of days and a blight lingers for the better part of a
    // week — the market remembers, and what it remembers is the price.
    LCE::Config::Configuration config;
    config.Set("sim.memory.fade", "0.02");
    config.Set("sim.drift.rate", "0.005");   // trust outlives the week

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;

    const auto farm = registry.CreateEntity();      // a place
    const auto market = registry.CreateEntity();    // a place
    const auto farmer = registry.CreateEntity();    // a mind

    // The market keeps its own memory — the price book. A place can
    // hold a Memory; only minds (Needs) decide.
    registry.AddComponent<Memory>(market, Memory{});

    // The farmer is hungry, knows where the market is, and keeps the
    // feelings that fair trade builds.
    registry.AddComponent<Needs>(farmer, Needs{
        { Need{ NeedType::Hunger, 0.30f, 0.10f } }
    });

    registry.AddComponent<Relationships>(farmer, Relationships{});

    auto farmerMemory = registry.GetComponent<Memory>(farmer);

    if (!farmerMemory)
    {
        registry.AddComponent<Memory>(farmer, Memory{});
        farmerMemory = registry.GetComponent<Memory>(farmer);
    }

    farmerMemory->Events.push_back(
        MemoryEvent{ market, InteractionKind::Trade, 1.0f });

    auto marketBook = registry.GetComponent<Memory>(market);

    WorldTime time{};
    int lastWalkDay = -1;
    int trades = 0;

    for (int tick = 0; tick < 144; ++tick)   // six days
    {
        const int hour = tick % 24;
        time.Day = static_cast<std::uint64_t>(tick / 24);

        // 06:00 — the farm brings its crop. Supply is a memory the
        // market keeps; every delivery pushes the price down a little.
        if (hour == 6)
        {
            marketBook->Events.push_back(MemoryEvent{
                farm, InteractionKind::Trade, 1.0f, time.Day });
        }

        // Day 1, 10:00 — a blight strikes the fields. Scarcity is a
        // world fact with an invalid Other: no one to blame, everyone
        // to remember. While remembered it raises the price; when it
        // fades the price falls back — no script set a single number.
        // Weight 6.0: a blight is salient, so it outlives the daily
        // deliveries (weight 1.0) — salience is exactly what Weight
        // means.
        if (time.Day == 1 && hour == 10)
        {
            marketBook->Events.push_back(MemoryEvent{
                EntityId{}, InteractionKind::Trade, 6.0f, time.Day });
        }

        // The tick: needs decay, memory fades, relationships drift,
        // and the farmer decides — the price book fades too, because
        // Update sweeps every Memory, mind or place.
        Update(registry, 1.0, tuning);

        // 12:00 — the market prices the day from what it remembers.
        if (hour == 12)
        {
            std::printf("day %llu %02d:00 — price %d caps\n",
                static_cast<unsigned long long>(time.Day), hour,
                PriceOf(*marketBook));
        }

        // The farmer, hungry, walks the remembered route and trades.
        // The intent fires at midnight; the walk happens when the
        // market opens — the mind decides, the world executes.
        const auto intent = registry.GetComponent<Intent>(farmer);

        if (intent && intent->Action == ActionType::MoveTo
            && hour >= 8
            && static_cast<int>(time.Day) != lastWalkDay)
        {
            lastWalkDay = static_cast<int>(time.Day);
            ++trades;

            std::printf("day %llu %02d:00 — the farmer walks to market "
                "and trades (%d caps)\n",
                static_cast<unsigned long long>(time.Day), hour,
                PriceOf(*marketBook));

            // A fair trade: the outcome becomes memory, the trust
            // grows, and the goal is served — one call, the whole
            // observe leg.
            ReportOutcome(registry, farmer,
                { market, InteractionKind::Trade, OutcomeResult::Success },
                tuning, nullptr, time);

            registry.GetComponent<Needs>(farmer)->List[0].Value = 0.90f;
        }
    }

    // The route is trust, remembered: each fair trade built Trust
    // toward the market (Trade outcomes move Trust, not Disposition).
    const auto relationships = registry.GetComponent<Relationships>(farmer);
    const float trust = (relationships
        && relationships->ByEntity.count(market) != 0)
        ? relationships->ByEntity.at(market).Trust
        : 0.0f;

    std::printf("\n%d fair trades later, the farmer's trust in the "
        "market is %.2f —\n", trades, trust);
    std::printf("the route exists because it is remembered.\n");

    // QueryWhere: who remembers the market at all? Ask the world,
    // don't keep a list — a modder's filtered read, one line.
    const auto regulars = registry.QueryWhere<Memory>(
        [market](EntityId, const Memory& memory)
        {
            for (const auto& event : memory.Events)
            {
                if (event.Kind == InteractionKind::Trade
                    && event.Other == market)
                {
                    return true;
                }
            }

            return false;
        });

    std::printf("\nwho remembers the market: %zu mind(s)\n",
        regulars.size());
    std::printf("the farm doesn't — it only delivers. That's the "
        "supply chain: a chain of memories.\n");

    return 0;
}
