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
// │          “If you can’t measure it, you can’t gate on it.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      main.cpp
//
// Purpose:
//
//      LCE Bench (0.8.7) — the Scale numbers, measured here and now.
//      The 0.8.0 milestone documented the cost of a settlement: ms per
//      tick at population, bytes per mind in the co-save, snapshot
//      round-trip, determinism at scale. This tool reproduces those
//      numbers on any machine — same seed, same scenarios, same
//      methodology as the Scale suite — so the 0.9.0 gate
//      ("performance at scale verified") is measurable, not asserted.
//
//      Numbers are machine-dependent by nature. The tool prints them;
//      the gate reads them. --sanity runs a tiny scenario and exits —
//      CI's smoke check that the tool runs on every toolchain.
//
// Project:
//
//      Living Commonwealth Engine (LCE)
//
// License:
//
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//
//      (c) 2026-present LCE Contributors
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace LCE::Simulation;

    //-------------------------------------------------------------------------
    // The byte pack/unpack helpers — the adapter's job in the real world.
    // The bench plays the adapter so the co-save numbers are provable.
    //-------------------------------------------------------------------------
    struct Writer
    {
        ComponentBlob Blob;

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

    struct Reader
    {
        const ComponentBlob& Bytes;
        std::size_t At = 0;

        std::uint64_t U64()
        {
            std::uint64_t value = 0;

            for (int shift = 0; shift < 64; shift += 8)
            {
                value |= static_cast<std::uint64_t>(Bytes[At + shift / 8])
                    << shift;
            }

            At += 8;
            return value;
        }

        float F()
        {
            const auto bits = static_cast<std::uint32_t>(U64());
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
    };

    ComponentSerializer<Needs> NeedsCodec()
    {
        return {
            [](const Needs& needs)
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
            [](const ComponentBlob& bytes)
            {
                Reader reader{ bytes };
                Needs needs;
                const auto count = reader.U64();

                for (std::uint64_t i = 0; i < count; ++i)
                {
                    needs.List.push_back(Need{
                        static_cast<NeedType>(reader.U64()),
                        reader.F(),
                        reader.F() });
                }

                return needs;
            } };
    }

    ComponentSerializer<Memory> MemoryCodec()
    {
        return {
            [](const Memory& memory)
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
            [](const ComponentBlob& bytes)
            {
                Reader reader{ bytes };
                Memory memory;
                const auto count = reader.U64();

                for (std::uint64_t i = 0; i < count; ++i)
                {
                    memory.Events.push_back(MemoryEvent{
                        EntityId{ reader.U64() },
                        static_cast<InteractionKind>(reader.U64()),
                        reader.F(),
                        reader.U64() });
                }

                return memory;
            } };
    }

    ComponentSerializer<Relationships> RelationshipsCodec()
    {
        return {
            [](const Relationships& relationships)
            {
                // Deterministic by construction: sort by Other.Value()
                // so two identical worlds serialize identically (the
                // QueryWhere discipline, applied to the codec).
                std::vector<std::pair<EntityId, Relationship>> ordered(
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
            [](const ComponentBlob& bytes)
            {
                Reader reader{ bytes };
                Relationships relationships;
                const auto count = reader.U64();

                for (std::uint64_t i = 0; i < count; ++i)
                {
                    const auto other = EntityId{ reader.U64() };
                    const auto disposition = reader.F();
                    const auto trust = reader.F();

                    relationships.ByEntity[other] =
                        Relationship{ disposition, trust };
                }

                return relationships;
            } };
    }

    void RegisterCodecs(EntityRegistry& registry)
    {
        registry.RegisterSerializer<Needs>(NeedsCodec());
        registry.RegisterSerializer<Memory>(MemoryCodec());
        registry.RegisterSerializer<Relationships>(RelationshipsCodec());
    }

    //-------------------------------------------------------------------------
    // A reusable mind: hungry, with a trader remembered and a relationship
    // to them — the shape of a settler going to market. The same shape the
    // Scale suite uses, so the numbers are comparable.
    //-------------------------------------------------------------------------
    void SeedMind(
        EntityRegistry& registry,
        EntityId mind,
        EntityId trader)
    {
        registry.AddComponent<Needs>(
            mind,
            Needs{ { Need{ NeedType::Hunger, 0.8f, 0.02f } } });

        Remember(registry, mind, { trader, InteractionKind::Trade, 1.0f });
        Remember(registry, mind, { trader, InteractionKind::Trade, 1.0f });
        Remember(registry, mind, { trader, InteractionKind::Social, 0.6f });
    }

    //-------------------------------------------------------------------------
    // Flatten: a canonical byte stream of a snapshot — sorted, so two
    // identical worlds flatten identically. Byte-for-byte equality of the
    // flattened streams IS the determinism claim (stone 17).
    //-------------------------------------------------------------------------
    std::string Flatten(const RegistrySnapshot& snapshot)
    {
        std::string out;

        const auto push = [&out](std::uint64_t value)
        {
            for (int shift = 0; shift < 64; shift += 8)
            {
                out.push_back(static_cast<char>((value >> shift) & 0xFFull));
            }
        };

        push(snapshot.Version);

        std::vector<const SnapshotEntity*> entities;

        for (const auto& entity : snapshot.Entities)
        {
            entities.push_back(&entity);
        }

        std::sort(
            entities.begin(),
            entities.end(),
            [](const SnapshotEntity* left, const SnapshotEntity* right)
            {
                return left->Id.Value() < right->Id.Value();
            });

        push(entities.size());

        for (const auto* entity : entities)
        {
            push(entity->Id.Value());

            std::vector<const SnapshotComponent*> components;

            for (const auto& component : entity->Components)
            {
                components.push_back(&component);
            }

            std::sort(
                components.begin(),
                components.end(),
                [](const SnapshotComponent* left,
                   const SnapshotComponent* right)
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

        return out;
    }

    //-------------------------------------------------------------------------
    // Fills a world with N seeded minds around one trader. The registry
    // is non-copyable by design (it owns component stores), so the world
    // is built in place.
    //-------------------------------------------------------------------------
    void FillWorld(EntityRegistry& registry, std::size_t minds)
    {
        RegisterCodecs(registry);

        const auto trader = registry.CreateEntity();

        for (std::size_t i = 0; i < minds; ++i)
        {
            SeedMind(registry, registry.CreateEntity(), trader);
        }
    }

    double Milliseconds(std::chrono::steady_clock::time_point start)
    {
        return std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - start)
            .count();
    }

    //-------------------------------------------------------------------------
    // Scenario 1 — tick cost at population: warm up, then average the
    // per-pass breakdown over `ticks` whole steps. The 0.9.0 gate's
    // "performance at scale" is this number, measured on the target box.
    //-------------------------------------------------------------------------
    void TickCost(std::size_t minds, std::size_t ticks)
    {
        EntityRegistry registry;
        FillWorld(registry, minds);

        SimulationTuning tuning;
        tuning.NeedJitter = 0.15f;
        tuning.MemoryCap = 32;

        Rng rng(2026);
        FixedStep fixed;
        TickReport report;

        // Warm up — allocators and caches settle before measurement.
        for (int i = 0; i < 20; ++i)
        {
            fixed.Advance(0.1, registry, tuning, nullptr, &rng, &report);
        }

        double needs = 0.0;
        double memory = 0.0;
        double relationships = 0.0;
        double goals = 0.0;
        double decide = 0.0;
        double total = 0.0;

        for (std::size_t i = 0; i < ticks; ++i)
        {
            fixed.Advance(0.1, registry, tuning, nullptr, &rng, &report);
            needs += report.NeedsMs;
            memory += report.MemoryMs;
            relationships += report.RelationshipsMs;
            goals += report.GoalsMs;
            decide += report.DecideMs;
            total += report.TotalMs;
        }

        const auto scale = 1.0 / static_cast<double>(ticks);

        std::printf(
            "  %5zu minds : %6.2f ms/tick "
            "(needs %.2f, memory %.2f, relationships %.2f, "
            "goals %.2f, decide %.2f)\n",
            minds,
            total * scale,
            needs * scale,
            memory * scale,
            relationships * scale,
            goals * scale,
            decide * scale);
    }

    //-------------------------------------------------------------------------
    // Scenario 2 — the co-save: bytes per mind and the round-trip cost.
    // The documented shape (0.8.0 stone 16): a seeded mind — needs, three
    // memory events, one relationship — costs ~207 bytes.
    //-------------------------------------------------------------------------
    void CoSave(std::size_t minds)
    {
        EntityRegistry registry;
        FillWorld(registry, minds);

        const auto captureStart = std::chrono::steady_clock::now();
        const auto snapshot = registry.Capture();
        const auto captureMs = Milliseconds(captureStart);

        const auto flattened = Flatten(snapshot);
        const auto entities = snapshot.Entities.size();

        auto reloaded = EntityRegistry{};
        RegisterCodecs(reloaded);

        const auto restoreStart = std::chrono::steady_clock::now();
        reloaded.Restore(snapshot);
        const auto restoreMs = Milliseconds(restoreStart);

        const auto exact = Flatten(reloaded.Capture()) == flattened;

        std::printf(
            "  %5zu entities : %zu bytes total, %zu per mind "
            "(capture %.2f ms, restore %.2f ms, round-trip exact: %s)\n",
            entities,
            flattened.size(),
            entities != 0 ? flattened.size() / entities : 0,
            captureMs,
            restoreMs,
            exact ? "yes" : "NO");
    }

    //-------------------------------------------------------------------------
    // Scenario 3 — determinism at scale: two worlds, same seed, same steps
    // — byte-identical snapshots. The claim the co-save stands on.
    //-------------------------------------------------------------------------
    void Determinism(std::size_t minds, std::size_t ticks)
    {
        EntityRegistry worldA;
        EntityRegistry worldB;
        FillWorld(worldA, minds);
        FillWorld(worldB, minds);

        Rng rngA(2026);
        Rng rngB(2026);

        SimulationTuning tuning;
        tuning.NeedJitter = 0.15f;

        FixedStep stepA;
        FixedStep stepB;

        for (std::size_t i = 0; i < ticks; ++i)
        {
            stepA.Advance(0.1, worldA, tuning, nullptr, &rngA);
            stepB.Advance(0.1, worldB, tuning, nullptr, &rngB);
        }

        const auto identical =
            Flatten(worldA.Capture()) == Flatten(worldB.Capture());

        std::printf(
            "  %5zu minds, %5zu steps, same seed : %s\n",
            minds,
            ticks,
            identical ? "bit-identical" : "DIVERGED");
    }

    //-------------------------------------------------------------------------
    // Scenario 4 — the memory cap bounds the hot path: remember far more
    // than the cap allows; every mind stays at the cap, tick included.
    //-------------------------------------------------------------------------
    void MemoryCap(std::size_t minds, std::size_t cap)
    {
        EntityRegistry registry;
        FillWorld(registry, minds);

        SimulationTuning tuning;
        tuning.MemoryCap = cap;

        const auto trader = registry.CreateEntity();
        const auto entities = registry.QueryWhere<Needs>(
            [](EntityId, const Needs&) { return true; });

        // Forty remembers into a cap of `cap` — the store must never
        // exceed it.
        for (const auto mind : entities)
        {
            for (int i = 0; i < 40; ++i)
            {
                Remember(
                    registry,
                    mind,
                    { trader, InteractionKind::Aid, 0.5f + 0.01f * i },
                    tuning);
            }
        }

        std::size_t over = 0;
        std::size_t totalEvents = 0;

        registry.ForEachWithComponent<Memory>(
            [&](EntityId, Memory& memory)
            {
                totalEvents += memory.Events.size();

                if (memory.Events.size() > cap)
                {
                    ++over;
                }
            });

        std::printf(
            "  %5zu minds, cap %zu : every mind at or under the cap: %s "
            "(%zu events total)\n",
            minds,
            cap,
            over == 0 ? "yes" : "NO",
            totalEvents);
    }
}

int main(int argc, char** argv)
{
    const bool sanity =
        argc > 1 && std::string_view(argv[1]) == "--sanity";

    std::printf("LCE Bench — the Scale numbers, measured here (0.8.7)\n");
    std::printf("seed 2026, settler-shaped minds (needs + 3 memories + 1 relationship)\n\n");

    if (sanity)
    {
        // CI smoke: tiny scenario, runs on every toolchain, exits 0.
        TickCost(100, 5);
        CoSave(100);
        Determinism(50, 10);
        MemoryCap(50, 8);
        std::printf("\nbench sanity: OK\n");
        return 0;
    }

    std::printf("tick cost (averaged over 200 steps, sim.jitter 0.15, sim.memory.cap 32):\n");
    TickCost(1000, 200);
    TickCost(5000, 200);
    TickCost(20000, 200);

    std::printf("\nco-save (per-mind cost of a seeded settler):\n");
    CoSave(1000);
    CoSave(5000);

    std::printf("\ndeterminism at scale:\n");
    Determinism(1000, 1000);

    std::printf("\nmemory cap:\n");
    MemoryCap(1000, 16);

    std::printf("\ndone — compare these to Docs/Design/Scale.md on this machine.\n");
    return 0;
}
