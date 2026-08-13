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
// │        “A plague spreads by contact, but it ends by memory.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.3 SDK · Sample Modules
//
// SAMPLE 9 — Disease. Outbreaks as facts and ticks. Zero new surface.
//
// The adapter's 0.8.0 verdict stands as the teaching here: Health is
// adapter-owned — the engine never learns what a disease is. What the
// core DOES supply is the loop that makes an outbreak behave: a
// quarantine is a world fact with an invalid Other (the door closes
// while it is remembered), the sick mind's cost is a Fatigue need
// (the toll), and recovery is rest (hold-then-recover). The world
// holds the Health table; the core holds the memory and the need.
//
// The modder's takeaway: an epidemic is not a system — it is a door
// fact, a need, and a day counter. The world decides who is sick;
// the core makes the settlement behave like a settlement.
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

int main()
{
    std::printf("Disease — an outbreak as facts and ticks.\n\n");

    // Tuning: the quarantine fact lingers for a few days, then fades
    // and the market reopens on its own — the door is a memory.
    LCE::Config::Configuration config;
    config.Set("sim.memory.fade", "0.06");

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;

    const auto alice = registry.CreateEntity();   // minds
    const auto bob = registry.CreateEntity();
    const auto cain = registry.CreateEntity();
    const auto market = registry.CreateEntity();  // a place

    for (const auto id : { alice, bob, cain })
    {
        registry.AddComponent<Needs>(id, Needs{
            { Need{ NeedType::Hunger, 0.30f, 0.10f },
              Need{ NeedType::Fatigue, 0.90f, 0.04f } }
        });

        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        memory->Events.push_back(
            MemoryEvent{ market, InteractionKind::Trade, 4.0f });
    }

    WorldTime time{};
    int lastActionDay = -1;

    // The world's Health table — adapter-owned. Only alice ever gets
    // sick in this sample. Whether the market is open is not a flag:
    // it is read from memory — the door is shut while the quarantine
    // fact is remembered, and it reopens the moment the fact fades.
    bool aliceSick = false;

    // The door, read from bob's memory: shut while a quarantine fact
    // (invalid Other, Trade kind) is still remembered.
    const auto marketOpen = [&registry, bob]()
    {
        const auto memory = registry.GetComponent<Memory>(bob);

        for (const auto& event : memory->Events)
        {
            if (event.Kind == InteractionKind::Trade
                && !event.Other.IsValid())
            {
                return false;
            }
        }

        return true;
    };

    for (int tick = 0; tick < 168; ++tick)   // seven days
    {
        const int hour = tick % 24;
        time.Day = static_cast<std::uint64_t>(tick / 24);

        // 06:00 — the world's bookkeeping for the day.
        if (hour == 6)
        {
            // Day 2: the plague arrives. The world marks alice sick and
            // slams the door: a quarantine fact with an invalid Other.
            // While it is remembered, Decide will not send anyone to
            // market — no script ordered a halt, the door is a memory.
            if (time.Day == 2)
            {
                aliceSick = true;

                for (const auto id : { alice, bob, cain })
                {
                    registry.GetComponent<Memory>(id)->Events.push_back(
                        MemoryEvent{ EntityId{}, InteractionKind::Trade,
                                     5.0f, time.Day });
                }
            }

            // The toll: sickness takes the appetite AND holds fatigue
            // low (the fever), so rest becomes the loudest voice.
            // Hold-then-recover: while sick, the needs stay held;
            // recovery restores them toward baseline.
            auto aliceNeeds = registry.GetComponent<Needs>(alice);

            if (aliceSick)
            {
                aliceNeeds->List[0].Value = 0.90f;   // no appetite
                aliceNeeds->List[1].Value = 0.20f;   // fatigue urgent
            }
            else
            {
                aliceNeeds->List[0].Value = 0.30f;   // baseline
                aliceNeeds->List[1].Value = 0.90f;
            }

            // The healthy two stay hungry; whether they may walk depends
            // on the door.
            registry.GetComponent<Needs>(bob)->List[0].Value = 0.30f;
            registry.GetComponent<Needs>(cain)->List[0].Value = 0.30f;

            // Day 5: alice recovers — the world clears her Health and
            // stops holding the toll. The quarantine fact is still
            // remembered; the market stays shut until it fades on its
            // own.
            if (time.Day == 5)
            {
                aliceSick = false;
            }
        }

        // The tick.
        Update(registry, 1.0, tuning);

        // One line per day, from the first decision of the morning.
        if (hour != 8 || static_cast<int>(time.Day) == lastActionDay)
        {
            continue;
        }

        lastActionDay = static_cast<int>(time.Day);
        const auto day = static_cast<unsigned long long>(time.Day);

        const auto aliceIntent = registry.GetComponent<Intent>(alice);
        const auto bobIntent = registry.GetComponent<Intent>(bob);

        std::printf("day %llu: ", day);

        if (aliceSick)
        {
            std::printf("alice is sick and %s; ",
                aliceIntent && aliceIntent->Action == ActionType::Rest
                    ? "rests (recovering)" : "stirs");
        }
        else if (time.Day < 2)
        {
            std::printf("alice is well; ");
        }
        else
        {
            std::printf("alice has recovered; ");
        }

        const bool open = marketOpen();

        if (!open)
        {
            std::printf("the market is shut (the quarantine is "
                "remembered) — %s\n",
                bobIntent && bobIntent->Action == ActionType::MoveTo
                    ? "the door has reopened" : "bob cannot trade");
        }
        else
        {
            std::printf("the market is open — bob walks to trade\n");
        }
    }

    // What the settlement remembers: the quarantine fact, day-stamped,
    // faded but not forgotten. The plague is a memory now.
    std::printf("\nthe settlement remembers the outbreak:\n");

    for (const auto id : { alice, bob, cain })
    {
        const auto memory = registry.GetComponent<Memory>(id);

        for (const auto& event : memory->Events)
        {
            if (event.Kind == InteractionKind::Trade && !event.Other.IsValid())
            {
                std::printf("  the quarantine, day %llu (weight %.2f)\n",
                    static_cast<unsigned long long>(event.Day),
                    event.Weight);
            }
        }
    }

    std::printf("\nthe fever broke when the memory faded —\n");
    std::printf("no cure scripted, just a door that closes and opens.\n");

    return 0;
}
