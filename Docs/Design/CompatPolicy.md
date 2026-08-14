# LCE Compatibility Policy

**The 0.8.4 freeze contract.** From 0.8.4 on, the public surface below is
the contract an adapter builds against. This document says what is
stable, what is append-only, what the world owns, and what breaks and
when. Written for the adapter that pins LCE, and for the modder who
reads the headers.

---

## 1. What is stable (never changes after the freeze)

These are the load-bearing walls. A change to any of them is a *major*
version bump and an explicit migration — never a silent one.

| Surface | Notes |
|---|---|
| `Simulation::Update` signature | the tick — `(registry, delta, tuning, events, rng, report)` |
| `Simulation::Decide` signature | `(registry, id, rng, desperateHunger)` |
| `Remember` / `ReportOutcome` / `Bequeath` / `InheritMemory` / `InheritGroupAttitudes` | the loop's verbs |
| `SimulationTuning` fields | every knob a world tunes |
| `FixedStep` | the timing-independent tick |
| `EntityRegistry` lifecycle | `CreateEntity` / `DestroyEntity` / `IsAlive` / `Capture` / `Restore` / `Clear` |
| Component stores | `AddComponent` / `GetComponent` / `QueryWhere` |
| Snapshot & serializers | `RegisterSerializer`, `ComponentBlob`, `kSnapshotVersion` |
| `Rng` | `Next` / `NextFloat` / `State` / `SetState` / `Derive` / `StableDerive` |

**What "stable" means in practice:** the signatures and their meanings
do not change. Implementation details (performance, iteration order
*within* a documented rule) may — but never the shape of the contract.

## 2. What is append-only

These grow at the end and never shrink, never renumber.

- **`InteractionKind` ordinals.** The adapter's co-save writes the raw
  ordinal, so new kinds go at the end, never in the middle, never
  removed. This is why the weather kinds and `Death` still exist
  (0.8.4): removing them would corrupt every existing save. New
  fact-types use `InteractionKind::Fact` + a label string instead —
  the enum stops growing because labels absorb it.
- **`NeedType` / `GoalType` / `ActionType` ordinals.** Same rule, same
  reason.
- **`SimulationTuning` keys** (`sim.memory.fade`, `sim.jitter`, …):
  new keys may be added; existing keys keep their meaning and default.
- **`TraitValue` names.** The world's vocabulary; the core never
  interprets them. A name is whatever the world says it is.

## 3. What the world owns (adapter-owned, never core vocabulary)

The core is deliberately vocabulary-free. These are *the world's* — the
core carries them and never interprets them:

- **The meaning of `InteractionKind::Fact` labels.** "radstorm",
  "plague", "the old road must hold" — the core fades them, forgets
  them, and never reads them. The world's serializer owns whether the
  label rides the co-save (0.8.4).
- **Health, disease, medicine.** The adapter's 0.8.0 verdict stands:
  the world owns the Health table; the core supplies memory and need.
- **Richer bonds.** `Disposition` + `Trust` are the core's full
  vocabulary. Loyalty, fear, love — model them as `Traits` or a
  side-table keyed by the `EntityId` pair. Promote one into the core
  only when a *game* actually consumes it.
- **Goals.** `ReportOutcome` serves and frustrates the active goal;
  the world decides what a goal means and when it is set.

## 4. What breaks, and when

| Change | When it breaks | Who migrates |
|---|---|---|
| Removing or renumbering an enum ordinal | immediately | adapter co-save |
| Changing a stable signature | major version bump | adapter code |
| Adding a field to `MemoryEvent` / `Relationship` / `Need` | when the world's serializer ignores or misreads it | adapter serializer + record version |
| Changing `kSnapshotVersion` | immediately | adapter record check |
| Changing the meaning of a tuning key | immediately | world config |

**The one rule above all:** every break ships with a migration path in
the release notes — never a silent change. The adapter's co-save record
owns its own version; when a schema change lands, the adapter bumps it,
migrates old records, or refuses them loudly (never silently misreads).

## 5. The tie-break and per-need metabolism (0.8.4)

Two behaviours the freeze locks in, because they are the seam where a
world's traits enter decisions:

- **Per-need metabolism.** Each need decays at its own seeded rate,
  keyed on the need *type* — a bold mind's Safety can decay faster than
  its Hunger. Same seed + same entity + same need = same rate, every
  tick. The key is never the list index: two minds with identical
  needs listed in different orders metabolize identically.
- **The personality tie-break.** When needs are within a small band
  (0.05) of the most urgent, the winner is the one with the highest
  per-need seeded draw — list order decides nothing. Same seed + same
  entity + same needs = same choice, always. A world multiplies its
  traits into this seam by adjusting needs or decay rates before the
  tick.

Both are vocabulary-free: the core never learns "boldness", it only
sees a seeded, deterministic draw.

## 6. The naming convention (frozen style)

The public headers follow a deliberate naming style; `SurfaceTest`
pins it, so it is contract, not taste:

- **Events read as "what happened".** A noun plus a past participle —
  `EntityCreatedEvent`, `IntentProducedEvent`, `OutcomeRecordedEvent`,
  `RelationshipChangedEvent`.
- **A single-payload event names its member after its type.**
  `IntentProducedEvent` holds `Intent Intent;` and
  `OutcomeRecordedEvent` holds `Outcome Outcome;`. The member reads as
  the thing it carries — "the intent that was produced" — not as a type
  declaration.

That member idiom is legal C++ and part of the frozen ABI. Clang and
MSVC accept it silently; GCC 13+ rejects it with `-Wchanges-meaning`
(a member named like its type is ambiguous in a redeclaration). The
build silences that diagnostic on GCC only — the API is never renamed
to appease a compiler, because the names are the contract.
