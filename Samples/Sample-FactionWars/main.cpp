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
// │        “Wars are fought over ground, but won over dispositions.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.3 SDK · Sample Modules
//
// SAMPLE 8 — Faction Wars. Territory, sieges, and diplomacy as groups
// and dispositions. Zero new engine surface.
//
// A faction is a group; loyalty is a disposition. The world engineers
// the events — a wrong from a comrade, a kindness from an enemy —
// and the mind does the rest: its feelings drift, and when the enemy
// starts to outrank the ally, the world reads the crossing and the
// mind changes sides. Membership flips, and the newcomer inherits the
// new group's collective attitudes (InheritGroupAttitudes) — the
// faction's grudges become their own.
//
// The modder's takeaway: war and peace need no systems. Groups are
// the map, dispositions are the loyalty meter, outcomes are the
// diplomacy, and InheritGroupAttitudes is the indoctrination. The
// engine never knows what a faction is.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Simulation/Decision/Behaviour.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Mind/Relationships.h"
#include "LCE/Simulation/Decision/Outcome.h"
#include "LCE/Simulation/Society/Groups.h"
#include "LCE/Simulation/Simulation.h"

#include <cstdio>

using namespace LCE::Simulation;

namespace
{
    constexpr GroupId kFactionA{ 1 };
    constexpr GroupId kFactionB{ 2 };

    float DispositionOf(
        const EntityRegistry& registry, EntityId id, EntityId other)
    {
        const auto relationships = registry.GetComponent<Relationships>(id);

        if (!relationships)
        {
            return 0.0f;
        }

        const auto iterator = relationships->ByEntity.find(other);

        return iterator == relationships->ByEntity.end()
            ? 0.0f
            : iterator->second.Disposition;
    }
}

int main()
{
    std::printf("Faction Wars — won over dispositions.\n\n");

    EntityRegistry registry;

    const auto leaderA = registry.CreateEntity();   // faction A
    const auto soldierA = registry.CreateEntity();
    const auto mara = registry.CreateEntity();      // the one who wavers
    const auto leaderB = registry.CreateEntity();   // faction B
    const auto soldierB = registry.CreateEntity();

    // Everyone is a mind (Social need), everyone knows their own side,
    // and mara knows the enemy's diplomat too — that is the seam the
    // war is fought over.
    for (const auto id : { leaderA, soldierA, mara })
    {
        registry.AddComponent<Needs>(id, Needs{
            { Need{ NeedType::Social, 0.50f, 0.02f } } });
        registry.AddComponent<Groups>(id, Groups{ { kFactionA } });
    }

    for (const auto id : { leaderB, soldierB })
    {
        registry.AddComponent<Needs>(id, Needs{
            { Need{ NeedType::Social, 0.50f, 0.02f } } });
        registry.AddComponent<Groups>(id, Groups{ { kFactionB } });
    }

    // Knowledge: faction-mates know each other; mara also knows the
    // enemy diplomat. The rival leaders know of each other (the
    // grudges InheritGroupAttitudes will spread).
    auto seed = [&registry](EntityId id, EntityId other)
    {
        auto memory = registry.GetComponent<Memory>(id);

        if (!memory)
        {
            registry.AddComponent<Memory>(id, Memory{});
            memory = registry.GetComponent<Memory>(id);
        }

        memory->Events.push_back(
            MemoryEvent{ other, InteractionKind::Social, 1.0f });
    };

    seed(leaderA, soldierA);
    seed(soldierA, leaderA);
    seed(leaderA, mara);
    seed(mara, leaderA);
    seed(soldierA, mara);
    seed(mara, soldierA);
    seed(leaderB, soldierB);
    seed(soldierB, leaderB);
    seed(mara, leaderB);          // the seam
    seed(leaderA, leaderB);       // the rivalry
    seed(leaderB, leaderA);

    std::printf("mara of faction A knows both sides — the seam.\n");

    // The world engineers two events (the diplomacy). The outcomes do
    // the rest: mara's feelings move, and the mind decides.
    std::printf("day 1: soldier A wrongs mara; diplomat B aids her.\n");

    ReportOutcome(registry, mara,
        { soldierA, InteractionKind::Wronged, OutcomeResult::Success },
        {}, nullptr, WorldTime{ 1 });
    ReportOutcome(registry, mara,
        { leaderB, InteractionKind::Aid, OutcomeResult::Success },
        {}, nullptr, WorldTime{ 1 });

    std::printf("  toward soldier A: %.2f, toward diplomat B: %.2f\n",
        DispositionOf(registry, mara, soldierA),
        DispositionOf(registry, mara, leaderB));

    // A second week of the same two events. The wrongs and kindnesses
    // accumulate; dispositions drift, never set.
    ReportOutcome(registry, mara,
        { soldierA, InteractionKind::Wronged, OutcomeResult::Success },
        {}, nullptr, WorldTime{ 8 });
    ReportOutcome(registry, mara,
        { leaderB, InteractionKind::Aid, OutcomeResult::Success },
        {}, nullptr, WorldTime{ 8 });

    std::printf("day 8: the same two events again.\n");
    std::printf("  toward soldier A: %.2f, toward diplomat B: %.2f\n",
        DispositionOf(registry, mara, soldierA),
        DispositionOf(registry, mara, leaderB));

    // The crossing: the enemy's diplomat now outranks the ally. The
    // world reads it (a bond threshold the world defined) and mara
    // changes sides — the sample's version of the adapter reading
    // RelationshipChangedEvent.
    std::printf("\nthe diplomat outranks the ally — mara changes sides.\n");

    auto maraGroups = registry.GetComponent<Groups>(mara);
    maraGroups->Memberships = { kFactionB };

    // The newcomer inherits the faction's collective attitudes: the
    // rivalry toward faction A's leader is now mara's own, without a
    // single lived wrong.
    InheritGroupAttitudes(registry, mara, kFactionB);

    std::printf("  mara now feels toward faction A's leader: %.2f\n",
        DispositionOf(registry, mara, leaderA));
    std::printf("  (inherited from the faction's collective memory —\n");
    std::printf("   trust stays 0; trust is earned personally)\n");

    // And the decision follows the feelings: with a Social need, mara
    // now chooses the diplomat over the old ally — ChooseTarget scores
    // by memory weight plus trust and disposition.
    const auto intent = Decide(registry, mara);

    std::printf("\nmara's Social need decides toward:\n");

    if (intent && intent->Action == ActionType::Socialize)
    {
        std::printf("  %s (the enemy diplomat, now the trusted friend)\n",
            intent->Target == leaderB ? "diplomat B" : "someone else");
    }

    std::printf("\nno siege engine in sight — just a disposition that\n");
    std::printf("crossed a line the world drew.\n");

    return 0;
}
