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

At 0.4.0 the core shipped `0.4.0-alpha` (14/14 suites then); the
current core is 0.6.0-alpha, 25/25 suites green. Two things changed
at 0.4.0 that still matter to this project:

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

The core is at `0.5.0` (25/25 suites green). All seven stones of the
boundary contract are live — each with its own section below (tuning,
outcome channel, observation events, query surface, seeded RNG, world
calendar, per-mind decay jitter) — and stone 08 (bond thresholds +
`RelationshipChanged`, Request A below) has already shipped as the
first 0.6.0 stone:

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

**Request A — `RelationshipChanged` observation event — SHIPPED (core
stone 08, 2026-08-10, proven by the BondThreshold suite).** The core
now watches a bond watch-list named by the world: every
`sim.bond.threshold.<name>` key in the tuning file draws one line
across disposition (`sim.bond.threshold.friend = 0.3`,
`sim.bond.threshold.enemy = -0.6`; broken values ignored, names sorted
for deterministic order). When an experience — `Remember` or
`ReportOutcome` — moves a relationship across a line, the core
publishes `RelationshipChangedEvent` on the EventBus, edge-triggered:
the moment of crossing, then silent while the relationship rests on
either side. Payload: `subject`, `other`, `disposition`, `trust`,
`threshold` (the line's name — the world's vocabulary), `day` (world
day of the crossing). Drift is deliberately quiet: a bond cooling
below a line is a dissolve, not an event — re-derive bonds from the
relationship state you already read. Default watch-list is empty:
name your lines or hear nothing. `Remember`'s new trailing `EventBus*`
is defaulted — existing calls are untouched.

**Request B — optional, deferred: `GoalType` growth.** If the core wants
arcs first-class (`FindPartner`, `Avenge`, `Lead`) rather than the
adapter chaining the four generic intents, add them. **Not required for
0.6.0** — the adapter chains `AcquireFood` / `ReachSafety` / `Socialize` /
`Prosper`.


**Request C — tentative, for the agency pillar (0.8.0).** If settlers build, haul, and demolish as first-class goals, `GoalType` grows with `Construct` / `Haul` / `Demolish`. Not required: the adapter can map `Prosper` to labor intents on the existing surface. Flagged so the core knows the direction before 0.8.0 lands.

## Society is here — the adapter's 0.6.0 substrate (core stone 09, SHIPPED 2026-08-10, proven by the Groups + Traits suites)

The core's Society layer ships in the same world-agnostic shape as everything else. What the adapter gets, for free:

- **`Groups` component + `GroupId`** — the world assigns the ids (a settlement = a workshop hashed, a family, a faction); the adapter attaches `Groups` to each mind's memberships at translation.
- **The echo — automatic.** Once minds carry `Groups`, every `Remember` / `ReportOutcome` spreads a fainter disposition echo to their group-mates at `sim.group.inheritance` (default 0.5): wrong one settler and the settlement cools toward the wrongdoer — and each mate's bond crossings publish `RelationshipChangedEvent` (stone 08 rides along). Trust is never echoed — reliability is personal.
- **`InheritGroupAttitudes(registry, id, groupId)`** — call it when a mind joins a group (after `AddComponent<Groups>`): the newcomer inherits the group's mean disposition toward everyone the group collectively knows; their own experiences then diverge it. Personal knowledge beats inherited; quiet — seeding is not an event.
- **The `Traits` substrate** — `JitteredTraits(base, id, rng, spread)` derives per-entity personality from the seeded RNG (persist via the co-save like any component); your behaviour tables read the component and decide what "boldness" means — the influence is yours, exactly like the species split.

Your 0.6.0 life stones (bonds, gossip, arcs) now have their substrate: settlement membership, the rally, the inherited grudge. The next core need, if any, will be named in the adapter's hand-off.

**What the adapter will NOT ask for:** population logic, aging rules, or
quest grammar in the core — those are world-specific and the adapter's
job by design. The core stays a stateless, world-free tick.

---

## Negative social weighting — decided: kind and result, never a sign (2026-08-11)

The adapter asked whether Remember can write a negative social effect, or
whether the core wants a sign parameter. Decided: as-is — no hand-over,
no sign parameter.

- In Remember the direction lives in the Kind, and event.Weight is never
  consulted for relationship effects — weight is memory salience only
  (fade). Aid/Social always add DispositionGain (+0.1); Wronged/Combat
  always subtract DispositionLoss (-0.25); Trade always gains TrustGain
  (+0.15). A negative weight would cool nothing and be a memory that
  starts below the forget threshold — instantly forgotten.
- Valence belongs to the kind of experience; salience to how strongly it
  is remembered. A sign on the weight would fuse the two axes and change
  the meaning of every existing kind.
- The two designed channels for "negative social":
  1. Executed intents that went badly -> ReportOutcome(Other, Social,
     Failure). ResultScale: Success +1.0, Partial +0.5, Failure -1.0, so
     a failed social costs -0.1 disposition and the group echo carries
     the same sign. This also serves/frustrates the goal and consumes
     the intent — right for an attempted interaction.
  2. Unprompted negative experiences -> Remember(Other, Wronged), -0.25.
- Footgun: Remember with Kind=Trade always gains trust — a failed trade
  recorded as a memory would wrongly build trust. Executed interactions
  go through ReportOutcome; experiences go through Remember.
- If a distinctly social negative is ever needed (a courtship snub that
  cools by less than a wrong), the tool is a new InteractionKind
  (append-only ordinal, the Death precedent), not a sign parameter. Ask
  with the concrete beat and the core will add it.

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
