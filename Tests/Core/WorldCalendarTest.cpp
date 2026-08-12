//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      WorldCalendarTest.cpp
//
// Purpose:
//
//      Verifies the world calendar (0.5.0) — the substrate 0.7.0 Legacy
//      stands on: memories anchored to world time (day counter, seasons,
//      age of a fact) and the timestamp surviving a snapshot round trip.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Mind/Memory.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/Substrate/WorldTime.h"

#include <bit>
#include <cstddef>
#include <utility>

namespace LCE::Tests
{
    //-------------------------------------------------------------------------
    // A serializer that carries the Day anchor, proving the timestamp
    // survives a save/load round trip.
    //-------------------------------------------------------------------------
    namespace
    {
        void RegisterStampedMemorySerializer(
            Simulation::EntityRegistry& registry)
        {
            //-------------------------------------------------------------------------
            // A minimal little-endian codec for the test: count, then per
            // event (Other, Kind, Weight, Day) — Day is the world-time
            // anchor under test.
            //-------------------------------------------------------------------------
            const auto push = [](Simulation::ComponentBlob& blob, std::uint64_t value)
            {
                for (int i = 0; i < 8; ++i)
                {
                    blob.push_back(
                        static_cast<std::byte>((value >> (i * 8)) & 0xFF));
                }
            };

            const auto pull = [](const Simulation::ComponentBlob& blob,
                                 std::size_t offset)
            {
                std::uint64_t value = 0;

                for (int i = 0; i < 8; ++i)
                {
                    value |= static_cast<std::uint64_t>(
                        std::to_integer<unsigned char>(blob[offset + i]))
                        << (i * 8);
                }

                return value;
            };

            Simulation::ComponentSerializer<Simulation::Memory> serializer;

            serializer.Serialize =
                [push](const Simulation::Memory& memory)
                {
                    Simulation::ComponentBlob blob;

                    push(blob, memory.Events.size());

                    for (const auto& event : memory.Events)
                    {
                        push(blob, event.Other.Value());
                        push(blob, static_cast<std::uint64_t>(event.Kind));

                        const auto bits =
                            std::bit_cast<std::uint32_t>(event.Weight);
                        push(blob, bits);

                        push(blob, event.Day);
                    }

                    return blob;
                };

            serializer.Deserialize =
                [pull](const Simulation::ComponentBlob& blob)
                {
                    Simulation::Memory memory;

                    std::size_t offset = 0;

                    const auto count = pull(blob, offset);
                    offset += 8;

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        const auto other = Simulation::EntityId{ pull(blob, offset) };
                        offset += 8;

                        const auto kind =
                            static_cast<Simulation::InteractionKind>(pull(blob, offset));
                        offset += 8;

                        const auto weight =
                            std::bit_cast<float>(static_cast<std::uint32_t>(pull(blob, offset)));
                        offset += 8;

                        const auto day = pull(blob, offset);
                        offset += 8;

                        memory.Events.push_back(
                            Simulation::MemoryEvent{ other, kind, weight, day });
                    }

                    return memory;
                };

            registry.RegisterSerializer<Simulation::Memory>(std::move(serializer));
        }
    }

    bool WorldCalendarTest()
    {
        //-------------------------------------------------------------------------
        // 1. Remember stamps the world day when a WorldTime is passed.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            Simulation::Remember(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade, 1.0f },
                {}, Simulation::WorldTime{ 42 });

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            if (memory->Events[0].Day != 42)
            {
                return false;   // anchored to world day 42
            }
        }

        //-------------------------------------------------------------------------
        // 2. ReportOutcome stamps the outcome's memory with the day.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            Simulation::ReportOutcome(
                registry, farmer,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Success },
                {}, nullptr, Simulation::WorldTime{ 77 });

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            if (memory->Events[0].Day != 77)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 3. A caller-set day is preserved (no overwrite) — and the age of
        //    a fact is now.Day - event.Day.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto farmer = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            Simulation::MemoryEvent oldEvent{
                merchant, Simulation::InteractionKind::Trade, 1.0f };
            oldEvent.Day = 10;

            Simulation::Remember(
                registry, farmer, oldEvent, {}, Simulation::WorldTime{ 50 });

            const auto memory = registry.GetComponent<Simulation::Memory>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            if (memory->Events[0].Day != 10)
            {
                return false;   // the caller's stamp wins
            }

            // The fact is forty world days old.
            if (Simulation::WorldTime{ 50 }.Day - memory->Events[0].Day != 40)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 4. Seasons derive from the day: 90-day seasons, 360-day year.
        //-------------------------------------------------------------------------
        {
            using Simulation::Season;

            if (Simulation::SeasonOf(0) != Season::Spring
                || Simulation::SeasonOf(89) != Season::Spring
                || Simulation::SeasonOf(90) != Season::Summer
                || Simulation::SeasonOf(179) != Season::Summer
                || Simulation::SeasonOf(180) != Season::Autumn
                || Simulation::SeasonOf(269) != Season::Autumn
                || Simulation::SeasonOf(270) != Season::Winter
                || Simulation::SeasonOf(359) != Season::Winter
                || Simulation::SeasonOf(360) != Season::Spring)   // wraps the year
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. The timestamp survives a snapshot round trip — a save and a
        //    load keep "the raid was on day 42".
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry source;

            RegisterStampedMemorySerializer(source);

            const auto farmer = source.CreateEntity();
            const auto merchant = source.CreateEntity();

            Simulation::Remember(
                source, farmer,
                { merchant, Simulation::InteractionKind::Combat, 0.9f },
                {}, Simulation::WorldTime{ 42 });

            const auto snapshot = source.Capture();

            Simulation::EntityRegistry restored;

            RegisterStampedMemorySerializer(restored);

            restored.Restore(snapshot);

            const auto memory =
                restored.GetComponent<Simulation::Memory>(farmer);

            if (!memory || memory->Events.empty())
            {
                return false;
            }

            if (memory->Events[0].Day != 42)
            {
                return false;   // the anchor survived the round trip
            }
        }

        return true;
    }
}
