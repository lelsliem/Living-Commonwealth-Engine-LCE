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
// │    “A village is a web of small feelings, each one pulling a thread.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · Sample Modules
//
// SAMPLE 2 — The Village. Many minds, and the feelings between them.
//
// The Farmer had one mind and one merchant. A village is dozens of
// minds who know each other: each Socialize outcome warms the bond,
// each wrong sours it — and the NEXT decision reads those feelings.
// The pattern shown here is the seed of Faction Wars and Children of
// the Commonwealth: relationships are shaped by outcomes, and choices
// follow relationships.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Needs.h"
#include "LCE/Simulation/Outcome.h"
#include "LCE/Simulation/Simulation.h"

#include <array>
#include <cstdio>
#include <string>

using namespace LCE::Simulation;

namespace
{
    // The village: five neighbours. Names are game knowledge — the core
    // never sees them; the sample's narrator does.
    constexpr std::array<const char*, 5> kNames{
        "Abigail", "Bram", "Cora", "Dorian", "Elara"
    };

    // A mind with a Social need.
    void SeedMind(EntityRegistry& registry, EntityId id)
    {
        registry.AddComponent<Needs>(id, Needs{
            { Need{ NeedType::Social, 0.50f, 0.02f } }
        });
    }

    // Knowledge, phase two: each villager knows their two nearest
    // neighbours, one step around the circle (Abigail knows Bram and
    // Cora, Bram knows Cora and Dorian, ...). Raw pushes — the same
    // rule as the Host's market facts. Runs after every mind exists,
    // so the first villager knows someone too.
    void SeedKnowledge(
        EntityRegistry& registry,
        const std::array<EntityId, 5>& villagers,
        std::size_t index)
    {
        auto memory = registry.GetComponent<Memory>(villagers[index]);

        if (!memory)
        {
            registry.AddComponent<Memory>(villagers[index], Memory{});
            memory = registry.GetComponent<Memory>(villagers[index]);
        }

        // Cora (2) knows Bram (1) — so the wrong can land where the
        // next decision will read it — and Dorian (3).
        const std::size_t first =
            index == 2 ? 1 : (index + 1) % villagers.size();
        const std::size_t second =
            index == 2 ? 3 : (index + 2) % villagers.size();

        memory->Events.push_back(MemoryEvent{
            villagers[first], InteractionKind::Social, 1.0f });
        memory->Events.push_back(MemoryEvent{
            villagers[second], InteractionKind::Social, 1.0f });
    }

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
    std::printf("The Village — five minds, and the feelings between them.\n\n");

    EntityRegistry registry;

    // Five villagers; each knows two neighbours (Abigail↔Bram↔Cora…).
    std::array<EntityId, 5> villagers{};

    for (std::size_t i = 0; i < villagers.size(); ++i)
    {
        villagers[i] = registry.CreateEntity();
    }

    for (const auto id : villagers)
    {
        SeedMind(registry, id);
    }

    for (std::size_t i = 0; i < villagers.size(); ++i)
    {
        SeedKnowledge(registry, villagers, i);
    }

    // One wrong, once, to show how feelings sour: Bram takes Cora's
    // grain. The sim records it; Cora's NEXT decision reads it.
    std::printf("— Bram takes Cora's grain. No quest fired. —\n");
    ReportOutcome(registry, villagers[2],
        { villagers[1], InteractionKind::Wronged, OutcomeResult::Success });

    std::printf("  Cora's disposition toward Bram: %.2f (it was 0.00)\n",
        DispositionOf(registry, villagers[2], villagers[1]));

    // The village lives: each tick, everyone with a Social need decides,
    // socializes with their preferred neighbour, and the outcome warms
    // the bond. Cora, who knows who wronged her, socializes elsewhere.
    std::printf("\n— The village lives —\n");

    for (int tick = 0; tick < 4; ++tick)
    {
        for (const auto id : villagers)
        {
            const auto intent = Decide(registry, id);

            if (!intent || intent->Action != ActionType::Socialize)
            {
                continue;
            }

            const auto idIndex = static_cast<std::size_t>(
                id.Value() - villagers[0].Value());
            const auto targetIndex = static_cast<std::size_t>(
                intent->Target.Value() - villagers[0].Value());

            std::printf("tick %d: %s chats with %s\n",
                tick, kNames[idIndex], kNames[targetIndex]);

            ReportOutcome(registry, id,
                { intent->Target, InteractionKind::Social,
                  OutcomeResult::Success });

            // The need is met: the game raises it after the visit.
            registry.GetComponent<Needs>(id)->List[0].Value = 1.0f;
        }
    }

    // The web, printed: every bond that formed or soured.
    std::printf("\n— The web of feelings —\n");

    for (std::size_t i = 0; i < villagers.size(); ++i)
    {
        for (std::size_t j = 0; j < villagers.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            const float disposition =
                DispositionOf(registry, villagers[i], villagers[j]);

            if (disposition != 0.0f)
            {
                std::printf("  %s -> %s: %+.2f\n",
                    kNames[i], kNames[j], disposition);
            }
        }
    }

    std::printf("\nNo script wrote a single bond. Each one is an outcome\n");
    std::printf("that became a feeling that steered the next choice.\n");

    return 0;
}
