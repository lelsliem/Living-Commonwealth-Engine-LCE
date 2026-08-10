//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SnapshotTest.cpp
//
// Purpose:
//
//      Proves the 0.4.0 save/load substrate: a living registry captured as
//      pure data and restored into a fresh one behaves identically — the
//      farmer still goes to market after the load.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
    //-------------------------------------------------------------------------
    // Tiny byte pack/unpack helpers. This is the adapter's job in the real
    // world — the core never interprets blobs. Here the test plays the
    // adapter to prove the registry round-trip.
    //-------------------------------------------------------------------------
    struct Writer
    {
        LCE::Simulation::ComponentBlob Blob;

        void U32(std::uint32_t value)
        {
            for (int i = 0; i < 4; ++i)
            {
                Blob.push_back(
                    static_cast<std::byte>((value >> (8 * i)) & 0xFFu));
            }
        }

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
            U32(bits);
        }
    };

    struct Reader
    {
        const LCE::Simulation::ComponentBlob& Blob;
        std::size_t Position = 0;

        std::uint32_t U32()
        {
            std::uint32_t value = 0;

            for (int i = 0; i < 4; ++i)
            {
                value |= std::to_integer<std::uint32_t>(Blob[Position++])
                    << (8 * i);
            }

            return value;
        }

        std::uint64_t U64()
        {
            std::uint64_t value = 0;

            for (int i = 0; i < 8; ++i)
            {
                value |= std::to_integer<std::uint64_t>(Blob[Position++])
                    << (8 * i);
            }

            return value;
        }

        float F()
        {
            std::uint32_t bits = U32();
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
    };

    LCE::Simulation::ComponentSerializer<LCE::Simulation::Needs>
    MakeNeedsSerializer()
    {
        return {
            [](const LCE::Simulation::Needs& needs)
            {
                Writer writer;
                writer.U32(static_cast<std::uint32_t>(needs.List.size()));

                for (const auto& need : needs.List)
                {
                    writer.U32(static_cast<std::uint32_t>(need.Type));
                    writer.F(need.Value);
                    writer.F(need.DecayRate);
                }

                return writer.Blob;
            },
            [](const LCE::Simulation::ComponentBlob& blob)
            {
                Reader reader{ blob };
                LCE::Simulation::Needs needs;

                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    needs.List.push_back(LCE::Simulation::Need{
                        static_cast<LCE::Simulation::NeedType>(reader.U32()),
                        reader.F(),
                        reader.F() });
                }

                return needs;
            }
        };
    }

    LCE::Simulation::ComponentSerializer<LCE::Simulation::Memory>
    MakeMemorySerializer()
    {
        return {
            [](const LCE::Simulation::Memory& memory)
            {
                Writer writer;
                writer.U32(static_cast<std::uint32_t>(memory.Events.size()));

                for (const auto& event : memory.Events)
                {
                    writer.U64(event.Other.Value());
                    writer.U32(static_cast<std::uint32_t>(event.Kind));
                    writer.F(event.Weight);
                    writer.U64(event.Day);   // world-time anchor (0.5.0)
                }

                return writer.Blob;
            },
            [](const LCE::Simulation::ComponentBlob& blob)
            {
                Reader reader{ blob };
                LCE::Simulation::Memory memory;

                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    memory.Events.push_back(LCE::Simulation::MemoryEvent{
                        LCE::Simulation::EntityId{ reader.U64() },
                        static_cast<LCE::Simulation::InteractionKind>(reader.U32()),
                        reader.F(),
                        reader.U64() });   // world-time anchor (0.5.0)
                }

                return memory;
            }
        };
    }

    LCE::Simulation::ComponentSerializer<LCE::Simulation::Relationships>
    MakeRelationshipsSerializer()
    {
        return {
            [](const LCE::Simulation::Relationships& relationships)
            {
                Writer writer;
                writer.U32(static_cast<std::uint32_t>(
                    relationships.ByEntity.size()));

                for (const auto& [other, relationship] : relationships.ByEntity)
                {
                    writer.U64(other.Value());
                    writer.F(relationship.Disposition);
                    writer.F(relationship.Trust);
                }

                return writer.Blob;
            },
            [](const LCE::Simulation::ComponentBlob& blob)
            {
                Reader reader{ blob };
                LCE::Simulation::Relationships relationships;

                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    const auto other = LCE::Simulation::EntityId{ reader.U64() };

                    relationships.ByEntity[other] =
                        LCE::Simulation::Relationship{
                            reader.F(), reader.F() };
                }

                return relationships;
            }
        };
    }
}

namespace LCE::Tests
{
    bool SnapshotTest()
    {
        //---------------------------------------------------------------------
        // The farmer's story survives a save and a load.
        //---------------------------------------------------------------------
        Simulation::EntityRegistry source;

        source.RegisterSerializer<Simulation::Needs>(MakeNeedsSerializer());
        source.RegisterSerializer<Simulation::Memory>(MakeMemorySerializer());
        source.RegisterSerializer<Simulation::Relationships>(
            MakeRelationshipsSerializer());

        const auto farmer = source.CreateEntity();
        const auto merchant = source.CreateEntity();

        source.AddComponent<Simulation::Needs>(
            farmer,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 0.3f, 0.1f } }
            });

        Simulation::Remember(
            source, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            source, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Update(source, 1.0);

        // Sanity: the source really is a mind going to market.
        {
            const auto intent = source.GetComponent<Simulation::Intent>(farmer);

            if (!intent
                || intent->Action != Simulation::ActionType::MoveTo
                || intent->Target != merchant)
            {
                return false;
            }
        }

        const auto snapshot = source.Capture();

        if (snapshot.Version != Simulation::kSnapshotVersion)
        {
            return false;
        }

        if (snapshot.Entities.size() != 2)
        {
            return false;   // the farmer and the merchant
        }

        if (snapshot.Entities[0].Id != farmer
            || snapshot.Entities[1].Id != merchant)
        {
            return false;   // slot order is deterministic
        }

        if (!snapshot.Entities[1].Components.empty())
        {
            return false;   // the merchant owns nothing — nothing persisted
        }

        //---------------------------------------------------------------------
        // A fresh registry — a fresh game session — loads the save.
        //---------------------------------------------------------------------
        Simulation::EntityRegistry restored;

        restored.RegisterSerializer<Simulation::Needs>(MakeNeedsSerializer());
        restored.RegisterSerializer<Simulation::Memory>(MakeMemorySerializer());
        restored.RegisterSerializer<Simulation::Relationships>(
            MakeRelationshipsSerializer());

        restored.Restore(snapshot);

        // Identity survived exactly: same index, same generation.
        if (!restored.IsAlive(farmer) || !restored.IsAlive(merchant))
        {
            return false;
        }

        const auto needs = restored.GetComponent<Simulation::Needs>(farmer);
        const auto memory = restored.GetComponent<Simulation::Memory>(farmer);
        const auto relationships =
            restored.GetComponent<Simulation::Relationships>(farmer);

        if (!needs || !memory || !relationships)
        {
            return false;
        }

        if (needs->List[0].Value > 0.21f || needs->List[0].Value < 0.19f)
        {
            return false;   // hunger decayed to 0.2, restored exactly
        }

        if (memory->Events.size() != 2 || memory->Events[0].Other != merchant)
        {
            return false;   // both trades remembered, still pointed at the merchant
        }

        if (relationships->ByEntity.at(merchant).Trust < 0.28f)
        {
            return false;   // two fair trades, trust restored
        }

        // The farmer still goes to market after the load. No script fired.
        Simulation::Update(restored, 0.0);

        const auto intent = restored.GetComponent<Simulation::Intent>(farmer);

        if (!intent
            || intent->Action != Simulation::ActionType::MoveTo
            || intent->Target != merchant)
        {
            return false;
        }

        //---------------------------------------------------------------------
        // A component type without a registered serializer is not persisted.
        //---------------------------------------------------------------------
        Simulation::EntityRegistry partial;
        partial.RegisterSerializer<Simulation::Needs>(MakeNeedsSerializer());

        const auto id = partial.CreateEntity();

        partial.AddComponent<Simulation::Needs>(
            id,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 1.0f, 0.1f } }
            });
        partial.AddComponent<Simulation::Memory>(id, Simulation::Memory{});

        const auto partialSnapshot = partial.Capture();

        if (partialSnapshot.Entities.size() != 1
            || partialSnapshot.Entities[0].Components.size() != 1)
        {
            return false;   // Needs persisted; Memory (no serializer) omitted
        }

        //---------------------------------------------------------------------
        // Clear resets the world; the registered serializers survive.
        //---------------------------------------------------------------------
        partial.Clear();

        if (partial.IsAlive(id))
        {
            return false;
        }

        // A cleared registry behaves like a fresh one: the next entity
        // may reuse slot 0 generation 1 (the old ID) — that is correct,
        // the old world is gone. What matters is that it lives and that
        // the serializers survived the clear.
        const auto fresh = partial.CreateEntity();

        if (!partial.IsAlive(fresh))
        {
            return false;
        }

        if (fresh != partial.Capture().Entities[0].Id)
        {
            return false;   // serializers still registered — capture works
        }

        //---------------------------------------------------------------------
        // After a restore, new entities reuse dead slots and never alias
        // the restored ones.
        //---------------------------------------------------------------------
        Simulation::EntityRegistry gap;
        gap.RegisterSerializer<Simulation::Needs>(MakeNeedsSerializer());

        const auto first = gap.CreateEntity();    // slot 0
        const auto middle = gap.CreateEntity();   // slot 1 — to be freed
        const auto last = gap.CreateEntity();     // slot 2

        gap.AddComponent<Simulation::Needs>(
            first,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 1.0f, 0.1f } }
            });
        gap.AddComponent<Simulation::Needs>(
            last,
            Simulation::Needs{
                { Simulation::Need{ Simulation::NeedType::Hunger, 1.0f, 0.1f } }
            });

        gap.DestroyEntity(middle);   // slot 1 is free again

        const auto gapSnapshot = gap.Capture();

        if (gapSnapshot.Entities.size() != 2)
        {
            return false;   // the gap is real: only slots 0 and 2 live
        }

        Simulation::EntityRegistry gapRestored;
        gapRestored.RegisterSerializer<Simulation::Needs>(MakeNeedsSerializer());
        gapRestored.Restore(gapSnapshot);

        const auto newcomer = gapRestored.CreateEntity();

        if (!gapRestored.IsAlive(first)
            || !gapRestored.IsAlive(last)
            || !gapRestored.IsAlive(newcomer))
        {
            return false;
        }

        if (newcomer == first || newcomer == last)
        {
            return false;   // a new entity must never alias a restored one
        }

        return true;
    }
}
