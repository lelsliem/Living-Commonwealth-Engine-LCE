//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      ScaleTest.cpp
//
// Purpose:
//
//      0.8.0 Scale proofs: the cost of a settlement is knowable
//      (stone 13 TickReport), the hot path is bounded (stone 14a the
//      memory cap), the tick is timing-independent (stone 14b
//      FixedStep), save/load round-trips at population scale (stone
//      16), and determinism holds byte-for-byte at scale (stone 17).
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace LCE::Tests
{
    namespace
    {
        //-------------------------------------------------------------------------
        // Tiny byte pack/unpack helpers — the adapter's job in the real
        // world; the test plays the adapter so the registry round-trip
        // and the byte-identical determinism claim are provable here.
        //-------------------------------------------------------------------------
        struct Writer
        {
            Simulation::ComponentBlob Blob;

            void U64(std::uint64_t value)
            {
                for (int i = 0; i < 8; ++i)
                {
                    Blob.push_back(
                        static_cast<std::byte>((value >> (8 * i)) & 0xFFull));
                }
            }

            void F(float value)
            {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                U64(bits);
            }
        };

        Simulation::ComponentSerializer<Simulation::Needs> NeedsCodec()
        {
            return {
                [](const Simulation::Needs& needs)
                {
                    Writer writer;
                    writer.U64(needs.List.size());

                    for (const auto& need : needs.List)
                    {
                        writer.U64(static_cast<std::uint64_t>(need.Type));
                        writer.F(need.Value);
                        writer.F(need.DecayRate);
                    }

                    return writer.Blob;
                },
                [](const Simulation::ComponentBlob& bytes)
                {
                    Simulation::Needs needs;
                    std::size_t at = 0;

                    const auto take = [&at, &bytes]() -> std::uint64_t
                    {
                        std::uint64_t value = 0;

                        for (int shift = 0; shift < 64; shift += 8)
                        {
                            value |= static_cast<std::uint64_t>(
                                bytes[at + shift / 8]) << shift;
                        }

                        at += 8;
                        return value;
                    };

                    const auto takeFloat = [&take]() -> float
                    {
                        const auto bits = static_cast<std::uint32_t>(take());
                        float value = 0.0f;
                        std::memcpy(&value, &bits, sizeof(value));
                        return value;
                    };

                    const auto count = take();

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        needs.List.push_back(Simulation::Need{
                            static_cast<Simulation::NeedType>(take()),
                            takeFloat(),
                            takeFloat() });
                    }

                    return needs;
                }};
        }

        Simulation::ComponentSerializer<Simulation::Memory> MemoryCodec()
        {
            return {
                [](const Simulation::Memory& memory)
                {
                    Writer writer;
                    writer.U64(memory.Events.size());

                    for (const auto& event : memory.Events)
                    {
                        writer.U64(event.Other.Value());
                        writer.U64(static_cast<std::uint64_t>(event.Kind));
                        writer.F(event.Weight);
                        writer.U64(event.Day);
                    }

                    return writer.Blob;
                },
                [](const Simulation::ComponentBlob& bytes)
                {
                    Simulation::Memory memory;
                    std::size_t at = 0;

                    const auto take = [&at, &bytes]() -> std::uint64_t
                    {
                        std::uint64_t value = 0;

                        for (int shift = 0; shift < 64; shift += 8)
                        {
                            value |= static_cast<std::uint64_t>(
                                bytes[at + shift / 8]) << shift;
                        }

                        at += 8;
                        return value;
                    };

                    const auto takeFloat = [&take]() -> float
                    {
                        const auto bits = static_cast<std::uint32_t>(take());
                        float value = 0.0f;
                        std::memcpy(&value, &bits, sizeof(value));
                        return value;
                    };

                    const auto count = take();

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        memory.Events.push_back(Simulation::MemoryEvent{
                            Simulation::EntityId{ take() },
                            static_cast<Simulation::InteractionKind>(take()),
                            takeFloat(),
                            take() });
                    }

                    return memory;
                }};
        }

        Simulation::ComponentSerializer<Simulation::Relationships>
        RelationshipsCodec()
        {
            return {
                [](const Simulation::Relationships& relationships)
                {
                    // Deterministic by construction: the store is an
                    // unordered_map whose bucket order is an implementation
                    // detail — sort by Other.Value() so a round-trip (and
                    // two identical worlds) serialize identically. The
                    // QueryWhere discipline, applied to the codec.
                    std::vector<std::pair<Simulation::EntityId,
                                          Simulation::Relationship>>
                        ordered(
                            relationships.ByEntity.begin(),
                            relationships.ByEntity.end());

                    std::sort(
                        ordered.begin(),
                        ordered.end(),
                        [](const auto& left, const auto& right)
                        {
                            return left.first.Value() < right.first.Value();
                        });

                    Writer writer;
                    writer.U64(ordered.size());

                    for (const auto& [other, relationship] : ordered)
                    {
                        writer.U64(other.Value());
                        writer.F(relationship.Disposition);
                        writer.F(relationship.Trust);
                    }

                    return writer.Blob;
                },
                [](const Simulation::ComponentBlob& bytes)
                {
                    Simulation::Relationships relationships;
                    std::size_t at = 0;

                    const auto take = [&at, &bytes]() -> std::uint64_t
                    {
                        std::uint64_t value = 0;

                        for (int shift = 0; shift < 64; shift += 8)
                        {
                            value |= static_cast<std::uint64_t>(
                                bytes[at + shift / 8]) << shift;
                        }

                        at += 8;
                        return value;
                    };

                    const auto takeFloat = [&take]() -> float
                    {
                        const auto bits = static_cast<std::uint32_t>(take());
                        float value = 0.0f;
                        std::memcpy(&value, &bits, sizeof(value));
                        return value;
                    };

                    const auto count = take();

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        // Read the fields into locals FIRST: in
                        // map[key] = value the right-hand side evaluates
                        // before the left, so an inline take() there
                        // would read the wrong bytes. Order the reads,
                        // then insert.
                        const auto other = Simulation::EntityId{ take() };
                        const auto disposition = takeFloat();
                        const auto trust = takeFloat();

                        relationships.ByEntity[other] =
                            Simulation::Relationship{ disposition, trust };
                    }

                    return relationships;
                }};
        }

        void RegisterCodecs(Simulation::EntityRegistry& registry)
        {
            registry.RegisterSerializer<Simulation::Needs>(NeedsCodec());
            registry.RegisterSerializer<Simulation::Memory>(MemoryCodec());
            registry.RegisterSerializer<Simulation::Relationships>(
                RelationshipsCodec());
        }

        //-------------------------------------------------------------------------
        // Flatten: a canonical byte stream of a snapshot — sorted, so two
        // identical worlds flatten identically whatever the stores'
        // unordered iteration happened to produce. Byte-for-byte equality
        // of the flattened streams IS the determinism claim (stone 17).
        //-------------------------------------------------------------------------
        std::string Flatten(const Simulation::RegistrySnapshot& snapshot)
        {
            std::string out;

            const auto push = [&out](std::uint64_t value)
            {
                for (int shift = 0; shift < 64; shift += 8)
                {
                    out.push_back(
                        static_cast<char>((value >> shift) & 0xFFull));
                }
            };

            push(snapshot.Version);

            std::vector<const Simulation::SnapshotEntity*> entities;

            for (const auto& entity : snapshot.Entities)
            {
                entities.push_back(&entity);
            }

            std::sort(
                entities.begin(),
                entities.end(),
                [](const Simulation::SnapshotEntity* left,
                   const Simulation::SnapshotEntity* right)
                {
                    return left->Id.Value() < right->Id.Value();
                });

            push(entities.size());

            for (const auto* entity : entities)
            {
                push(entity->Id.Value());

                std::vector<const Simulation::SnapshotComponent*> components;

                for (const auto& component : entity->Components)
                {
                    components.push_back(&component);
                }

                std::sort(
                    components.begin(),
                    components.end(),
                    [](const Simulation::SnapshotComponent* left,
                       const Simulation::SnapshotComponent* right)
                    {
                        return left->Type < right->Type;
                    });

                push(components.size());

                for (const auto* component : components)
                {
                    push(component->Data.size());
                    out.append(
                        reinterpret_cast<const char*>(component->Data.data()),
                        component->Data.size());
                }
            }

            if (snapshot.Legacy)
            {
                push(1);
                push(snapshot.Legacy->size());
                out.append(
                    reinterpret_cast<const char*>(snapshot.Legacy->data()),
                    snapshot.Legacy->size());
            }
            else
            {
                push(0);
            }

            return out;
        }

        // A reusable mind: hungry, with a trader remembered and a
        // relationship to them — the shape of a settler going to market.
        void SeedMind(
            Simulation::EntityRegistry& registry,
            Simulation::EntityId mind,
            Simulation::EntityId trader)
        {
            registry.AddComponent<Simulation::Needs>(
                mind,
                Simulation::Needs{
                    { Simulation::Need{
                          Simulation::NeedType::Hunger, 0.8f, 0.02f } } });

            Simulation::Remember(
                registry, mind, { trader, Simulation::InteractionKind::Trade, 1.0f });
            Simulation::Remember(
                registry, mind, { trader, Simulation::InteractionKind::Trade, 1.0f });
            Simulation::Remember(
                registry, mind,
                { trader, Simulation::InteractionKind::Social, 0.6f });
        }
    }

    bool ScaleTest()
    {
        //-------------------------------------------------------------------------
        // 1. Stone 13 — TickReport: counts are exact on a known registry,
        //    and the per-pass wall times are present (>= 0, never NaN).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto trader = registry.CreateEntity();
            const auto alice = registry.CreateEntity();
            const auto bob = registry.CreateEntity();

            SeedMind(registry, alice, trader);
            SeedMind(registry, bob, trader);

            Simulation::SimulationTuning tuning;
            Simulation::TickReport report;

            Simulation::Update(registry, 0.5, tuning, nullptr, nullptr, &report);

            if (report.Entities != 2)
            {
                return false;   // exactly the two minds
            }

            // Alice and Bob each remembered 3 events; none should have
            // faded in half a second at the default rate.
            if (report.MemoryEvents != 6)
            {
                return false;
            }

            // Each mind has one relationship (toward the trader).
            if (report.Relationships != 2)
            {
                return false;
            }

            if (report.TotalMs < 0.0 || report.NeedsMs < 0.0
                || report.MemoryMs < 0.0 || report.RelationshipsMs < 0.0
                || report.DecideMs < 0.0)
            {
                return false;
            }

            // A second tick without a report is the untouched default —
            // the tick runs, the world moves, nothing is measured.
            Simulation::TickReport untouched;
            Simulation::Update(registry, 0.5, tuning, nullptr, nullptr, nullptr);

            if (untouched.Entities != 0)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 2. Stone 14a — the memory cap: a mind can only hold so much.
        //    The lowest-weight event is evicted; ties go to the oldest.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;
            Simulation::SimulationTuning tuning;
            tuning.MemoryCap = 3;

            const auto other = registry.CreateEntity();
            const auto mind = registry.CreateEntity();

            Simulation::Remember(
                registry, mind,
                { other, Simulation::InteractionKind::Aid, 0.9f }, tuning);
            Simulation::Remember(
                registry, mind,
                { other, Simulation::InteractionKind::Trade, 0.4f }, tuning);
            Simulation::Remember(
                registry, mind,
                { other, Simulation::InteractionKind::Aid, 0.8f }, tuning);
            Simulation::Remember(
                registry, mind,
                { other, Simulation::InteractionKind::Social, 0.6f }, tuning);

            const auto memory = registry.GetComponent<Simulation::Memory>(mind);

            if (!memory)
            {
                return false;
            }

            // Five remembers, cap three: the weakest (0.4) is gone, the
            // rest survive in append order.
            if (memory->Events.size() != 3)
            {
                return false;
            }

            for (const auto& event : memory->Events)
            {
                if (event.Weight <= 0.4f + 0.0001f)
                {
                    return false;   // the evicted one must not be present
                }
            }

            // Ties evict the oldest: two events at the same lowest weight,
            // the earlier one goes.
            Simulation::EntityRegistry registry2;
            Simulation::SimulationTuning tuning2;
            tuning2.MemoryCap = 2;

            const auto other2 = registry2.CreateEntity();
            const auto mind2 = registry2.CreateEntity();

            Simulation::Remember(
                registry2, mind2,
                { other2, Simulation::InteractionKind::Aid, 0.5f }, tuning2);
            Simulation::Remember(
                registry2, mind2,
                { other2, Simulation::InteractionKind::Trade, 1.0f }, tuning2);
            Simulation::Remember(
                registry2, mind2,
                { other2, Simulation::InteractionKind::Social, 0.5f }, tuning2);

            const auto memory2 = registry2.GetComponent<Simulation::Memory>(mind2);

            if (!memory2 || memory2->Events.size() != 2)
            {
                return false;
            }

            // Exactly one 0.5 survives (the later one — ties evict the
            // oldest), and it must not be the first in append order.
            std::size_t halfWeights = 0;
            bool oldestIsHalf = false;

            for (std::size_t i = 0; i < memory2->Events.size(); ++i)
            {
                if (memory2->Events[i].Weight == 0.5f)
                {
                    ++halfWeights;
                    oldestIsHalf = (i == 0);
                }
            }

            if (halfWeights != 1 || oldestIsHalf)
            {
                return false;
            }

            // Cap 0 = unbounded: every event stays (the default, so no
            // existing caller is affected).
            Simulation::EntityRegistry registry3;
            Simulation::SimulationTuning tuning3;   // MemoryCap == 0

            const auto other3 = registry3.CreateEntity();
            const auto mind3 = registry3.CreateEntity();

            for (int i = 0; i < 10; ++i)
            {
                Simulation::Remember(
                    registry3, mind3,
                    { other3, Simulation::InteractionKind::Aid, 1.0f });
            }

            const auto memory3 = registry3.GetComponent<Simulation::Memory>(mind3);

            if (!memory3 || memory3->Events.size() != 10)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. Stone 14b — FixedStep: whole steps only, the remainder
        //    carries, and same seed + same steps = same world whatever
        //    the frame rate (the timing-independence proof).
        //-------------------------------------------------------------------------
        {
            // The carry mechanics, in exact-binary seconds (0.25, 0.125):
            // whole steps run, the remainder carries, and a later frame
            // completes it. Binary-exact deltas keep the test honest —
            // the real cadence is proven by the slow/fast worlds below.
            Simulation::FixedStep fixed;
            fixed.Step = 0.25;

            Simulation::EntityRegistry registry;
            const auto trader = registry.CreateEntity();
            const auto mind = registry.CreateEntity();
            SeedMind(registry, mind, trader);

            const auto steps = fixed.Advance(0.5, registry);
            const auto more = fixed.Advance(0.125, registry);
            const auto finished = fixed.Advance(0.125, registry);

            if (steps != 2)
            {
                return false;   // 0.5s at 0.25s cadence = two whole steps
            }

            if (more != 0)
            {
                return false;   // the half step carries, nothing runs yet
            }

            if (finished != 1)
            {
                return false;   // the carried 0.125 completes a third step
            }

            // Two identical worlds, same seed, different frame rates:
            // the slow world advances 0.5s a call; the fast world
            // advances 0.1s a call. Both run the same whole steps —
            // the sims are bit-identical.
            Simulation::EntityRegistry slow;
            Simulation::EntityRegistry fast;
            RegisterCodecs(slow);
            RegisterCodecs(fast);

            Simulation::Rng slowRng(1234);
            Simulation::Rng fastRng(1234);
            Simulation::SimulationTuning tuning;

            const auto slowTrader = slow.CreateEntity();
            const auto fastTrader = fast.CreateEntity();

            for (int i = 0; i < 60; ++i)
            {
                SeedMind(slow, slow.CreateEntity(), slowTrader);
                SeedMind(fast, fast.CreateEntity(), fastTrader);
            }

            Simulation::FixedStep slowFixed;
            Simulation::FixedStep fastFixed;

            for (int i = 0; i < 40; ++i)
            {
                slowFixed.Advance(0.5, slow, tuning, nullptr, &slowRng);
            }

            for (int i = 0; i < 200; ++i)
            {
                fastFixed.Advance(0.1, fast, tuning, nullptr, &fastRng);
            }

            if (Flatten(slow.Capture()) != Flatten(fast.Capture()))
            {
                return false;   // timing must not leak into the world
            }
        }

        //-------------------------------------------------------------------------
        // 4. Stone 16 — save/load at population scale: 5000 minds
        //    round-trip, and the per-mind co-save cost is documented.
        //-------------------------------------------------------------------------
        {
            std::printf("[Scale] marker 4\n");
            Simulation::EntityRegistry source;
            RegisterCodecs(source);

            const auto trader = source.CreateEntity();

            for (int i = 0; i < 5000; ++i)
            {
                SeedMind(source, source.CreateEntity(), trader);
            }

            const auto snapshot = source.Capture();
            const auto totalBytes = Flatten(snapshot).size();

            if (snapshot.Entities.size() != 5001)
            {
                return false;   // the trader plus the 5000 minds
            }

            const auto perMind = totalBytes / 5001u;

            // The documented shape: a seeded mind (needs + three memory
            // events + one relationship) costs well under a kilobyte.
            if (perMind > 1024u)
            {
                return false;
            }

            Simulation::EntityRegistry reloaded;
            RegisterCodecs(reloaded);
            reloaded.Restore(snapshot);

            if (Flatten(reloaded.Capture()) != Flatten(snapshot))
            {
                return false;   // round-trip is exact at population scale
            }

            // The documented shape (0.8.0 stone 16): a seeded mind —
            // needs, three memory events, one relationship — costs well
            // under a kilobyte in the co-save. The number goes into
            // Docs/Design/Scale.md as the per-entity cost model.
            std::printf(
                "[Scale] 5000 minds round-trip: %llu bytes total, "
                "%llu per mind\n",
                static_cast<unsigned long long>(totalBytes),
                static_cast<unsigned long long>(perMind));
        }

        //-------------------------------------------------------------------------
        // 5. Stone 17 — determinism at scale: two worlds, same seed, same
        //    steps, thousands of minds — bit-identical snapshots.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry worldA;
            Simulation::EntityRegistry worldB;
            RegisterCodecs(worldA);
            RegisterCodecs(worldB);

            Simulation::Rng rngA(2026);
            Simulation::Rng rngB(2026);
            Simulation::SimulationTuning tuning;
            tuning.NeedJitter = 0.15f;

            const auto traderA = worldA.CreateEntity();
            const auto traderB = worldB.CreateEntity();

            for (int i = 0; i < 1000; ++i)
            {
                SeedMind(worldA, worldA.CreateEntity(), traderA);
                SeedMind(worldB, worldB.CreateEntity(), traderB);
            }

            Simulation::FixedStep stepA;
            Simulation::FixedStep stepB;

            for (int i = 0; i < 1000; ++i)
            {
                stepA.Advance(0.1, worldA, tuning, nullptr, &rngA);
                stepB.Advance(0.1, worldB, tuning, nullptr, &rngB);
            }

            if (Flatten(worldA.Capture()) != Flatten(worldB.Capture()))
            {
                return false;   // same seed + same steps = same world
            }
        }

        return true;
    }
}
