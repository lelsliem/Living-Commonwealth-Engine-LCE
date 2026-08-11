//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      WorldLegacyTest.cpp
//
// Purpose:
//
//      Verifies the legacy-as-world-fact stone (0.7.0 stone 12):
//      LeaveLegacy / ReadLegacy / ForgetLegacy — the promise that
//      outlives its maker. A registry-level store: permanent until the
//      world deletes, riding the co-save through the world's registered
//      serializer, wiped by Clear() while the serializer survives. And
//      the seam: the world reads a legacy and teaches it as a memory.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace LCE::Tests
{
    namespace
    {
        //-------------------------------------------------------------------------
        // A trivial codec for the legacy name map — the world's job in
        // the game, a helper here. Layout: count, then per fact: name
        // (length + bytes), owner value, day, weight. All fields are
        // 8-byte little-endian; the deserialize strides by shift/8, the
        // mirror of the serialize's bit shifts.
        //-------------------------------------------------------------------------
        Simulation::ComponentSerializer<
            std::unordered_map<std::string, Simulation::LegacyFact>>
        LegacyCodec()
        {
            return {
                [](const std::unordered_map<std::string, Simulation::LegacyFact>& facts)
                {
                    std::vector<std::byte> bytes;

                    const auto push = [&bytes](std::uint64_t value)
                    {
                        for (int shift = 0; shift < 64; shift += 8)
                        {
                            bytes.push_back(
                                static_cast<std::byte>((value >> shift) & 0xFF));
                        }
                    };

                    const auto pushFloat = [&push](float value)
                    {
                        std::uint32_t bits = 0;
                        std::memcpy(&bits, &value, sizeof(bits));
                        push(bits);
                    };

                    const auto pushString = [&bytes, &push](const std::string& value)
                    {
                        push(value.size());

                        for (const char c : value)
                        {
                            bytes.push_back(static_cast<std::byte>(c));
                        }
                    };

                    push(facts.size());

                    for (const auto& [name, fact] : facts)
                    {
                        pushString(name);
                        push(fact.Owner.Value());
                        push(fact.Day);
                        pushFloat(fact.Weight);
                    }

                    return bytes;
                },
                [](const Simulation::ComponentBlob& bytes)
                {
                    std::unordered_map<std::string, Simulation::LegacyFact> facts;
                    std::size_t at = 0;

                    const auto take = [&at, &bytes]() -> std::uint64_t
                    {
                        std::uint64_t value = 0;

                        // The mirror of push: shift/8 steps through the
                        // byte sequence while the shift packs the bits.
                        for (int shift = 0; shift < 64; shift += 8)
                        {
                            value |= static_cast<std::uint64_t>(
                                bytes[at + shift / 8])
                                << shift;
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

                    const auto takeString =
                        [&at, &bytes, &take]() -> std::string
                    {
                        const auto size = take();
                        std::string value(size, '\0');

                        for (std::size_t i = 0; i < size; ++i)
                        {
                            value[i] = static_cast<char>(bytes[at + i]);
                        }

                        at += size;
                        return value;
                    };

                    const auto count = take();

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        Simulation::LegacyFact fact;
                        const auto name = takeString();

                        fact.Owner = Simulation::EntityId{ take() };
                        fact.Day = take();
                        fact.Weight = takeFloat();
                        fact.Name = name;

                        facts[name] = std::move(fact);
                    }

                    return facts;
                }};
        }
    }

    bool WorldLegacyTest()
    {
        //-------------------------------------------------------------------------
        // 1. The promise outlives its maker: a legacy remains after the
        //    owner's DestroyEntity — permanent, day-stamped, weight kept.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto miller = registry.CreateEntity();

            registry.LeaveLegacy(
                Simulation::LegacyFact{ miller, 900, "the miller's pledge", 1.0f });

            registry.DestroyEntity(miller);

            if (registry.IsAlive(miller))
            {
                return false;
            }

            const auto pledge =
                registry.ReadLegacy("the miller's pledge");

            if (!pledge || pledge->Day != 900 || pledge->Weight != 1.0f
                || pledge->Owner != miller)
            {
                return false;   // the promise outlived the voice
            }
        }

        //-------------------------------------------------------------------------
        // 2. The world's explicit decay: Read misses, Forget retires.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            if (registry.ReadLegacy("nothing here"))
            {
                return false;
            }

            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 1, "the bridge is mended", 1.0f });

            registry.ForgetLegacy("the bridge is mended");

            if (registry.ReadLegacy("the bridge is mended"))
            {
                return false;   // fulfilled — retired
            }
        }

        //-------------------------------------------------------------------------
        // 3. Named and independent: two legacies coexist; the same name
        //    replaces (the world's record keeps its latest word).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 1, "the miller's pledge", 1.0f });
            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 2, "the baker's debt", 0.8f });
            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 3, "the miller's pledge", 0.5f });

            const auto pledge = registry.ReadLegacy("the miller's pledge");
            const auto debt = registry.ReadLegacy("the baker's debt");

            if (!pledge || pledge->Weight != 0.5f || pledge->Day != 3)
            {
                return false;   // the latest word stands
            }

            if (!debt || debt->Day != 2)
            {
                return false;   // the other record is untouched
            }
        }

        //-------------------------------------------------------------------------
        // 4. Save/load: the legacy rides the co-save — captured, restored,
        //    intact (the registry-level snapshot section, schema v2).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            registry.RegisterLegacySerializer(LegacyCodec());

            const auto miller = registry.CreateEntity();

            registry.AddComponent<Simulation::Memory>(
                miller,
                Simulation::Memory{
                    { { {}, Simulation::InteractionKind::WeatherRain, 1.0f, 12 } } });
            registry.LeaveLegacy(
                Simulation::LegacyFact{ miller, 900, "the miller's pledge", 1.0f });

            const auto snapshot = registry.Capture();

            if (!snapshot.Legacy)
            {
                return false;   // the section exists in the schema
            }

            Simulation::EntityRegistry reloaded;
            reloaded.RegisterLegacySerializer(LegacyCodec());
            reloaded.Restore(snapshot);

            const auto pledge = reloaded.ReadLegacy("the miller's pledge");

            if (!pledge || pledge->Day != 900 || pledge->Owner != miller)
            {
                return false;   // the promise survived the save
            }
        }

        //-------------------------------------------------------------------------
        // 5. A fresh world: Clear wipes legacies — but the serializer
        //    survives, so the next game's legacies ride the co-save again.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            registry.RegisterLegacySerializer(LegacyCodec());
            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 1, "the miller's pledge", 1.0f });

            registry.Clear();

            if (registry.ReadLegacy("the miller's pledge"))
            {
                return false;   // the world died with the world
            }

            // No re-registration: the serializer is still there.
            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 2, "the baker's debt", 0.8f });

            const auto snapshot = registry.Capture();

            if (!snapshot.Legacy)
            {
                return false;   // the new record persists again
            }
        }

        //-------------------------------------------------------------------------
        // 6. The seam: the world reads a legacy and teaches it — the
        //    mind reaches the bridge, hears the pledge, and remembers it
        //    as a world fact (invalid Other, the adapter's call, not the
        //    core's).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto mind = registry.CreateEntity();

            registry.LeaveLegacy(Simulation::LegacyFact{
                {}, 900, "the miller's pledge", 1.0f });

            const auto pledge = registry.ReadLegacy("the miller's pledge");

            if (!pledge)
            {
                return false;
            }

            Simulation::Remember(
                registry, mind,
                { {}, Simulation::InteractionKind::Social,
                  pledge->Weight, pledge->Day });

            const auto memory = registry.GetComponent<Simulation::Memory>(mind);

            if (!memory || memory->Events.size() != 1
                || memory->Events[0].Day != 900
                || memory->Events[0].Weight != 1.0f)
            {
                return false;   // the promise became a mind's memory
            }
        }

        return true;
    }
}
