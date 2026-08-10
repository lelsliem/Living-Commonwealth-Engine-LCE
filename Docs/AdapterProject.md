# The Living Commonwealth — Fallout 4 Adapter Project

**Handoff document.** This file exists so a new agent (a new Freebuff tab
rooted at `C:\Fallout4Adaption`) can start this project with full context —
the plan, the conventions, and the contract — without having been part of
the conversation that built the engine.

**Companion documents** (in the LCE core repo, `C:\LivingCommonwealthEngine`):
- `Docs/Architecture/PlatformIntegration.md` — the 0.4.0 boundary design
  (read this first; it is the contract this project implements)
- `Docs/LearningPath.md` — how the engine works and what each piece teaches
- `Docs/ProjectPhilosophy.md` — the Six Design Laws, especially
  Law 001: *simple things; compose the complex*

---

## The Project

**Name:** The Living Commonwealth (the mod). The Fallout 4 adapter for the
Living Commonwealth Engine (LCE). Chosen because Fallout 4's setting *is*
the Commonwealth, and this mod makes it live.

**Repo naming:** decided 2026-08-10 — the adapter lives at
`The-Commonwealth-Lives` (the mod's public name: The Living
Commonwealth); the engine lives at `Living-Commonwealth-Engine-LCE-`.
Both under the lelsliem GitHub account.

**License:** GPL — the mod links CommonLibF4 (GPL-3.0 with modding/linking
exceptions). The core stays MIT. This split is deliberate and physical: the
adapter lives in its own repo and never shares a tree with the core.

**Check:** verify the name is not already taken on Nexus Mods before
publishing.

---

## What This Project Is

The adapter is a **client** of LCE.Core — exactly like the engine's test
harness, but living in its own repository and talking to a game. It does
NOT reimplement simulation. Per `PlatformIntegration.md`, the boundary is
the core's public API:

- The adapter **calls**: `CreateEntity`, `DestroyEntity`, `Remember`
  (experiences and world facts), `Update` — and **reads** intents via
  `GetComponent<Intent>`.
- The adapter **guarantees**: an intent is a *hint*, not a command — the
  adapter decides how to walk the settler, and may refuse.
- The core **promises**: no game knowledge, no queries of the world, a
  stateless tick.

The adapter is an F4SE plugin built on CommonLibF4 (`F4SE::Init`,
`GetMessagingInterface()`, `GetSerializationInterface()`, `GetTaskInterface()`,
`RE::` types).

---

## The core's 0.4.0 side is built — what the adapter gains

The core is now `0.4.0-alpha`, 14/14 suites green. Two things changed
that matter to this project:

1. **The boundary is the public API only.** The old
   `Include/LCE/Interfaces/` stubs (`IGameAdapter`, `IWorld`, `IEntity`)
   are **deleted**. The adapter is a client of the core — nothing to
   implement, nothing to include.
2. **Save/load has its substrate.** The core now ships `RegistrySnapshot`
   (`Include/LCE/Simulation/RegistrySnapshot.h`) and four registry
   operations the co-save stone will use:
   - `RegisterSerializer<T>({ serialize, deserialize })` — required for a
     component type to appear in a snapshot. Register once at init for
     every type the adapter uses (Needs, Memory, Relationships, Goals,
     Intent, and any adapter-defined components).
   - `Capture()` — the whole registry as pure data; entity identities
     (index + generation) preserved exactly.
   - `Restore(snapshot)` — rebuilds a registry with identical IDs;
     requires the same serializers to be registered.
   - `Clear()` — blank registry; serializers survive (register once,
     reuse across games).

   Semantics to respect:
   - A component type with **no serializer is not persisted** — omitted
     silently. Data presence decides membership.
   - The snapshot is a **process-local exchange format**. The adapter
     translates it into the F4SE co-save record with its own stable type
     names and its own versioning — save-compatibility is the adapter's
     job (migrate old saves on load).
   - Snapshot components are keyed by `std::type_index` — stable within
     a process, NOT across processes; another reason the durable record
     needs the adapter's own names.

   The lifecycle table below is now implementable: `GameLoaded` → create
   entities; `PreSaveGame` → `Capture()` → co-save record; `PostLoadGame`
   → read record → `Restore()`; `PreLoadGame`/`DeleteGame` → `Clear()`.
   The round-trip is proven by the core's Snapshot suite — the farmer
   still goes to market after a save and a load.

## The core's 0.5.0 side is built — the complete boundary contract

The core is at `0.5.0` (21/21 suites green). All seven stones of the
boundary contract are live — each with its own section below (tuning,
outcome channel, observation events, query surface, seeded RNG, world
calendar, per-mind decay jitter):

**Tuning ergonomics — `SimulationTuning::FromConfiguration(config)`.**
The modder's knob. Build the world's tuning from the Configuration
service in one call:

**Tuning ergonomics — `SimulationTuning::FromConfiguration(config)`.**
The modder's knob. Build the world's tuning from the Configuration
service in one call:

```cpp
Config::Configuration config;      // loaded from your ini/text file
config.Set("sim.memory.fade", "0.05");

const auto tuning =
    LCE::Simulation::SimulationTuning::FromConfiguration(config);

// Then feed it to the tick:
LCE::Simulation::Update(registry, deltaSeconds, tuning);
```

Contract (proven by the core's Tuning suite):

- **Known keys override defaults** — `sim.memory.fade`,
  `sim.memory.forget`, `sim.drift.rate`, `sim.goal.urgency`,
  `sim.trust.gain`, `sim.disposition.gain`, `sim.disposition.loss`.
- **A missing, empty, or unparsable value keeps the default** — a broken
  line never breaks the world.
- **Unknown keys are ignored** — the adapter may carry its own keys
  (`market.open.hour`, whatever) in the same file, and the core will
  never see them.

This is the *input* channel of your 0.5.0 checklist. The world's
personality — how fast memories fade, how quickly settlers forgive —
becomes a text file your users can edit. No recompile.

### The adapter's 0.5.0 roadmap — "The Settler Goes to Market"

Your roadmap item 2 (tuning from Configuration) now has its substrate.
The remaining stones, and what they need from the core:

1. **World facts via `Remember`** — already shipped and proven in the
   core (0.3.1). Weather, market open/closed, road conditions: push as
   memory events with an *invalid* Other. While the fact is remembered,
   the interaction is unavailable to the mind; when it fades, it
   reopens. The adapter controls duration by re-pushing.
   **Weather memory (2026-08-10):** the core grew the `Weather*`
   InteractionKinds (appended at the end — the adapter co-save writes
   the raw ordinal, so old saves stay valid). The adapter now classifies
   the sky and pushes day-stamped weather facts (`{ invalid,
   WeatherRain, 1.0, day }`); Decide never gates them — they are labels,
   not doors.
2. **Tuning from Configuration** — ✅ core stone 01 (this section).
   One call, one text file.
3. **The real test — a settler goes to market because they are hungry.**
   Needs decay → goal urgency grows → the mind produces a
   `MoveTo`-class intent → the executor walks them (already proven).
   The core does not need to know the market is a place; the adapter
   resolves "which trader" — locations stay out of the core.
4. **The outcome channel** — ✅ core stone 02 (shipped, 16/16 green).
   `LCE::Simulation::ReportOutcome(registry, id, outcome, tuning)`:

   ```cpp
   // After the executor acts on an Intent, report how it went:
   LCE::Simulation::Outcome outcome;
   outcome.Other   = trader;      // the counterparty
   outcome.Kind    = LCE::Simulation::InteractionKind::Trade;
   outcome.Result  = LCE::Simulation::OutcomeResult::Success; // or Partial/Failure
   outcome.Weight  = 1.0f;        // salience of the memory

   LCE::Simulation::ReportOutcome(registry, settler, outcome, tuning);
   ```

   Semantics (proven by the core's Outcome suite):
   - The memory is recorded (weights carry, fade, reinforce).
   - Relationships scale by result — a *successful* trade builds trust;
     a *failed* one loses it (the merchant proved unreliable); a wrong
     is a wrong either way.
   - A Success serves the active goal the kind feeds; a Partial halves
     its urgency; a Failure leaves it growing.
   - **The intent is consumed** — the next tick decides fresh with the
     outcome in memory. The loop closes: decide → act → observe →
     remember → decide.
   - World outcomes (`Other` invalid — "the road was blocked") record
     memory only.

   What this means for the adapter: after each executed walk, translate
   what the game actually did into an `Outcome` and call
   `ReportOutcome`. The settler robbed en route remembers it; the
   merchant who cheats loses trust; the next market trip chooses
   differently — a learning settler, no script. Use `OutcomeResult`
   honestly: a walk that never reached the trader is a `Failure`; one
   that got there but didn't trade is `Partial`; a fair trade is a
   `Success`.

---

## Observation events (core stone 03, shipped — push, not poll)

The simulation now tells you what happened instead of waiting to be
asked. Subscribe once at init:

```cpp
// EntityRegistry::SetEventSink(&bus) once — then CreateEntity publishes
// EntityCreatedEvent for every genuinely NEW entity. Snapshot restore
// does NOT publish: loading a co-save is a restore, not a creation
// flood — you hear only the minds that actually appear.

// Update(registry, dt, tuning, &bus) publishes IntentProducedEvent for
// every fresh decision — the executor can react to new intents without
// polling the registry.

// ReportOutcome(registry, id, outcome, tuning, &bus) publishes
// OutcomeRecordedEvent — react to results immediately.
```

Event types live in `LCE/Simulation/SimulationEvents.h`
(`EntityCreatedEvent{ Id }`, `IntentProducedEvent{ Id, Intent }`,
`OutcomeRecordedEvent{ Id, Outcome }`), all deriving
`LCE::Events::Event`; subscribe by `typeid`. The bus is an input,
never global state — pass it where you want it, omit it where you
don't.

## Query surface (core stone 04, shipped — filtered reads, deterministic order)

`EntityRegistry::QueryWhere<T>(predicate)` returns every entity with a
component of type T that satisfies the predicate, as a
`std::vector<EntityId>` **sorted ascending by ID** — the same query
returns the same result every run, so your co-save round-trip and any
future seeded randomness stay reproducible.

```cpp
// Everyone hungry:
const auto hungry = registry.QueryWhere<LCE::Simulation::Needs>(
    [](LCE::Simulation::EntityId, const LCE::Simulation::Needs& needs)
    {
        for (const auto& need : needs.List)
            if (need.Value < 0.5f) return true;
        return false;
    });

// Settlers who remember the raid — cross-component by capture:
const auto witnesses = registry.QueryWhere<LCE::Simulation::Memory>(
    [&registry](LCE::Simulation::EntityId id, const LCE::Simulation::Memory& m)
    {
        return registry.HasComponent<LCE::Simulation::Needs>(id)
            && /* m recalls a Combat event */;
    });
```

The predicate gets `(EntityId, const T&)` — the component is `const`,
a query never mutates what it reads. Empty store or no match → empty
vector. This is the replacement for hand-rolled sweeps: read first,
then act.

## Seeded RNG (core stone 05, shipped — determinism a save can resume)

`LCE::Simulation::Rng` — splitmix64 with **one word of state**: persist
`rng.State()` in your co-save record and a restored world resumes the
exact same randomness. `SetState` restores it.

```cpp
// World-level variation (your stream, you own it):
LCE::Simulation::Rng rng{ seedFromConfigOrGame };
const auto variation = rng.NextFloat(0.0f, 1.0f);

// Per-entity personality (order-independent):
LCE::Simulation::Update(registry, dt, tuning, &bus, &rng);
// Decide derives each entity's jitter from its ID — iteration order
// can never leak into results. Same seed, same world, every run.
```

Contract: `Derive(key)` never advances the parent, so the tick's
unordered store iteration is safe by construction. Pass `nullptr` (or
omit) to keep the deterministic id-hash fallback — behavior unchanged.

**The herd, broken (stone 07):** once you pass the `Rng`, the tick's
need-decay step gives every mind its own metabolism — each entity's
needs decay at `DecayRate * Derive(id).NextFloat(1 ± jitter)`, so
identical settlers stop getting hungry on the same clock. The knob is
`sim.jitter` (default 0.15); `0` turns the spread off. No Rng → decay
is exactly as it always was. In-game symptom this fixes: the whole
settlement marching to the bench in lockstep. Pair it with the
settlement-market stone (each mind seeded with its home settlement's
market fact) and the march becomes a market day.

## Animals are not settlers — a field finding for the translator

In-game observation: junkyard dogs and brahmin get minds and walk to
the market. Root cause in `SimRelevant.cpp`: the door into the sim is
`IsSimRelevant` → `IsInFaction(SettlerFaction)` — and in Fallout 4,
animal workshop actors carry **WorkshopNPCFaction too**, so they pass
the same gate as settlers.

This is the adapter's problem, and the core is already shaped for it:
the game's notion of "animal" is game knowledge — the adapter owns the
door. Two layers to fix:

1. **At the door (translation).** `IsSimRelevant` should exclude
   non-settler actors — e.g. check the actor's race/creature-ness
   (`RE::Actor::race` is exposed in the clone; or the actor base's
   creature data). A brahmin is not a settler.
2. **Component shaping (the elegant part).** The core separates minds
   from rocks *by components* — "a rock has no needs and is
   untouched." An animal that gets only a Hunger need — no Social
   need, no trade memories, no Prosper goal — is a *grazer*: it
   explores when hungry and never heads to a trader. No core change
   needed; the behaviour difference is what you attach at
   translation time. If animals should be in the sim at all, give
   them the animal-shaped mind; if not, don't create an entity for
   them.

Either way the core stays game-agnostic — it never needs to know what
a dog is.

## World calendar + memory timestamps (core stone 06, shipped — the last boundary stone)

`LCE::Simulation::WorldTime` — a day counter you drive from the game's
clock — and `SeasonOf(day)` (four 90-day seasons). `MemoryEvent::Day`
anchors a memory to the world day it happened:

```cpp
// Pass today's world day when pushing facts or reporting outcomes:
LCE::Simulation::Remember(
    registry, id, { merchant, LCE::Simulation::InteractionKind::Trade, 1.0f },
    {}, LCE::Simulation::WorldTime{ currentGameDay });

LCE::Simulation::ReportOutcome(
    registry, id, outcome, tuning, &bus,
    LCE::Simulation::WorldTime{ currentGameDay });
```

Rules: a caller-set `event.Day` wins (report a historical event while
passing today); otherwise the passed `WorldTime` stamps it. The age of
a fact is `now.Day - event.Day`. Seasons are derived, never stored.

**⚠ Save-format change — your co-save record must carry it.** The
Memory serializer must now write `event.Day` (the core's Snapshot
suite does). This is your co-save version seam's first real exercise:
write the new field, bump your record format version, and migrate old
saves on load. Save-compat is yours.

## The adapter's 0.6.0 needs — life & emergent quests (hand-over 2026-08-10)

The adapter's 0.5.0 is complete, in-game verified, and published
(GitHub, tag `0.5.0-beta`). Its 0.6.0 plan — **"The Commonwealth
Remembers"** — simulates birth, life, death, and good/bad relationships,
with quests that emerge from sim state (feuds, grief, courtship,
departure, famine). The full plan lives in the adapter repo
(`C:\Fallout4Adaption\Docs\Design\Life.md`); this section is the
adapter's ask of the core.

**The honest headline: the 0.5.0 boundary contract suffices for nearly
all of it.** The adapter builds lifecycle (arrival / death / departure),
bonds (named relationship states), households, gossip, emergent arcs,
and experimental births on the existing surface — `CreateEntity` /
`DestroyEntity` / `Remember` / `Update`, `Relationships`, `Goals`,
`Memory`, the `Outcome` channel, observation events, `WorldTime`, and
the seeded `Rng`. No new core components are required to start.

**Request A — `RelationshipChanged` observation event (candidate core
stone 08).** The core owns disposition/trust dynamics (drift, outcome
shifts). Let the core emit an observation event when a relationship
crosses a configurable threshold — so bond formation is an **event** the
adapter (and, through gossip, other minds) reacts to, instead of the
adapter polling `Relationships` every tick. Payload: `subject`, `other`,
disposition, trust, threshold name (from tuning, e.g.
`sim.bond.threshold.*`), world day. The core stays world-agnostic: it
knows nothing about marriage — only that a relationship crossed a line
the world configured. This is a small, testable addition in the same
shape as the shipped observation events (push, not poll).

**Request B — optional, deferred: `GoalType` growth.** If the core wants
arcs first-class (`FindPartner`, `Avenge`, `Lead`) rather than the
adapter chaining the four generic intents, add them. **Not required for
0.6.0** — the adapter chains `AcquireFood` / `ReachSafety` / `Socialize` /
`Prosper`.


**Request C — tentative, for the agency pillar (0.8.0).** If settlers build, haul, and demolish as first-class goals, `GoalType` grows with `Construct` / `Haul` / `Demolish`. Not required: the adapter can map `Prosper` to labor intents on the existing surface. Flagged so the core knows the direction before 0.8.0 lands.

**What the adapter will NOT ask for:** population logic, aging rules, or
quest grammar in the core — those are world-specific and the adapter's
job by design. The core stays a stateless, world-free tick.

---

## Canonical Copy

The Living Commonwealth Engine repo owns this document. The adapter
project (`C:\Fallout4Adaption`) reads it from
`C:\LivingCommonwealthEngine\Docs\AdapterProject.md` — the core repo is
the single source of truth, and any copy in the adapter repo is a
snapshot, not a fork. When the core ships a new stone, this document
grows here first.

## Dependency Wiring

`C:\Fallout4Adaption\Depends\` holds the clones the build needs:
`commonlibf4` (the static dependency, built via xmake `includes`) and
`spdlog` (offline source for the core build's v1.16.0). The original six
were trimmed in 2026-08 — see the adapter repo's `Depends/README.md`
for what was removed and why.

- **CommonLibF4** — the mod's game API + plugin contract. Built from the
  local clone; its `RUNTIME_LATEST` (1.11.221) matches the game.
- **F4SE** — runtime-only. The plugin does not link the F4SE source;
  CommonLibF4 replaces it as the static dependency. The `f4se_1_10_*.dll`
  runtime is a download, installed via the mod manager.
- **LCE.Core** — linked statically, built by its own CMake via the
  `lce.core` rule into `Build/core` (never touching the core repo's
  `Build/`). The rule pins the core version: it reads `Version.h` and
  refuses anything below 0.4.0.
- **spdlog** — the local clone feeds the core build's `v1.16.0` (matching
  the plugin's xrepo spdlog, `std::format` mode) so one spdlog serves the
  DLL. The mod itself logs through LCE's API and `REX::LOG`, never spdlog
  directly.

The adapter repo owns the living copy of this handoff; this file in the
core repo is the snapshot from which it grew.

---

## Lifecycle Mapping (the mod's heartbeat)

| Game event | The adapter does |
|------------|------------------|
| `GameLoaded` | Create the registry; translate each sim-relevant Actor into an entity with components |
| game tick | `Update(registry, delta)`; read intents; execute via `RE::` |
| world events | `Remember` — experiences and world facts ("the market is closed" = `{ invalid, Trade, weight }`) |
| `PreSaveGame` / `PostSaveGame` | Snapshot the registry into the co-save record; release |
| `PreLoadGame` / `PostLoadGame` | Clear the registry; restore from the co-save record |
| `DeleteGame` | Clear everything |

Translation rules: components ↔ game data (a settler's `Hunger` ↔ an
`ActorValue` write through `RE::Actor`); intents ↔ game actions (`MoveTo`
→ a movement AI package; `Flee` → a flee package; `Rest` → wait/sleep);
locations stay out of the core — intents target entities, the adapter
resolves the road.

---

## Working Conventions (how the engine was built — follow these)

1. **The build rhythm, one stone at a time:**
   design document → headers → source → tests → docs → green build.
   A milestone is not done until the docs claim it truthfully.
2. **The four questions** for every piece of design:
   *Can it be simpler? Does it belong? Do we need it at all? Will this help
   build living worlds through simulation?*
3. **The quote ritual:** every new `.h`/`.cpp` gets a banner with a line
   reserved for a joke or quote from the author. Leave the slot; the author
   fills it. Tests get compact headers, no banners.
4. **Version ritual:** on milestone completion, bump the version everywhere
   (`Version.h`, `CMakeLists.txt`, README badge) and commit after the
   author approves. The author reads and questions everything — no question
   is too basic; explain the *why*, not just the *what*.
5. **Commit style:** concise messages that say *why*, one coherent change
   per commit, no stray files, never commit `Build/` artifacts or tool dirs
   (`.gitignore` first).
6. **No global state** (ADR-0014). Time and tuning are inputs.

---

## First Stones (the initial commit list)

1. `git init` + `.gitignore` (learn from the core's: `/Build/`, `.vs/`,
   `*.user`, tool dirs).
2. Project scaffold: `CMakeLists.txt` (C++23, `/W4 /WX`, static runtime —
   match the core's compiler config), dependency wiring (CommonLibF4 +
   F4SE + LCE.Core), `F4SEPlugin_Load` entry point with version banner and
   quote slot.
3. A hello-world plugin that loads in Fallout 4 and logs through LCE —
   the "first heartbeat" of the mod.
4. Then the real stones: entity ↔ form translation, intent executor,
   co-save, and the real test: *a settler goes to market because they are
   hungry — no script.*
