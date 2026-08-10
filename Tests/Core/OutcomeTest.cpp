//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      OutcomeTest.cpp
//
// Purpose:
//
//      Verifies ReportOutcome — the observe leg of the living loop
//      (0.5.0). Outcomes record memory, scale relationships by result,
//      serve or frustrate the active goal, and consume the intent so the
//      next tick decides fresh. The money test: a settler who is cheated
//      twice learns to trade elsewhere.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool OutcomeTest()
    {
        //-------------------------------------------------------------------------
        // 1. A successful trade: memory recorded, trust gained, the
        //    AcquireFood goal served, the intent consumed.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Goals>(
                farmer,
                Simulation::Goals{
                    Simulation::Goal{ Simulation::GoalType::AcquireFood, 0.5f }
                });

            // The executor acted on the intent; here is how it went.
            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Success });

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);
            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(farmer);
            const auto goals = registry.GetComponent<Simulation::Goals>(farmer);
            const auto intent = registry.GetComponent<Simulation::Intent>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            if (memory->Events.back().Other != merchant
                || memory->Events.back().Kind != Simulation::InteractionKind::Trade)
            {
                return false;
            }

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(merchant);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Trust < 0.149f)   // 0.0 + 0.15
            {
                return false;
            }

            // The ambition is served: the goal is gone.
            if (goals->Active)
            {
                return false;
            }

            // The action concluded: the intent is consumed.
            if (intent)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. A failed trade: the merchant proved unreliable — trust is
        //    LOST, the goal survives to keep growing.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Goals>(
                farmer,
                Simulation::Goals{
                    Simulation::Goal{ Simulation::GoalType::AcquireFood, 0.3f }
                });

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Failure });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(farmer);
            const auto goals = registry.GetComponent<Simulation::Goals>(farmer);

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(merchant);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Trust > -0.149f)   // 0.0 - 0.15
            {
                return false;
            }

            // Unserved ambition: the goal persists.
            if (!goals->Active
                || goals->Active->Type != Simulation::GoalType::AcquireFood)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. A partial trade: half the trust, and the goal's urgency is
        //    halved, not cleared.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Goals>(
                farmer,
                Simulation::Goals{
                    Simulation::Goal{ Simulation::GoalType::AcquireFood, 0.8f }
                });

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Partial });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(farmer);
            const auto goals = registry.GetComponent<Simulation::Goals>(farmer);

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(merchant);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Trust < 0.074f   // 0.0 + 0.15 * 0.5
                || iterator->second.Trust > 0.076f)
            {
                return false;
            }

            if (!goals->Active
                || goals->Active->Urgency < 0.39f   // 0.8 * 0.5
                || goals->Active->Urgency > 0.41f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. A world outcome (invalid Other): memory only — no
        //    relationship is shaped, no goal is served.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();

            registry.AddComponent<Simulation::Goals>(
                farmer,
                Simulation::Goals{
                    Simulation::Goal{ Simulation::GoalType::AcquireFood, 0.4f }
                });

            Simulation::ReportOutcome(
                registry, farmer,
                { Simulation::EntityId{}, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Failure, 0.8f });

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);
            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(farmer);
            const auto goals = registry.GetComponent<Simulation::Goals>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            // "The road was blocked" — remembered, but nobody to blame.
            if (relationships)
            {
                return false;
            }

            if (!goals->Active)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. A wrong is a wrong: full disposition loss whatever the
        //    result claims.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto raider = registry.CreateEntity();

            Simulation::ReportOutcome(
                registry, farmer,
                { raider, Simulation::InteractionKind::Wronged,
                  Simulation::OutcomeResult::Success });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(farmer);

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(raider);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Disposition > -0.249f)   // full loss
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 6. The money test — the learning settler. The farmer trades
        //    well with merchant A and prefers A; after being cheated
        //    twice, the farmer learns to trade with B instead. No script:
        //    the decision function reads the outcomes' memory.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchantA = registry.CreateEntity();
            const auto merchantB = registry.CreateEntity();

            // The farmer is a mind: hungry, so the decision function
            // acts on the outcomes' memory.
            registry.AddComponent<Simulation::Needs>(
                farmer,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
                });

            // The farmer knows both stalls exist.
            Simulation::Remember(
                registry, farmer,
                { merchantA, Simulation::InteractionKind::Trade, 1.0f });
            Simulation::Remember(
                registry, farmer,
                { merchantB, Simulation::InteractionKind::Trade, 1.0f });

            // A good first trade: trust A.
            Simulation::ReportOutcome(
                registry, farmer,
                { merchantA, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Success });

            auto intent = Simulation::Decide(registry, farmer);

            if (!intent || intent->Target != merchantA)
            {
                return false;   // A proved reliable; the farmer prefers A
            }

            // Cheated once... then twice. Trust in A collapses below B's.
            Simulation::ReportOutcome(
                registry, farmer,
                { merchantA, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Failure });
            Simulation::ReportOutcome(
                registry, farmer,
                { merchantA, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Failure });

            intent = Simulation::Decide(registry, farmer);

            if (!intent || intent->Target != merchantB)
            {
                return false;   // A is unreliable; the farmer learned
            }
        }

        return true;
    }
}
