//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      RngTest.cpp
//
// Purpose:
//
//      Verifies the seeded generator (0.5.0): the same seed reproduces
//      the same world; a captured state resumes the exact stream; Derive
//      gives order-independent per-entity noise; and a tick under a seed
//      is deterministic — the substrate save/load determinism stands on.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <cstdint>

namespace LCE::Tests
{
    bool RngTest()
    {
        //-------------------------------------------------------------------------
        // 1. Same seed → identical stream; the generator is deterministic.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng a{ 42 };
            Simulation::Rng b{ 42 };

            for (int i = 0; i < 64; ++i)
            {
                if (a.Next() != b.Next())
                {
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------
        // 2. Different seeds → different streams.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng a{ 42 };
            Simulation::Rng b{ 43 };

            bool anyDifferent = false;

            for (int i = 0; i < 64 && !anyDifferent; ++i)
            {
                anyDifferent = (a.Next() != b.Next());
            }

            if (!anyDifferent)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. Capture/restore → the stream resumes exactly. This is the
        //    co-save contract: one number in the save resumes the world's
        //    randomness.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng rng{ 7 };

            for (int i = 0; i < 10; ++i)
            {
                rng.Next();
            }

            const auto saved = rng.State();

            std::uint64_t afterSave[8];
            for (auto& draw : afterSave)
            {
                draw = rng.Next();
            }

            // A fresh generator restored from the saved state continues
            // the identical sequence.
            Simulation::Rng restored{ 999 };   // seed irrelevant after restore
            restored.SetState(saved);

            for (const auto expected : afterSave)
            {
                if (restored.Next() != expected)
                {
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------
        // 4. Float draws stay in their documented ranges.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng rng{ 12345 };

            for (int i = 0; i < 1024; ++i)
            {
                const auto unit = rng.NextFloat();

                if (unit < 0.0f || unit >= 1.0f)
                {
                    return false;
                }

                const auto ranged = rng.NextFloat(-0.05f, 0.05f);

                if (ranged < -0.05f || ranged >= 0.05f)
                {
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------
        // 5. Derive is order-independent: same (state, key) → same child,
        //    different key → different child, and the parent never moves.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng parent{ 99 };

            const auto before = parent.State();

            auto childA = parent.Derive(1000);
            auto childA2 = parent.Derive(1000);
            auto childB = parent.Derive(2000);

            // Same key, same state → identical child streams.
            for (int i = 0; i < 16; ++i)
            {
                if (childA.Next() != childA2.Next())
                {
                    return false;
                }
            }

            // Different key → different child.
            bool anyDifferent = false;

            for (int i = 0; i < 16 && !anyDifferent; ++i)
            {
                anyDifferent = (childA.Next() != childB.Next());
            }

            if (!anyDifferent)
            {
                return false;
            }

            // The parent was not advanced — deriving never consumes it.
            if (parent.State() != before)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 6. The tick under a seed is deterministic: two identical worlds
        //    with the same seed produce identical intents, and the seed
        //    shapes personality without changing actions.
        //-------------------------------------------------------------------------
        {
            // The registry is intentionally non-copyable, so each world
            // is built in place and the farmer's ID is read back after
            // the tick. Two worlds, one seed.
            Simulation::EntityRegistry worldA;

            const auto farmerA = worldA.CreateEntity();
            const auto merchantA = worldA.CreateEntity();

            worldA.AddComponent<Simulation::Needs>(
                farmerA,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
                });

            Simulation::Remember(
                worldA, farmerA,
                { merchantA, Simulation::InteractionKind::Trade, 1.0f });

            Simulation::EntityRegistry worldB;

            const auto farmerB = worldB.CreateEntity();
            const auto merchantB = worldB.CreateEntity();

            worldB.AddComponent<Simulation::Needs>(
                farmerB,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
                });

            Simulation::Remember(
                worldB, farmerB,
                { merchantB, Simulation::InteractionKind::Trade, 1.0f });

            Simulation::Rng seedA{ 4242 };
            Simulation::Rng seedB{ 4242 };

            Simulation::Update(worldA, 1.0, {}, nullptr, &seedA);
            Simulation::Update(worldB, 1.0, {}, nullptr, &seedB);

            const auto intentA = worldA.GetComponent<Simulation::Intent>(farmerA);
            const auto intentB = worldB.GetComponent<Simulation::Intent>(farmerB);

            if (!intentA || !intentB)
            {
                return false;
            }

            // Same seed, same world, same decision — confidence included.
            if (intentA->Action != intentB->Action
                || intentA->Target != intentB->Target
                || intentA->Confidence != intentB->Confidence)
            {
                return false;
            }

            // A different seed changes personality but not the action:
            // both farmers are hungry and know the merchant, so both go
            // to market — the jitter only varies how strongly.
            Simulation::EntityRegistry worldC;

            const auto farmerC = worldC.CreateEntity();
            const auto merchantC = worldC.CreateEntity();

            worldC.AddComponent<Simulation::Needs>(
                farmerC,
                Simulation::Needs{
                    { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
                });

            Simulation::Remember(
                worldC, farmerC,
                { merchantC, Simulation::InteractionKind::Trade, 1.0f });

            Simulation::Rng seedC{ 9999 };

            Simulation::Update(worldC, 1.0, {}, nullptr, &seedC);

            const auto intentC = worldC.GetComponent<Simulation::Intent>(farmerC);

            if (!intentC || intentC->Action != intentA->Action)
            {
                return false;
            }

            bool jitterDiffers = false;

            for (int i = 0; i < 16; ++i)
            {
                Simulation::Rng probeA{ 4242 };
                Simulation::Rng probeC{ 9999 };

                if (probeA.Derive(farmerA.Value()).NextFloat(-0.05f, 0.05f)
                    != probeC.Derive(farmerC.Value()).NextFloat(-0.05f, 0.05f))
                {
                    jitterDiffers = true;
                    break;
                }
            }

            // The two seeds may occasionally collide on a draw, but over
            // several draws at least one must differ — different seeds
            // mean different personalities.
            if (!jitterDiffers)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 7. StableDerive is seed-anchored: the parent may advance freely
        //    between calls (births, mediation — the adapter's pattern),
        //    and the child for a key never moves. This is the 0.8.x
        //    field finding: a near-tied mind re-rolled its intent every
        //    frame because the tick's per-entity noise followed the
        //    parent's live state. A settled mind must rest, not re-roll.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng parent{ 31415 };

            const auto before = parent.StableDerive(7000);

            // The parent advances — the adapter's births draw between
            // ticks. The anchored child must be untouched.
            for (int i = 0; i < 64; ++i)
            {
                parent.Next();
            }

            const auto after = parent.StableDerive(7000);

            for (int i = 0; i < 16; ++i)
            {
                if (before.Next() != after.Next())
                {
                    return false;
                }
            }

            // Same seed, same key → same child, always. Different seed →
            // different personality (the jitter comes from the seed).
            Simulation::Rng twin{ 31415 };
            Simulation::Rng other{ 31416 };

            auto twinChild = twin.StableDerive(7000);
            auto otherChild = other.StableDerive(7000);

            bool anyDifferent = false;

            for (int i = 0; i < 16 && !anyDifferent; ++i)
            {
                anyDifferent = (twinChild.Next() != otherChild.Next());
            }

            if (!anyDifferent)
            {
                return false;
            }

            // A fresh Rng from the same seed matches the advanced parent
            // — the anchor is the seed, not the stream position.
            Simulation::Rng fresh{ 31415 };
            auto freshChild = fresh.StableDerive(7000);
            auto advancedChild = parent.StableDerive(7000);

            for (int i = 0; i < 16; ++i)
            {
                if (freshChild.Next() != advancedChild.Next())
                {
                    return false;
                }
            }
        }

        return true;
    }
}
