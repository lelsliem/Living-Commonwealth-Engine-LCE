# Platform Integration (0.4.0) — The Handshake

**Milestone:** 0.4.0 — Alpha · Platform Integration
**Status:** Design — pending review
**Related ADRs:** ADR-0023 (the core never includes game headers — now a
build fact), ADR-0024 (adapters translate, don't simulate), ADR-0014 (no
global state), Law 001 (simple things; compose the complex)

---

## The Spec Is a City

> A player walks into Diamond City. The settlers are not on quest scripts —
> they are hungry, they remember who cheated them, they flee when raiders
> come, and the market closes on rainy days. The game does nothing but show
> the result.

That sentence is the test plan. 0.4.0 makes it true by handing the
simulation to Fallout 4 — through one seam, in a separate project, with the
core untouched.

---

## The Boundary Is the Whole Design

Two decisions are already made, and both are now physical:

1. **The core never knows the game.** ADR-0023 is a build fact: the game
   headers are not even on disk in this repo. There is nothing to violate.
2. **The adapter is a separate project.** It links LCE.Core + CommonLibF4
   as an F4SE plugin. The license split is physical too: MIT core, GPL
   adapter (CommonLibF4 is GPL-3.0 with modding/linking exceptions).

The question 0.4.0 answers is therefore narrow and precise:
**what exactly crosses the seam?**

---

## Decision #1: the boundary is the public API

Three empty stubs sit in `Include/LCE/Interfaces/` (`IGameAdapter`,
`IWorld`, `IEntity`) — reserved before the simulation existed, on the guess
that the core would need virtual seams. The design review says:
**delete them.**

The core's public API already *is* the boundary:

- The adapter **calls**: `CreateEntity`, `DestroyEntity`, `Remember`
  (experiences and world facts), `Update` — and **reads** intents via
  `GetComponent<Intent>`.
- The adapter **guarantees**: an intent is a *hint*, not a command — the
  adapter decides how to walk the farmer, and may refuse.
- The core **promises**: no game knowledge, no queries of the world, a
  stateless tick (ADR-0014).

A virtual `IGameAdapter` would add a layer the game plugin never needs (it
can call the API directly) and one the tests don't need (the harness
already drives the simulation). The contract lives in this document and in
the adapter repo's implementation — not in empty headers.

The four questions, answered: *simpler?* yes. *belongs?* no. *needed?* no.
*helps?* no. If 0.5.0's SDK ever wants a host API — a desktop app driving a
world — we add a real interface then, shaped by a real consumer (YAGNI).

---

## The Two Sides

```
   Fallout 4 (the game)                  LCE.Core (the engine)
   ─────────────────────                 ─────────────────────
   RE::Actor, TESForm,                   EntityId, components,
   ActorValues, AI packages              Needs, Memory, Intent
        │                                     │
        │   adapter project (GPL)             │   this repo (MIT)
        │   an F4SE plugin                    │
        │   - form ↔ entity mapping           │
        │   - translator (form ↔ component)   │
        │   - executor (intent → game action) │
        └──────────┬──────────────────────────┘
                   │  the seam: the public API
                   │  adapter calls: CreateEntity, Remember, Update
                   │  adapter reads: GetComponent<Intent>
```

The adapter is a *client* of the core, exactly like the test harness — it
just happens to live in a different repository and talk to a game.

---

## Lifecycle — the game's heartbeat drives the simulation

F4SE's Messaging interface delivers the game's life events; the adapter
maps them onto the simulation:

| Game event   | The adapter does                                        |
|--------------|---------------------------------------------------------|
| `GameLoaded` | Create the registry; translate each sim-relevant Actor into an entity with components |
| game tick    | `Update(registry, delta)`; read intents; execute via RE:: |
| world events | `Remember` — experiences and world facts                |
| `PreSaveGame`| Snapshot the registry into the co-save record           |
| `PostSaveGame` | release the snapshot                                  |
| `PreLoadGame` | Clear the registry                                     |
| `PostLoadGame`| Restore the registry from the co-save record           |
| `DeleteGame` | Clear everything                                         |

The simulation never hears the word "quest." It hears: entities created,
time passing, things remembered.

---

## Translation rules — the adapter's art

- **Components ↔ game data.** A settler's `Hunger` becomes an `ActorValue`
  write through `RE::Actor`; a `Memory` of a merchant maps to the
  merchant's form. Translation happens at the edge (ADR-0024), never inside
  the core.
- **Intents ↔ game actions.** `MoveTo{merchant}` → a movement AI package to
  the merchant's location; `Flee{bandit}` → a flee package; `Rest` →
  wait/sleep; `Socialize` → a conversation scene. An intent names *what*,
  never *how*.
- **World facts — the proven channel (0.3.1).** "The market is closed
  today" = `Remember(id, { invalid, Trade, weight })`. The core blocks
  trade while the fact is remembered and reopens when it fades; the adapter
  re-pushes to extend. The core never queries the game.
- **Locations stay out of the core.** Intents target *entities*, not
  coordinates. The adapter resolves "the road to town."

---

## Threading — simple first

For 0.4.0 the simulation ticks **on the game thread** (a Papyrus timer or
a frame hook): zero contention, trivially debuggable, and the adapter's
data structures need no locking. When the simulation moves off-thread (a
later stone), F4SE's `TaskInterface` already exists to marshal
game-touching calls back — and because translation happens at the edge, the
adapter is already shaped for it.

---

## Save/Load — the deep stone

The game owns the save file; the simulation rides inside it (co-save via
F4SE's Serialization interface). The core side needs exactly one new
capability: **snapshot and restore the registry** — every entity, its
components, versioned.

Component types are erased in the registry; only the adapter knows their
shape. So serialization is *registered*, never assumed:

```cpp
// Core (0.4.0). A snapshot is type-erased bytes plus a version:
// the adapter turns it into F4SE serialization records and back.
struct RegistrySnapshot
{
    std::uint32_t Version;
    // one record per entity: id, alive flag, then per-component blobs
};

// The adapter registers a serializer per component type it uses.
// Compile-time type in; runtime blob out — and back again.
registry.RegisterSerializer<Needs>(
    [](const Needs& n) { return Blob{/* ... */}; },   // serialize
    [](const Blob& b) { return Needs{/* ... */}; });  // deserialize

auto snapshot = registry.Capture();     // into RegistrySnapshot
registry.Restore(snapshot);             // back into a fresh registry
```

The snapshot is a **pure data exchange** — no game knowledge crosses it.
Save-compatibility is the adapter's job: version the record, migrate on
load. The core test is a round-trip: snapshot a living registry, restore
into a fresh one, assert identical behaviour — the farmer still goes to
market.

---

## What stays OUT of the core (the isolation, again)

- Coordinates and locations
- Form IDs and game handles
- Papyrus (the adapter registers natives if mods need to ask the sim)
- Game events, quests, dialogue
- The threading policy

The core's world is entities, needs, memories, relationships, and intents.
Nothing else exists there — and the tree physically cannot contain anything
else (ADR-0023 is a build fact).

---

## Dependencies in the adapter project

The adapter fetches its own dependencies via FetchContent — the same
pattern this repo now uses for spdlog, pinned and self-contained:

- **CommonLibF4** (GPL-3.0 + modding/linking exceptions) — the typed game
  API: `F4SE::Init`, `GetMessagingInterface()`, `GetSerializationInterface()`,
  `GetTaskInterface()`, and `RE::` types (Actor, TESForm, ActorValueOwner).
- **F4SE** — the runtime and plugin contract (`F4SEPlugin_Load`); required
  at runtime, replaced as a static dependency by CommonLibF4.

The core repo carries none of it — the tree is the proof.

---

## Test plan

| Where | Proves |
|-------|--------|
| Core (this repo) | Snapshot round-trip preserves a living registry; boundary semantics (intents readable, Remember channel, world facts); existing 13 suites stay green |
| Adapter project (separate repo) | The real test: a settler in Fallout 4 goes to market because they are hungry — no script |

---

## Decisions to review

1. **API-only boundary — delete the `Interfaces/` stubs.** (Recommended;
   the four questions say simpler.)
2. **Tick on the game thread for 0.4.0.** (Simple first; thread-migration
   is a later stone with TaskInterface already waiting.)
3. **Snapshot via registered serializers.** (Components stay erased; the
   adapter owns save-compat.)
4. **World facts through `Remember`** — settled and proven in 0.3.1;
   restated here because the adapter is its first real consumer.

Review these, and the build is: delete the stubs, add
`RegistrySnapshot` + registered serializers, round-trip tests, docs — and
the adapter repository is born.
