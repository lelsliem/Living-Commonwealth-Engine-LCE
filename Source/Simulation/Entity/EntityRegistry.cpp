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
// │        “If an entity acts strange, that’s not a bug. That’s personality.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EntityRegistry.cpp
//
// Purpose:
//
//      Implements the Entity Registry's bookkeeping: creating, destroying,
//      and testing the life of entities. Component access lives in the
//      header (templates).
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

#include "LCE/Simulation/Entity/EntityRegistry.h"

#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/SimulationEvents.h"

namespace LCE::Simulation
{
    EntityId EntityRegistry::CreateEntity()
    {
        EntityId id{};

        // Prefer a reused slot: destroying an entity pushes its index onto
        // the free list, and memory stays bounded because slots are never
        // forgotten. Reuse bumps the generation, so old IDs for the slot
        // become stale instead of aliasing the new entity.
        if (!m_FreeIndices.empty())
        {
            const auto index = m_FreeIndices.back();
            m_FreeIndices.pop_back();

            auto& slot = m_Slots[index];

            ++slot.Generation;

            // Generation 0 would collide with the invalid sentinel when the
            // index is also 0 — never allow the wrap.
            if (slot.Generation == 0)
            {
                slot.Generation = 1;
            }

            slot.Alive = true;

            id = EntityId::Make(index, slot.Generation);
        }
        else
        {
            // No free slot: grow the slot array. Generations start at 1 so
            // the invalid sentinel (value 0) can never be a live entity.
            const auto index = static_cast<std::uint32_t>(m_Slots.size());

            m_Slots.push_back({ 1, true });

            id = EntityId::Make(index, 1);
        }

        // Observation (0.5.0): a genuinely new entity is news. Restore and
        // Clear use private paths and never publish — a loaded world is not
        // a creation flood.
        if (m_EventSink != nullptr)
        {
            m_EventSink->Publish(EntityCreatedEvent{ id });
        }

        return id;
    }

    void EntityRegistry::DestroyEntity(EntityId id)
    {
        const auto index = id.Index();

        if (index >= m_Slots.size())
        {
            return;
        }

        auto& slot = m_Slots[index];

        if (!slot.Alive || slot.Generation != id.Generation())
        {
            return;   // already dead, or a stale ID — nothing to destroy
        }

        slot.Alive = false;

        m_FreeIndices.push_back(index);

        // Drop every component the entity owns without knowing what those
        // components are: each type-erased store cleans up its own type.
        for (const auto& entry : m_Stores)
        {
            entry.second->RemoveEntity(id);
        }
    }

    bool EntityRegistry::IsAlive(EntityId id) const
    {
        const auto index = id.Index();

        if (index >= m_Slots.size())
        {
            return false;
        }

        const auto& slot = m_Slots[index];

        // Alive, AND the slot's generation still matches the ID's. A stale
        // ID carries an old generation and fails this check even though the
        // slot may have been reused by a brand-new entity.
        return slot.Alive && slot.Generation == id.Generation();
    }

    void EntityRegistry::DestroyAllEntities()
    {
        for (std::size_t index = 0; index < m_Slots.size(); ++index)
        {
            if (m_Slots[index].Alive)
            {
                DestroyEntity(EntityId::Make(
                    static_cast<std::uint32_t>(index),
                    m_Slots[index].Generation));
            }
        }
    }

    void EntityRegistry::Materialize(EntityId id)
    {
        const auto index = id.Index();

        // Grow the slot array so the index exists. New slots are dead and
        // free — exactly as CreateEntity leaves the tail of the array.
        while (m_Slots.size() <= index)
        {
            const auto newIndex = static_cast<std::uint32_t>(m_Slots.size());

            m_Slots.push_back({ 0, false });
            m_FreeIndices.push_back(newIndex);
        }

        // The slot is about to be alive: take it off the free list. The
        // list is a stack of reusable indices; remove this one occurrence.
        for (auto iterator = m_FreeIndices.begin();
             iterator != m_FreeIndices.end();
             ++iterator)
        {
            if (*iterator == index)
            {
                m_FreeIndices.erase(iterator);
                break;
            }
        }

        auto& slot = m_Slots[index];

        // Adopt the snapshot's generation so the ID means what it meant
        // when captured — stale references can never alias a restored
        // entity any more than a live one.
        slot.Generation = id.Generation();
        slot.Alive = true;
    }

    RegistrySnapshot EntityRegistry::Capture() const
    {
        RegistrySnapshot snapshot;
        snapshot.Version = kSnapshotVersion;

        for (std::size_t index = 0; index < m_Slots.size(); ++index)
        {
            const auto& slot = m_Slots[index];

            if (!slot.Alive)
            {
                continue;
            }

            SnapshotEntity entity;
            entity.Id = EntityId::Make(
                static_cast<std::uint32_t>(index),
                slot.Generation);

            // Every store that can serialize this entity contributes one
            // blob. Types without a registered serializer are skipped —
            // data presence decides membership, as everywhere in LCE.
            for (const auto& [type, store] : m_Stores)
            {
                ComponentBlob blob;

                if (store->Serialize(entity.Id, blob))
                {
                    entity.Components.push_back(
                        { type, std::move(blob) });
                }
            }

            snapshot.Entities.push_back(std::move(entity));
        }

        // The legacy store rides the co-save too (0.7.0 stone 12): the
        // promise that outlives its maker must survive a save and a
        // load. Absent when empty or unserialized — like a component
        // type with no serializer, it simply is not part of the record.
        if (const auto legacy = m_Legacy.Serialize())
        {
            snapshot.Legacy = *legacy;
        }

        return snapshot;
    }

    void EntityRegistry::Restore(const RegistrySnapshot& snapshot)
    {
        DestroyAllEntities();

        for (const auto& entity : snapshot.Entities)
        {
            if (!entity.Id.IsValid())
            {
                continue;
            }

            Materialize(entity.Id);

            for (const auto& component : entity.Components)
            {
                const auto iterator = m_Stores.find(component.Type);

                if (iterator == m_Stores.end())
                {
                    continue;   // no serializer registered for this type
                }

                iterator->second->Deserialize(entity.Id, component.Data);
            }
        }

        // Legacy (0.7.0 stone 12): the world's records return too. A
        // snapshot taken before the section existed simply starts with
        // none.
        if (snapshot.Legacy)
        {
            m_Legacy.Deserialize(*snapshot.Legacy);
        }
    }

    void EntityRegistry::Clear()
    {
        DestroyAllEntities();

        // A blank registry: no slots, no free list. The stores survive —
        // their serializers are registered once at init and must remain
        // for the next game's Restore.
        m_Slots.clear();
        m_FreeIndices.clear();

        // The world's records die with the world, not with the entities.
        m_Legacy.Clear();
    }
}
