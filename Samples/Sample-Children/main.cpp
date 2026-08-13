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
// │        “A child is born owing nothing — and inheriting everything.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.8.2 SDK · Sample Modules
//
// SAMPLE 7 — Children. Family as a group: birth, inherited attitudes,
// and the traits that make each child their own. Zero new surface.
//
// A family is a group; a child joins it and inherits the family's
// collective feelings — the group's mean disposition toward everyone
// it knows — while trust stays personal (never inherited). The
// child's traits are the family base varied by the seeded RNG: same
// seed, different tree. And the moment the child has their own
// experience, that personal knowledge beats what was inherited.
//
// The modder's takeaway: generational memory is not a new system —
// it is a group, a membership, an InheritGroupAttitudes call, and a
// JitteredTraits call. The bonds are relationships; the variation is
// a seed.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Mind/Relationships.h"
#include "LCE/Simulation/Society/Groups.h"
#include "LCE/Simulation/Society/Traits.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/Substrate/Rng.h"

#include <cstdio>

using namespace LCE::Simulation;

namespace
{
    // The family — one opaque group id. The core never learns its
    // meaning; the sample's narrator does.
    constexpr GroupId kFamily{ 1 };

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
    std::printf("Children — born owing nothing, inheriting everything.\n\n");

    EntityRegistry registry;

    const auto mother = registry.CreateEntity();   // minds
    const auto father = registry.CreateEntity();
    const auto child = registry.CreateEntity();    // born this sample
    const auto neighbour = registry.CreateEntity();// the family's rival

    // Both parents belong to the family group and hold feelings about
    // the neighbour — the family's collective attitude.
    registry.AddComponent<Groups>(mother, Groups{ { kFamily } });
    registry.AddComponent<Groups>(father, Groups{ { kFamily } });

    auto motherRels = registry.GetComponent<Relationships>(mother);

    if (!motherRels)
    {
        registry.AddComponent<Relationships>(mother, Relationships{});
        motherRels = registry.GetComponent<Relationships>(mother);
    }

    auto fatherRels = registry.GetComponent<Relationships>(father);

    if (!fatherRels)
    {
        registry.AddComponent<Relationships>(father, Relationships{});
        fatherRels = registry.GetComponent<Relationships>(father);
    }

    motherRels->ByEntity[neighbour].Disposition = -0.60f;
    fatherRels->ByEntity[neighbour].Disposition = -0.40f;

    std::printf("the family: the mother distrusts the neighbour (-0.60),\n");
    std::printf("the father distrusts them (-0.40).\n\n");

    // The child is born into the family and inherits its collective
    // feeling — the mean of the parents' dispositions. Trust is not
    // inherited; trust is earned personally.
    registry.AddComponent<Groups>(child, Groups{ { kFamily } });

    InheritGroupAttitudes(registry, child, kFamily);

    std::printf("the child is born into the family and inherits its "
        "feelings:\n");
    std::printf("  disposition toward the neighbour: %.2f "
        "(the family's mean)\n",
        DispositionOf(registry, child, neighbour));

    const auto childRels = registry.GetComponent<Relationships>(child);
    const float inheritedTrust = (childRels
        && childRels->ByEntity.count(neighbour) != 0)
        ? childRels->ByEntity.at(neighbour).Trust
        : 0.0f;

    std::printf("  trust toward the neighbour: %.2f — trust is earned, "
        "never inherited\n", inheritedTrust);

    // Traits: the family base, varied per child by the seeded RNG.
    // Same seed, different tree — each child their own person, but
    // recognizably of the family.
    const Traits familyBase{ {
        { "boldness", 1.0f },
        { "sociability", 1.0f },
    } };

    Rng rng{ 12345 };

    const auto childTraits = JitteredTraits(familyBase, child, &rng, 0.25f);

    std::printf("\nthe child's traits (the family base, varied):\n");

    for (const auto& trait : childTraits.List)
    {
        std::printf("  %s: %.2f\n", trait.Name.c_str(), trait.Value);
    }

    // Personal knowledge beats inherited: the child's own experience
    // with the neighbour — a kind act — moves their disposition, and
    // this time the trust is earned firsthand.
    auto childMemory = registry.GetComponent<Memory>(child);

    if (!childMemory)
    {
        registry.AddComponent<Memory>(child, Memory{});
        childMemory = registry.GetComponent<Memory>(child);
    }

    childMemory->Events.push_back(MemoryEvent{
        neighbour, InteractionKind::Aid, 1.0f });

    const auto before = DispositionOf(registry, child, neighbour);
    const SimulationTuning tuning{};

    ReportOutcome(registry, child,
        { neighbour, InteractionKind::Aid, OutcomeResult::Success },
        tuning, nullptr, WorldTime{ 1 });

    std::printf("\none kind act from the neighbour, and the child's "
        "feelings change:\n");
    std::printf("  disposition: %.2f -> %.2f (personal experience "
        "beats inherited)\n", before,
        DispositionOf(registry, child, neighbour));

    std::printf("  trust: %.2f — kindness warms, but only a fair trade "
        "earns trust\n",
        childRels->ByEntity.at(neighbour).Trust);

    // And the one thing trust is actually earned by: a trade that
    // goes through. The child's first deal with the neighbour — the
    // trust is now firsthand, not borrowed.
    childMemory->Events.push_back(MemoryEvent{
        neighbour, InteractionKind::Trade, 1.0f });

    ReportOutcome(registry, child,
        { neighbour, InteractionKind::Trade, OutcomeResult::Success },
        tuning, nullptr, WorldTime{ 2 });

    std::printf("\none fair trade, and the first trust of the child's "
        "own is born:\n");
    std::printf("  trust: %.2f\n",
        childRels->ByEntity.at(neighbour).Trust);

    std::printf("\na family is a group; a child is a seed.\n");

    return 0;
}
