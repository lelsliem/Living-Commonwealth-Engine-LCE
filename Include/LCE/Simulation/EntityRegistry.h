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
// │          “If an entity disappears, it probably had somewhere better to be.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EntityRegistry.h
//
// Purpose:
//
//      Defines the registry that owns simulation entities and their
//      components. An entity is an EntityId; all data lives in components
//      attached through this registry.
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

#pragma once

#include "LCE/Simulation/EntityId.h"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // Detail
    //
    // The type-erased component stores. Two faces of erasure, as in the
    // Service Registry and EventBus:
    //   - IComponentStore is the uniform face: the registry can wipe an
    //     entity's components without knowing what they are.
    //   - ComponentStore<T> is the typed face: callers reach their
    //     components as the real type T.
    //-------------------------------------------------------------------------
    namespace Detail
    {
        class IComponentStore
        {
        public:
            virtual ~IComponentStore() = default;

            // Removes every component of this store's type that belongs to
            // the given entity. Called by DestroyEntity.
            virtual void RemoveEntity(EntityId id) = 0;
        };

        template <typename T>
        class ComponentStore final : public IComponentStore
        {
        public:
            void Set(EntityId id, std::shared_ptr<T> component)
            {
                m_Components[id] = std::move(component);
            }

            [[nodiscard]]
            std::shared_ptr<T> Get(EntityId id) const noexcept
            {
                const auto iterator = m_Components.find(id);

                if (iterator == m_Components.end())
                {
                    return {};
                }

                return iterator->second;
            }

            [[nodiscard]]
            bool Has(EntityId id) const noexcept
            {
                return m_Components.contains(id);
            }

            void Remove(EntityId id)
            {
                m_Components.erase(id);
            }

            void RemoveEntity(EntityId id) override
            {
                Remove(id);
            }

        private:
            std::unordered_map<EntityId, std::shared_ptr<T>> m_Components;
        };
    }

    class EntityRegistry
    {
    public:
        EntityRegistry() = default;
        ~EntityRegistry() = default;

        EntityRegistry(const EntityRegistry&) = delete;
        EntityRegistry& operator=(const EntityRegistry&) = delete;

        //-------------------------------------------------------------------------
        // Creates a new entity and returns its unique, valid ID.
        //-------------------------------------------------------------------------
        EntityId CreateEntity();

        //-------------------------------------------------------------------------
        // Destroys the entity: its slot becomes reusable and every component
        // it owns is dropped. A no-op for a dead or stale ID.
        //-------------------------------------------------------------------------
        void DestroyEntity(EntityId id);

        //-------------------------------------------------------------------------
        // Returns whether the ID names a live entity. Stale IDs (a slot that
        // has since been reused) return false — they can never alias.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        bool IsAlive(EntityId id) const;

        //-------------------------------------------------------------------------
        // Attaches a component of type T to the entity. The component is
        // stored by value internally. A no-op for a dead or stale ID.
        //-------------------------------------------------------------------------
        template <typename T>
        void AddComponent(EntityId id, T component);

        //-------------------------------------------------------------------------
        // Detaches the entity's component of type T, if any.
        //-------------------------------------------------------------------------
        template <typename T>
        void RemoveComponent(EntityId id);

        //-------------------------------------------------------------------------
        // Returns whether the entity has a component of type T.
        //-------------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        bool HasComponent(EntityId id) const;

        //-------------------------------------------------------------------------
        // Returns the entity's component of type T, or an empty pointer if
        // the entity has none (or is not alive).
        //-------------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        std::shared_ptr<T> GetComponent(EntityId id) const;

    private:
        struct Slot
        {
            std::uint32_t Generation = 0;
            bool Alive = false;
        };

        // Looks up the store for T without creating one. Returns nullptr if
        // no component of type T has ever been attached. The returned
        // pointer is mutable because components are intentionally mutable
        // through the registry.
        template <typename T>
        [[nodiscard]]
        Detail::ComponentStore<T>* FindStore() const noexcept
        {
            const auto iterator = m_Stores.find(std::type_index(typeid(T)));

            if (iterator == m_Stores.end())
            {
                return nullptr;
            }

            return static_cast<Detail::ComponentStore<T>*>(iterator->second.get());
        }

        // Finds the store for T, creating it on first use.
        template <typename T>
        Detail::ComponentStore<T>& GetStore()
        {
            const auto key = std::type_index(typeid(T));

            auto& store = m_Stores[key];

            if (!store)
            {
                store = std::make_shared<Detail::ComponentStore<T>>();
            }

            return static_cast<Detail::ComponentStore<T>&>(*store);
        }

        std::vector<Slot> m_Slots;
        std::vector<std::uint32_t> m_FreeIndices;
        std::unordered_map<
            std::type_index,
            std::shared_ptr<Detail::IComponentStore>> m_Stores;
    };

    //-------------------------------------------------------------------------
    // Implementation
    //
    // The component accessors are templates, so they live here in the header
    // — the same reasoning as the Service Registry. The bookkeeping
    // (CreateEntity, DestroyEntity, IsAlive) lives in EntityRegistry.cpp.
    //-------------------------------------------------------------------------

    template <typename T>
    void EntityRegistry::AddComponent(EntityId id, T component)
    {
        if (!IsAlive(id))
        {
            return;
        }

        GetStore<T>().Set(id, std::make_shared<T>(std::move(component)));
    }

    template <typename T>
    void EntityRegistry::RemoveComponent(EntityId id)
    {
        auto store = FindStore<T>();

        if (store != nullptr)
        {
            store->Remove(id);
        }
    }

    template <typename T>
    bool EntityRegistry::HasComponent(EntityId id) const
    {
        const auto store = FindStore<T>();

        return store != nullptr && store->Has(id);
    }

    template <typename T>
    std::shared_ptr<T> EntityRegistry::GetComponent(EntityId id) const
    {
        const auto store = FindStore<T>();

        if (store == nullptr)
        {
            return {};
        }

        return store->Get(id);
    }
}
