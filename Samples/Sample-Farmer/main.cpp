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
// │       “Every living world starts with one mind and one hunger.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · Sample Modules
//
// SAMPLE 1 — The Farmer. The smallest living loop that still lives:
//
//     one farmer, one hunger, one merchant, one decision.
//
// Everything the engine does is here in miniature — an entity with a
// need, a decision, an executed outcome, and a mind that learned
// something. Read it beside the Sample Host: the Host shows the whole
// production shape (tuning, events, save/load); this shows the atoms.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Simulation/Decision/Behaviour.h"
#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Needs.h"
#include "LCE/Simulation/Decision/Outcome.h"
#include "LCE/Simulation/Simulation.h"

#include <cstdio>

using namespace LCE::Simulation;

int main()
{
    std::printf("The Farmer — the smallest living loop.\n\n");

    // The world: one registry, two entities. The farmer is a mind
    // (has Needs); the merchant is a place (has none) — a rock has no
    // needs, and the simulation never touches it.
    EntityRegistry registry;

    const auto farmer = registry.CreateEntity();
    const auto merchant = registry.CreateEntity();

    // The farmer is hungry and knows where the merchant trades. The
    // memory is knowledge, pushed directly (see the Host's note on why
    // market knowledge is not an experience).
    registry.AddComponent<Needs>(farmer, Needs{
        { Need{ NeedType::Hunger, 0.30f, 0.10f } }
    });

    auto memory = registry.GetComponent<Memory>(farmer);

    if (!memory)
    {
        registry.AddComponent<Memory>(farmer, Memory{});
        memory = registry.GetComponent<Memory>(farmer);
    }

    memory->Events.push_back(
        MemoryEvent{ merchant, InteractionKind::Trade, 1.0f });

    // THE LOOP — three steps, no script:
    for (int tick = 0; tick < 3; ++tick)
    {
        // 1. DECIDE — the stateless decision function reads the mind.
        const auto intent = Decide(registry, farmer);

        if (!intent)
        {
            std::printf("tick %d: no decision — not hungry enough yet.\n", tick);
            return 0;
        }

        std::printf("tick %d: the farmer decides %s (confidence %.2f)\n",
            tick,
            intent->Action == ActionType::MoveTo ? "MoveTo" : "?",
            intent->Confidence);

        // 2. ACT — the game executes the intent (in a real game: walk
        //    the navmesh). Here, the meal happens instantly.
        std::printf("tick %d: the farmer walks to the merchant and trades.\n", tick);

        // 3. OBSERVE + REMEMBER — how it went is reported back; the
        //    sim records the memory, builds trust, serves the goal.
        ReportOutcome(registry, farmer,
            { merchant, InteractionKind::Trade, OutcomeResult::Success });

        // The fed write-through: the game raises the need after eating.
        registry.GetComponent<Needs>(farmer)->List[0].Value = 0.90f;
    }

    // The proof of learning: the farmer now trusts the merchant.
    const auto relationships = registry.GetComponent<Relationships>(farmer);
    const float trust = relationships
        ? relationships->ByEntity.at(merchant).Trust
        : 0.0f;

    std::printf("\nAfter %d trades, the farmer trusts the merchant %.2f.\n",
        3, trust);
    std::printf("One mind, one hunger, one lesson — that is the whole engine.\n");

    return 0;
}
