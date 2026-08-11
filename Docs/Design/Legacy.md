# Legacy (0.7.0) — Design

The engine-side design for 0.7.0 — Legacy · "The Debt to the Past".
Locked 2026-08-11 after discussion. Three stones: what an entity
bequeaths as it goes, how descendants inherit memory selectively, and
the promise that outlives its maker.

## The substrate (all shipped)

- WorldTime + MemoryEvent::Day — the age of a fact is now.Day - event.Day.
- Memory fade (weight over time) and day stamps are independent axes.
- InteractionKind::Death — a fact, never a door.
- Groups + InheritGroupAttitudes — feelings already inherit from the
  group mean (stone 09); the grudge travels on this channel.
- The adapter's death path (gossip the fact, DestroyEntity) and birth
  path (blank-mind child) — the two seams this milestone extends.

## The boundary, in one line

The world names the people (heirs, ancestors, what a legacy means); the
core owns the mechanics (what salience merits, how faint a secondhand
story is, what outlives the owner).

## Stone 10 — Death lifecycle: Bequeath

The world names the heirs (the adapter knows family, household,
settlement); the core keeps what salience merits.

    std::size_t Bequeath(
        EntityRegistry& registry,
        EntityId dying,
        std::span<const EntityId> heirs,
        const SimulationTuning& tuning = {});

- Copies the dying entity's MemoryEvents at or above
  tuning.BequestFloor into each heir's Memory, scaled by
  tuning.InheritanceScale.
- The fact keeps its original world day — the story's age survives the
  transfer ("the feud is decades old"). Refinement over the first
  sketch ("day-stamped as of the death"): re-stamping would destroy the
  age information that "entities remember decades" exists to carry. A
  fact's date is a property of the story, not of the hearer — the same
  rule as Remember's caller-set day.
- Append, never overwrite: the heir's own memories are untouched.
- Deterministic: heirs are processed in ascending EntityId order, so
  the caller's list order can never leak into results.
- Returns how many facts were bequeathed. No new event — the adapter
  called the death; it needs no announcement back.

Decisions: no Will component (the adapter owns the death path; a Will
adds bookkeeping for no gain — a 0.7.x extension if a modder asks). The
settlement's feelings toward the dead need no work: they live in
survivors' Relationship stores and outlive DestroyEntity naturally.

## Stone 11 — Generational handoff: InheritMemory

The story travels through the core; the grudge rides the group echo.

    std::size_t InheritMemory(
        EntityRegistry& registry,
        EntityId heir,
        EntityId ancestor,
        const SimulationTuning& tuning = {},
        WorldTime time = {},
        bool (*accept)(const MemoryEvent&) = nullptr);

- The world's predicate selects which facts travel (nullptr = all) —
  the same vocabulary-free seam as QueryWhere. "Selective" is in the
  roadmap's own words; the seam ships now.
- The core scales (tuning.InheritanceScale — a story heard is fainter
  than a life lived) and ages (tuning.LegacyMaxAgeDays filters facts
  older than the world's patience; 0 keeps everything; unstamped or
  future-dated facts pass — age is the story's).
- The heir's own memories are never touched.
- The grandson proof (the Inheritance suite's integration block):
  wrong done to the ancestor -> the family group turns cold via the
  echo -> the grandchild joins the family at birth, so
  InheritGroupAttitudes seeds the cold disposition -> InheritMemory
  brings the story -> Decide refuses the merchant, chooses the other
  stall. The feud outlived its owner — memory and relationship, two
  channels composing.

## Stone 12 — Legacy as world fact: the promise that outlives its maker

A registry-level store, deliberately shaped like the component stores.

    struct LegacyFact
    {
        EntityId Owner;          // goes stale on death — the fact persists
        std::uint64_t Day = 0;   // when it was left
        std::string Name;        // opaque — the world names it
        float Weight = 1.0f;
    };

    // EntityRegistry: LeaveLegacy / ReadLegacy / ForgetLegacy /
    // RegisterLegacySerializer (the same serializer contract as
    // components — the world encodes the name map for the co-save).

- The core holds opaque named data with an owner and a day — it knows
  nothing of bridges or pledges (the GroupId boundary).
- Permanent until the world deletes: the promise that outlives its
  maker. Decay is an explicit world choice (ForgetLegacy on a calendar
  event — the bridge is repaired, the pledge fulfilled; or re-leave at
  lower weight when retold). A global fade axis is rejected: decay is
  the mind's job, not the record's, and core-owned timing would be
  world-vocabulary judgment. The store's unbounded growth is a 0.8.0
  Scale question, decided with data; a per-fact fade field is the
  natural 0.8.0 fix if it hurts.
- RegistrySnapshot gains a registry-level Legacy section (optional
  blob); schema version bumps to 2. Clear() wipes legacies; the
  serializer survives (same contract as component stores).
- Learning is the world's call: the adapter pushes a legacy into a
  mind's Memory with Remember when the mind encounters it. The core
  never auto-teaches.

## Tuning (0.5.0 FromConfiguration path)

- sim.legacy.bequestFloor    (0.5)  — salience below which a fact stays
  with the dead.
- sim.legacy.inheritanceScale (0.5) — secondhand stories are fainter
  than lived experience (applies to both Bequeath and InheritMemory).
- sim.legacy.maxAgeDays       (0)    — inheritance age limit; 0 = any
  age. (InheritMemory only; Bequeath passes everything above the floor
  — the world filters later via predicate.)

Defaults are mechanic-level: the core doesn't know the game, so it
stays conservative (0.5/0.5, permanent).

## Determinism and save-compat

- Bequeath and InheritMemory are pure copies — no RNG, no mutation of
  the source. Heirs sorted ascending like QueryWhere: results cannot
  depend on list order.
- All new state flows through components or the legacy store — both in
  the snapshot. Nothing in globals (ADR-0014).
- Legacy section rides the co-save through the world's registered
  serializer; the world layers its own record versioning on top, as
  always.

## The adapter's 0.7.0 (when it starts)

- Stone 10: call Bequeath in its death path (heirs fall out of its
  household bonds), before DestroyEntity.
- Stone 11: call InheritMemory in Birth::Create (with a predicate:
  parents' memories about people the child can know) — one line where
  it currently seeds blank memory.
- Stone 12: LeaveLegacy on death ("the miller's pledge"), Remember it
  into a mind when the mind reaches the place — the generational hooks
  for its feud and quest arcs.
