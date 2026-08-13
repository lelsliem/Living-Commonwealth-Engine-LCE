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
// │      “What we leave behind is the only self that survives us.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.2 SDK · Sample Modules
//
// SAMPLE 5 — Legacy. Death, inheritance, and the name that outlives
// the voice. Zero new engine surface.
//
// The Farmer had one mind, the Village had many, the Market gave the
// world a voice, the Economy turned prices into memories. This sample
// shows the last of the loop: what happens when a mind ends. The
// salient facts bequeath to the heirs (fainter — a story heard is
// dimmer than a life lived); the faint ones die with their owner. A
// named legacy survives in the world's books after the body is gone.
// And a generation later, only the recent and the wanted travel on.
//
// The modder's takeaway: death is not a delete — it is three
// functions (Bequeath, InheritMemory, LeaveLegacy) and one fact
// (InteractionKind::Death). The world keeps the books; the core
// keeps the promise that what mattered can outlive what made it.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Simulation/Decision/Behaviour.h"
#include "LCE/Simulation/Decision/Legacy.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Decision/Outcome.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/Substrate/WorldTime.h"

#include <cstdio>

using namespace LCE::Simulation;

namespace
{
    // The predicate a generation uses to choose what travels: only
    // trade knowledge — the feud and the grief stay behind. It is a
    // plain function, no captures — exactly the shape InheritMemory
    // takes.
    bool OnlyTrade(const MemoryEvent& event)
    {
        return event.Kind == InteractionKind::Trade;
    }

    const char* KindName(InteractionKind kind)
    {
        switch (kind)
        {
        case InteractionKind::Trade:   return "a fair trade";
        case InteractionKind::Wronged: return "the old feud";
        case InteractionKind::Social:  return "a passing kindness";
        case InteractionKind::Death:   return "a death";
        default:                       return "a memory";
        }
    }
}

int main()
{
    std::printf("Legacy — what outlives the voice.\n\n");

    LCE::Config::Configuration config;
    config.Set("sim.legacy.maxAgeDays", "30");   // the world's patience

    const auto tuning = SimulationTuning::FromConfiguration(config);

    EntityRegistry registry;

    const auto elder = registry.CreateEntity();       // a mind, near the end
    const auto heir = registry.CreateEntity();        // their child
    const auto grandchild = registry.CreateEntity();  // a generation on
    const auto merchant = registry.CreateEntity();    // a place
    const auto raiders = registry.CreateEntity();     // a remembered danger

    // The elder's life, as three memories: a feud (salient), a fair
    // trade (salient), a passing kindness (faint — below the bequest
    // floor, so it will die with them).
    auto elderMemory = registry.GetComponent<Memory>(elder);

    if (!elderMemory)
    {
        registry.AddComponent<Memory>(elder, Memory{});
        elderMemory = registry.GetComponent<Memory>(elder);
    }

    elderMemory->Events.push_back(MemoryEvent{
        raiders, InteractionKind::Wronged, 0.9f, 5 });    // day 5
    elderMemory->Events.push_back(MemoryEvent{
        merchant, InteractionKind::Trade, 0.7f, 20 });    // day 20
    elderMemory->Events.push_back(MemoryEvent{
        merchant, InteractionKind::Social, 0.3f, 21 });   // day 21

    std::printf("the elder's life: a feud (0.9), a fair trade (0.7),\n");
    std::printf("a passing kindness (0.3).\n\n");

    // Day 40: the elder dies. Salient facts bequeath, scaled by
    // InheritanceScale (0.5) — the feud and the trade travel, fainter.
    // The kindness (0.3 < BequestFloor 0.5) dies with its owner.
    const EntityId heirs[] = { heir };

    const auto passed = Bequeath(registry, elder, heirs, tuning);

    // A named legacy survives the body: the old bridge the elder kept.
    registry.LeaveLegacy(LegacyFact{ elder, 40, "the-old-bridge", 1.0f });

    // The heir remembers the death itself — a fact, like any other.
    {
        auto heirMemory = registry.GetComponent<Memory>(heir);

        if (!heirMemory)
        {
            registry.AddComponent<Memory>(heir, Memory{});
            heirMemory = registry.GetComponent<Memory>(heir);
        }

        heirMemory->Events.push_back(MemoryEvent{
            elder, InteractionKind::Death, 1.0f, 40 });
    }

    registry.DestroyEntity(elder);

    std::printf("day 40: the elder dies. %zu facts bequeathed to the heir,\n",
        passed);
    std::printf("each fainter (x0.5) — the kindness stayed with the dead.\n");

    const auto heirMemory = registry.GetComponent<Memory>(heir);

    std::printf("the heir now remembers:\n");

    for (const auto& event : heirMemory->Events)
    {
        std::printf("  %s (weight %.2f, day %llu)\n",
            KindName(event.Kind), event.Weight,
            static_cast<unsigned long long>(event.Day));
    }

    // The world's books: the bridge fact outlived its maker — the
    // owner is a stale ID now, but the fact persists until forgotten.
    const auto bridge = registry.ReadLegacy("the-old-bridge");

    std::printf("\nthe old bridge: %s (weight %.1f, left day %llu)\n",
        bridge.has_value() ? "still remembered" : "gone",
        bridge.has_value() ? bridge->Weight : 0.0f,
        static_cast<unsigned long long>(bridge ? bridge->Day : 0));
    std::printf("its keeper is gone; the name survives.\n");

    // Day 55: the heir makes their own trade. Day 60: a generation
    // later, InheritMemory passes only what the predicate accepts and
    // only what is young enough to matter.
    heirMemory->Events.push_back(MemoryEvent{
        merchant, InteractionKind::Trade, 0.8f, 55 });

    WorldTime now{ 60 };

    const auto travelled = InheritMemory(
        registry, grandchild, heir, tuning, now, OnlyTrade);

    std::printf("\nday 60: the heir passes knowledge to the grandchild.\n");
    std::printf("only recent trade knowledge travels; the old feud\n");
    std::printf("(day 5), the inherited trade (day 20), and the death stay.\n");

    const auto grandchildMemory = registry.GetComponent<Memory>(grandchild);

    std::printf("\nthe grandchild inherits %zu fact(s):\n", travelled);

    for (const auto& event : grandchildMemory->Events)
    {
        std::printf("  %s (weight %.2f, day %llu)\n",
            KindName(event.Kind), event.Weight,
            static_cast<unsigned long long>(event.Day));
    }

    std::printf("\na story heard is fainter than a life lived —\n");
    std::printf("and the very old and the very sad stay with the dead.\n");

    return 0;
}
