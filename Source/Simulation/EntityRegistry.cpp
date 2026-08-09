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

#include "LCE/Simulation/EntityRegistry.h"

namespace LCE::Simulation
{
    EntityId EntityRegistry::CreateEntity()
    {
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

            return EntityId::Make(index, slot.Generation);
        }

        // No free slot: grow the slot array. Generations start at 1 so the
        // invalid sentinel (value 0) can never be a live entity.
        const auto index = static_cast<std::uint32_t>(m_Slots.size());

        m_Slots.push_back({ 1, true });

        return EntityId::Make(index, 1);
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
}
