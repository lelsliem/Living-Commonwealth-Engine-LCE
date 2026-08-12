//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      TraitsTest.cpp
//
// Purpose:
//
//      Verifies the personality substrate (0.6.0 stone 09): JitteredTraits
//      turns a base template into per-entity variation — identical base
//      traits, different individuals — deterministically under a seed,
//      with a stable no-Rng fallback, a zero-spread identity, and
//      snapshot persistence like any component. The influence of a trait
//      is the world's business; this suite proves the substrate.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Entity/EntityRegistry.h"
#include "LCE/Simulation/Substrate/Rng.h"
#include "LCE/Simulation/Society/Traits.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace LCE::Tests
{
    namespace
    {
        //-------------------------------------------------------------------------
        // A tiny little-endian codec proving Traits round-trips through a
        // snapshot like any other component.
        //-------------------------------------------------------------------------
        void RegisterTraitSerializer(Simulation::EntityRegistry& registry)
        {
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

            Simulation::ComponentSerializer<Simulation::Traits> serializer;

            serializer.Serialize =
                [push](const Simulation::Traits& traits)
                {
                    Simulation::ComponentBlob blob;

                    push(blob, traits.List.size());

                    for (const auto& trait : traits.List)
                    {
                        push(blob, trait.Name.size());

                        for (const auto character : trait.Name)
                        {
                            blob.push_back(static_cast<std::byte>(character));
                        }

                        push(blob, std::bit_cast<std::uint32_t>(trait.Value));
                    }

                    return blob;
                };

            serializer.Deserialize =
                [pull](const Simulation::ComponentBlob& blob)
                {
                    Simulation::Traits traits;

                    std::size_t offset = 0;

                    const auto count = pull(blob, offset);
                    offset += 8;

                    for (std::uint64_t i = 0; i < count; ++i)
                    {
                        const auto length = pull(blob, offset);
                        offset += 8;

                        std::string name;
                        name.reserve(length);

                        for (std::uint64_t j = 0; j < length; ++j)
                        {
                            name.push_back(
                                std::to_integer<char>(blob[offset + j]));
                        }

                        offset += length;

                        const auto value = std::bit_cast<float>(
                            static_cast<std::uint32_t>(pull(blob, offset)));
                        offset += 8;

                        traits.List.push_back(
                            Simulation::TraitValue{ std::move(name), value });
                    }

                    return traits;
                };

            registry.RegisterSerializer<Simulation::Traits>(std::move(serializer));
        }
    }

    bool TraitsTest()
    {
        const Simulation::Traits base{
            { { "boldness", 0.5f }, { "sociability", 0.5f } }
        };

        //-------------------------------------------------------------------------
        // 1. Divergence: identical base traits, two entities, one seed —
        //    different individuals.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng rng{ 42 };

            const auto first =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 1 }, &rng);
            const auto second =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 2 }, &rng);

            if (first.List.size() != 2 || second.List.size() != 2)
            {
                return false;
            }

            if (first.List[0].Value == second.List[0].Value
                && first.List[1].Value == second.List[1].Value)
            {
                return false;   // same seed, different tree
            }
        }

        //-------------------------------------------------------------------------
        // 2. Determinism: the same entity under the same seed is
        //    bit-identical — the save resumes the exact same personalities.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng firstRng{ 42 };
            Simulation::Rng secondRng{ 42 };

            const auto first =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 7 }, &firstRng);
            const auto second =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 7 }, &secondRng);

            if (first.List[0].Value != second.List[0].Value
                || first.List[1].Value != second.List[1].Value)
            {
                return false;   // same seed + same entity = same mind
            }
        }

        //-------------------------------------------------------------------------
        // 3. The no-Rng fallback: still per-entity and per-trait, still
        //    stable for the same entity — and the parent Rng (when given)
        //    never advances.
        //-------------------------------------------------------------------------
        {
            const auto first =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 3 }, nullptr);
            const auto second =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 4 }, nullptr);

            if (first.List[0].Value == first.List[1].Value)
            {
                return false;   // traits within one entity differ too
            }

            if (first.List[0].Value == second.List[0].Value)
            {
                return false;   // and entities differ from each other
            }

            const auto again =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 3 }, nullptr);

            if (again.List[0].Value != first.List[0].Value)
            {
                return false;   // the fallback is deterministic
            }

            Simulation::Rng rng{ 1 };
            const auto before = rng.State();

            (void)Simulation::JitteredTraits(base, Simulation::EntityId{ 9 }, &rng);

            if (rng.State() != before)
            {
                return false;   // Derive never advances the parent stream
            }
        }

        //-------------------------------------------------------------------------
        // 4. Zero spread is the identity: the base template comes back
        //    exactly — a world that wants no personality variation can
        //    have none.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng rng{ 7 };

            const auto exact =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 5 }, &rng, 0.0f);

            if (exact.List[0].Value != base.List[0].Value
                || exact.List[1].Value != base.List[1].Value)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 5. The component survives a snapshot round trip — a saved
        //    personality restores exactly.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry source;

            RegisterTraitSerializer(source);

            const auto farmer = source.CreateEntity();

            Simulation::Rng rng{ 42 };

            source.AddComponent<Simulation::Traits>(
                farmer,
                Simulation::JitteredTraits(base, farmer, &rng));

            const auto snapshot = source.Capture();

            Simulation::EntityRegistry restored;

            RegisterTraitSerializer(restored);

            restored.Restore(snapshot);

            const auto traits = restored.GetComponent<Simulation::Traits>(farmer);

            if (!traits || traits->List.size() != 2)
            {
                return false;
            }

            const auto original =
                source.GetComponent<Simulation::Traits>(farmer);

            if (traits->List[0].Name != original->List[0].Name
                || traits->List[0].Value != original->List[0].Value
                || traits->List[1].Name != original->List[1].Name
                || traits->List[1].Value != original->List[1].Value)
            {
                return false;   // the personality survived the round trip
            }
        }

        //-------------------------------------------------------------------------
        // 6. Names are carried: the variation changes values, never the
        //    world's vocabulary.
        //-------------------------------------------------------------------------
        {
            Simulation::Rng rng{ 42 };

            const auto varied =
                Simulation::JitteredTraits(base, Simulation::EntityId{ 11 }, &rng);

            if (varied.List[0].Name != "boldness"
                || varied.List[1].Name != "sociability")
            {
                return false;
            }
        }

        return true;
    }
}
