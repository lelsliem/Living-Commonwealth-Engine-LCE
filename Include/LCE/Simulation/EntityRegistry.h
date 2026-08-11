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
#include "LCE/Simulation/Legacy.h"
#include "LCE/Simulation/RegistrySnapshot.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace LCE::Events
{
    class EventBus;   // forward declaration — the registry holds a sink pointer
}

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

            // Appends the entity's component of this store's type as a
            // blob. Returns false when the entity has no such component or
            // no serializer is registered — that type is simply not part
            // of the snapshot.
            virtual bool Serialize(EntityId id, ComponentBlob& out) const = 0;

            // Replaces the entity's component with one deserialized from
            // the blob. Requires the serializer registered at capture time.
            virtual void Deserialize(EntityId id, const ComponentBlob& blob) = 0;
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

            // The adapter registers how this type becomes bytes and back.
            void SetSerializer(ComponentSerializer<T> serializer)
            {
                m_Serializer = std::move(serializer);
            }

            bool Serialize(EntityId id, ComponentBlob& out) const override
            {
                const auto iterator = m_Components.find(id);

                if (iterator == m_Components.end() || !m_Serializer)
                {
                    return false;
                }

                out = m_Serializer->Serialize(*iterator->second);

                return true;
            }

            void Deserialize(EntityId id, const ComponentBlob& blob) override
            {
                if (!m_Serializer)
                {
                    return;
                }

                m_Components[id] =
                    std::make_shared<T>(m_Serializer->Deserialize(blob));
            }

            // Visits every (entity, component) pair in this store. Exposed
            // through EntityRegistry::ForEachWithComponent — how systems
            // find their subjects.
            template <typename F>
            void ForEach(F&& function)
            {
                for (auto& [id, component] : m_Components)
                {
                    function(id, *component);
                }
            }

        private:
            std::optional<ComponentSerializer<T>> m_Serializer;
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
        // Attaches an event bus the registry publishes observation events
        // to (0.5.0). When set, CreateEntity publishes EntityCreatedEvent
        // for every genuinely new entity. Snapshot restore uses a private
        // path and does not publish — a loaded world is not a creation
        // flood. The bus is an input, never global state (ADR-0014).
        //-------------------------------------------------------------------------
        void SetEventSink(LCE::Events::EventBus* bus) noexcept
        {
            m_EventSink = bus;
        }

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

        //-------------------------------------------------------------------------
        // Visits every entity that has a component of type T, calling
        // function(EntityId, T&) for each. Lets systems sweep their
        // subjects without knowing which entities have the component.
        //
        // Do not modify this registry's component stores of type T inside
        // the callback — same iterator-invalidation rule as everywhere
        // else in LCE. Collect first, apply after.
        //-------------------------------------------------------------------------
        template <typename T, typename F>
        void ForEachWithComponent(F&& function)
        {
            auto store = FindStore<T>();

            if (store == nullptr)
            {
                return;
            }

            store->ForEach(std::forward<F>(function));
        }

        //-------------------------------------------------------------------------
        // The query surface (0.5.0). Returns every entity that has a
        // component of type T AND whose component satisfies the predicate,
        // in deterministic order: ascending EntityId::Value(). The
        // underlying store is unordered — it promises nothing about order;
        // the query promises everything, so the same query returns the
        // same result on every run (the determinism hook seeded RNG and
        // save-compat stand on).
        //
        // The predicate receives (EntityId, const T&) so cross-component
        // filters can reach the registry by capturing it — "settlers who
        // remember the raid" is a query over Memory whose predicate also
        // checks Needs. The component is const: a query reads, never
        // mutates. An empty store or no match yields an empty vector.
        //-------------------------------------------------------------------------
        template <typename T, typename Pred>
        [[nodiscard]]
        std::vector<EntityId> QueryWhere(Pred&& predicate) const
        {
            std::vector<EntityId> ids;

            const auto store = FindStore<T>();

            if (store == nullptr)
            {
                return ids;
            }

            store->ForEach(
                [&predicate, &ids](EntityId id, const T& component)
                {
                    if (predicate(id, component))
                    {
                        ids.push_back(id);
                    }
                });

            std::sort(
                ids.begin(),
                ids.end(),
                [](EntityId a, EntityId b)
                {
                    return a.Value() < b.Value();
                });

            return ids;
        }

        //-------------------------------------------------------------------------
        // Registers how component type T becomes bytes and back. Required
        // for T to appear in a snapshot — a component type with no
        // serializer is simply not persisted. Register at init, once; the
        // registration survives Clear() so Restore always has it.
        //-------------------------------------------------------------------------
        template <typename T>
        void RegisterSerializer(ComponentSerializer<T> serializer)
        {
            GetStore<T>().SetSerializer(std::move(serializer));
        }

        //-------------------------------------------------------------------------
        // Captures every live entity and its serializable components as
        // pure data. Types without a registered serializer are omitted.
        //-------------------------------------------------------------------------
        [[nodiscard]]
        RegistrySnapshot Capture() const;

        //-------------------------------------------------------------------------
        // Clears the registry and rebuilds it from the snapshot, preserving
        // entity IDs exactly (index + generation). Requires the same
        // serializers to be registered as when the snapshot was captured.
        //-------------------------------------------------------------------------
        void Restore(const RegistrySnapshot& snapshot);

        //-------------------------------------------------------------------------
        // Destroys every entity and resets the registry to blank. Stores
        // (and their serializers) survive — they are registered once at
        // init and must remain for the next game's Restore.
        //-------------------------------------------------------------------------
        void Clear();

        //-------------------------------------------------------------------------
        // The legacy store (0.7.0 stone 12) — the promise that outlives
        // its maker. Registry-level: facts keyed by name, permanent
        // until the world forgets them. LeaveLegacy/ReadLegacy/
        // ForgetLegacy; RegisterLegacySerializer rides the co-save
        // (the same serializer contract as the component stores).
        //-------------------------------------------------------------------------
        void LeaveLegacy(LegacyFact fact)
        {
            m_Legacy.Leave(std::move(fact));
        }

        [[nodiscard]]
        std::optional<LegacyFact> ReadLegacy(std::string_view name) const
        {
            return m_Legacy.Read(name);
        }

        void ForgetLegacy(std::string_view name)
        {
            m_Legacy.Forget(name);
        }

        void RegisterLegacySerializer(
            ComponentSerializer<std::unordered_map<std::string, LegacyFact>> serializer)
        {
            m_Legacy.SetSerializer(std::move(serializer));
        }

    private:
        struct Slot
        {
            std::uint32_t Generation = 0;
            bool Alive = false;
        };

        LCE::Events::EventBus* m_EventSink = nullptr;

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

        // Destroys every live entity, leaving stores (and serializers)
        // intact. Shared by Clear and Restore.
        void DestroyAllEntities();

        // Makes a slot live with the exact ID from a snapshot, growing the
        // slot array and clearing the slot from the free list as needed.
        void Materialize(EntityId id);

        std::vector<Slot> m_Slots;
        std::vector<std::uint32_t> m_FreeIndices;
        LegacyStore m_Legacy;
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
