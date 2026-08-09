# Entity System

**Milestone:** 0.2.0 — Alpha · Entity System
**Status:** Implemented and tested
**Related ADRs:** ADR-0014 (No Global State), ADR-0015 (Dependency Injection),
ADR-0019 (Layered Dependency Rule), ADR-0034 (Type-erased containers)

---

## Purpose

The first stone of the simulation layer. The Entity System answers four
questions from the roadmap:

1. What is an entity? — an **EntityId**
2. What lives on an entity? — **Components**
3. Who owns entities? — the **EntityRegistry**
4. What happens when an entity dies? — **Lifetime Management**

---

## The Big Idea: An Entity Is an ID, Not an Object

In classic object-oriented thinking, a "farmer" would be a `Farmer` class
holding hunger, home, relationships. That is a single object that must know
about everything.

LCE's entity system is the opposite: **an entity is nothing but an
identifier**. All data lives in *components* — small, focused types like
`Health`, `Position`, `Name` — that get attached to the entity by the
registry. A farmer isn't a class; a farmer is an ID with `Health`,
`Hunger`, `Home`, and `Relationship` components attached.

Why this shape (it is not just fashion):

- **Composition beats inheritance.** Adding a behaviour is attaching a
  component, not redesigning a class hierarchy. A dog and a settler can
  share a `Hunger` component without a common ancestor.
- **The core never knows the game.** `LCE::Simulation` knows what an ID and
  a component are — not what a farmer is. The Fallout 4 adapter (0.4.0)
  decides which components a settler has.
- **It is how the data gets dense.** When every `Health` component lives in
  one place, systems can sweep all of them in one cache-friendly pass.
  (0.2.0 keeps storage simple; density is a later performance stone.)

---

## EntityId — a Tagged Type

```cpp
namespace LCE::Simulation
{
    class EntityId
    {
    public:
        using ValueType = std::uint64_t;

        constexpr EntityId() = default;                 // invalid
        constexpr explicit EntityId(ValueType value);   // from raw value

        [[nodiscard]] constexpr ValueType Value() const noexcept;
        [[nodiscard]] constexpr bool IsValid() const noexcept;

        friend constexpr bool operator==(EntityId, EntityId) = default;
        friend constexpr bool operator!=(EntityId, EntityId) = default;

        static constexpr ValueType InvalidValue = 0;

    private:
        ValueType m_Value = InvalidValue;
    };
}
```

### Why a tagged type and not a plain `uint64_t`

A raw integer can be anything: a count, an index, an ID. The compiler
cannot tell them apart, so `DestroyEntity(healthPoints)` compiles. A tagged
type wraps the value in a named type — `DestroyEntity(EntityId{...})` —
so a wrong argument is a **compile error**, not a runtime surprise.

This is the same lesson as the Service Registry: type safety is not a
feature of the language, it is a habit of the types you define.

### Why index + generation packed into one value

The 64-bit value stores **two numbers**: a 32-bit *index* (which slot in the
registry this entity occupies) and a 32-bit *generation* (how many times
that slot has been reused).

- **Index** tells the registry where the entity lives — O(1) lookup.
- **Generation** makes destroyed-and-reused slots safe: a stale ID from two
  generations ago no longer matches the slot's current generation, so
  holding an old `EntityId` can never silently point at a new entity.

`Value = (generation << 32) | index`. `0` is reserved as the invalid ID:
a live entity always has generation ≥ 1, so value 0 is never alive.

---

## EntityRegistry — the Owner

```cpp
namespace LCE::Simulation
{
    class EntityRegistry
    {
    public:
        EntityRegistry() = default;
        ~EntityRegistry() = default;

        EntityRegistry(const EntityRegistry&) = delete;
        EntityRegistry& operator=(const EntityRegistry&) = delete;

        EntityId CreateEntity();
        void DestroyEntity(EntityId id);
        [[nodiscard]] bool IsAlive(EntityId id) const;

        template <typename T>
        void AddComponent(EntityId id, T component);

        template <typename T>
        void RemoveComponent(EntityId id);

        template <typename T>
        [[nodiscard]] bool HasComponent(EntityId id) const;

        template <typename T>
        [[nodiscard]] std::shared_ptr<T> GetComponent(EntityId id) const;

    private:
        struct Slot
        {
            std::uint32_t Generation = 0;
            bool Alive = false;
        };

        std::vector<Slot> m_Slots;
        std::vector<std::uint32_t> m_FreeIndices;
        std::unordered_map<std::type_index,
            std::shared_ptr<Detail::IComponentStore>> m_Stores;
    };
}
```

### Internals, explained

- **`m_Slots`** — one `Slot` per possible entity, indexed by the entity's
  index. O(1) create, destroy, and alive-check.
- **`m_FreeIndices`** — a *free list*. When an entity is destroyed, its
  index is pushed here instead of being forgotten. The next `CreateEntity`
  pops a free index and **bumps its generation** — the slot is reused, but
  old IDs for it are now stale. Reuse keeps memory bounded and the slot
  array compact.
- **`m_Stores`** — one type-erased component store per component type. This
  is the Service Registry pattern (ADR-0034) applied to components: the
  registry holds `IComponentStore` behind a `std::type_index` key, and the
  templates `AddComponent<T>`/`GetComponent<T>` find or create the right
  store for `T`.

### Why an erased `IComponentStore` interface

The registry must be able to destroy *all* of an entity's components in one
place, without knowing what those components are:

```cpp
// Detail — internal, lives in EntityRegistry.h
class IComponentStore
{
public:
    virtual ~IComponentStore() = default;
    virtual void RemoveEntity(EntityId id) = 0;
};

template <typename T>
class ComponentStore final : public IComponentStore
{
    std::unordered_map<EntityId, std::shared_ptr<T>> m_Components;
    // Set / Get / Has / Remove, plus RemoveEntity(id)
};
```

`DestroyEntity` walks `m_Stores` and calls `RemoveEntity(id)` on each —
one loop, no knowledge of concrete component types. The virtual interface
is the second face of type erasure: the template gives the registry *typed*
access per component, the base class gives it *uniform* access across all
components.

### Component semantics

- `AddComponent<T>(id, T component)` takes the component **by value** and
  stores it as `std::shared_ptr<T>` internally — the caller writes
  `registry.AddComponent<Health>(id, Health{100})`, no manual allocation.
- `GetComponent<T>(id)` returns `std::shared_ptr<T>`, or an empty pointer
  when the entity lacks that component — the same "empty pointer means
  absent" contract as the Service Registry.
- Component storage is deliberately simple for 0.2.0 (one map per type,
  one heap object per component). The cache-dense, value-stored component
  arrays are a later performance stone — noted, not built.

### Lifetime rules

- `CreateEntity` → returns a valid, unique `EntityId`.
- `DestroyEntity(id)` → no-op if the ID is not alive; otherwise the slot is
  freed, its generation is prepared for reuse, and every component store
  drops the entity's data.
- Holding an `EntityId` past destruction is safe: `IsAlive` returns false,
  and component access on a dead ID is a no-op. Stale IDs can never alias.

---

## Dependencies

- **Upward:** nothing.
- **Downward:** standard C++ only (`<vector>`, `<unordered_map>`,
  `<typeindex>`, `<memory>`, `<cstdint>`, `<utility>`).
- **Third-party:** none.

Namespace: `LCE::Simulation` — the first stone of the simulation layer,
matching the README's "Next" table and the existing `Include/LCE/Simulation`
folder.

---

## Test Plan

| Case | Expectation |
|------|-------------|
| `CreateEntity` returns a valid, non-zero ID | `IsValid()` true, value ≠ 0. |
| Two creations return distinct IDs | IDs differ. |
| `IsAlive` on a fresh ID | `true`. |
| Default-constructed `EntityId` | `IsValid()` false, never alive. |
| `DestroyEntity` then `IsAlive` | `false`. |
| Destroy → create reuses the slot | New ID differs (generation bumped) even if the index is the same. |
| `IsAlive` with a stale (pre-destroy) ID | `false` — never aliases the reused entity. |
| `AddComponent<Health>` then `GetComponent<Health>` | Same instance, value intact. |
| `GetComponent<T>` for a missing type | Empty pointer. |
| `HasComponent<T>` before/after remove | `false` → `true` → `false`. |
| `DestroyEntity` on an entity with components | All component stores drop it; `GetComponent` empty. |
| Unrelated component types | No collision — `Health` and `Name` never interfere. |
| Component access on a destroyed ID | No-op (empty pointer, no crash). |

All cases run in the harness as `EntityRegistryTest`.

---

## Decisions to Confirm (the review)

1. **Generational indices over plain integers.** Barely more code, and it
   makes destruction safe and memory bounded. It is the correct foundation —
   but if the four questions say "simpler", plain monotonic IDs are the
   fallback.
2. **`shared_ptr` components.** Consistent with the Service Registry, at the
   cost of one heap allocation per component. The dense value-storage
   upgrade is a deliberate later stone, not an accident.
3. **`LCE::Simulation` namespace.** Matches the README's claim and the
   existing folder. If a cleaner home appears, it is one rename.
