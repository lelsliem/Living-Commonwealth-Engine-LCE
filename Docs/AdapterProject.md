> **Status: closed record (0.8.9).** The engine's API is frozen; every
> hand-over below has landed. This document is the historical contract
> between the two repos — what the core provides and how the adapter
> consumes it — and the record of the engine↔adapter proof. No new
> entries are expected while the freeze is in force; the adapter's own
> project keeps its own copy current for its 0.9.0 gate.

# The Living Commonwealth — Fallout 4 Adapter Project

**Handoff document.** This file exists so a new agent (a new Freebuff tab
rooted at `C:\Fallout4Adaption`) can start this project with full context —
the plan, the conventions, and the contract — without having been part of
the conversation that built the engine.

**Current core:** `0.8.9-alpha` (32/32 suites; 0.8.1 was tagged
`v0.8.1` and pushed 2026-08-13). Since 0.8.0: the ADR-0029 re-roll
fix (`Rng::StableDerive`), the Simulation folder reorganization
(your includes were re-synced), the surface guards (HeaderMapTest + the new SurfaceTest
declaration-level freeze — enum ordinals, field types, and
signatures pinned at compile time + the Doctor include-layout
check), ALL SEVEN pattern samples (0.8.2:
Economy, Legacy, Weather, Children; 0.8.3: Faction Wars, Disease,
Roads — teaching-only, zero new surface for the adapter to
consume), and the first 0.8.4 freeze stones — the personality
tie-break, per-need metabolism, the Fact kind + label, and the
compat policy doc, the surface-stability test, and the public-header
audit — which found ZERO API change for you (the Goals seam is now
documented honestly: Decide reads needs only; goals influence through
your planning layer biasing needs before the tick — behaviour
unchanged). 32/32 suites. Each has its own section below.

Two 0.8.4 changes could matter to you: (1) near-tied needs now
resolve by a per-need seeded draw instead of list order — if you
constructed multi-need minds with exactly equal needs and relied on
the order deciding, the coin is now seeded (deterministic, but
possibly a different side); (2) `MemoryEvent` gained a `Label`
string — your Memory serializer is the world's to keep or drop it;
if you don't write it, restore yields an empty label, which is
fine. The `InteractionKind` ordinals you persist are untouched
(Fact was appended at the end). The JitteredTraits fix from 0.8.2
still stands: if you captured trait values into a co-save,
regenerate them — the values change, the schema does not.
The adapter's in-game verification of 0.8.0 Scale (and the
StableDerive fix) remains the open gate — its 0.8.6c scale pass
covers it. Per the adapter's own hand-over (2026-08-13): Request E
is none — its run to the Nexus beta needs zero new engine surface,
so the engine is free to proceed with its own ladder.

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
current core is 0.8.9-alpha, 32/32 suites green. Two things changed
at 0.4.0 that still matter to this project:

1. **The boundary is the public API only.** The old
   `Include/LCE/Interfaces/` stubs (`IGameAdapter`, `IWorld`, `IEntity`)
   are **deleted**. The adapter is a client of the core — nothing to
   implement, nothing to include.
2. **Save/load has its substrate.** The core now ships `RegistrySnapshot`
   (`Include/LCE/Simulation/Entity/RegistrySnapshot.h`) and four registry
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

At 0.5.0 the core shipped `0.5.0-alpha` (25/25 suites then); the
current core is 0.8.9-alpha, 32/32 suites. All seven stones of the
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
needs decay at `DecayRate * StableDerive(id).NextFloat(1 ± jitter)`,
so identical settlers stop getting hungry on the same clock. The
knob is `sim.jitter` (default 0.15); `0` turns the spread off. No
Rng → decay is exactly as it always was. In-game symptom this
fixes: the whole settlement marching to the bench in lockstep. Pair
it with the settlement-market stone (each mind seeded with its home
settlement's
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

## The core's 0.7.0 side is built — Legacy (hand-over 2026-08-11)

Engine side of 0.7.0 shipped that day (28/28 suites; design in
Docs/Design/Legacy.md) and was verified in-game by the adapter's
0.7.0 release — the feud chain ran end to end (shut stall → blame →
rival → mediated). What follows is the hand-over as written then,
kept as the record. The world names the people; the core owns
the mechanics. Three one-liners for the adapter's death and birth
paths:

1. **Bequeath** — `Bequeath(registry, dying, heirs)` before
   `DestroyEntity`: the dead's facts at or above
   sim.legacy.bequestFloor pass to the heirs, scaled by
   sim.legacy.inheritanceScale, their own world day intact. Heirs
   fall out of the adapter's household bonds. Append, never
   overwrite; heir order is deterministic.
2. **InheritMemory** — `InheritMemory(registry, child, parent,
   tuning, time, predicate)` in Birth::Create where the child is
   seeded blank: the world's predicate selects (parents' memories
   about people the child can know), the core scales and ages
   (sim.legacy.maxAgeDays). The grudge still rides
   InheritGroupAttitudes — the story travels on the memory
   channel, the feud on the group echo.
3. **Legacy as world fact** — `registry.LeaveLegacy({ owner, day,
   name, weight })` on death ("the miller's pledge"), read with
   `ReadLegacy`, retired with `ForgetLegacy`; teach a mind by
   `Remember`-ing it as a world fact when the mind reaches the
   place. Permanent until the world deletes — decay is a 0.8.0
   Scale question.

Snapshot schema is v2: the registry-level Legacy section is an
optional blob (registered via `RegisterLegacySerializer`), absent
in old records — a Restore simply starts with no legacies. The
adapter's own record versioning still layers on top, as always.

## The conflict source was blocked — a field finding, now fixed (2026-08-11)

Proving 0.7.0 in-game surfaced a real blocker in the core's Decide:
**the slight could never fire, so feuds could not begin.** The adapter
requested one engine change — "desperate hunger ignores the closed
sign." **Shipped (engine commit `509a54d`):** `sim.hunger.desperate`
(default 0.0 = never desperate, unchanged behavior; the adapter sets
0.2 in its INI). Below the threshold a mind still chooses MoveTo to
the remembered market while the Trade fact is unavailable; moderate
hunger still respects the shut door.

**Verified in-game by the adapter (2026-08-11).** With the market
forced shut and hunger high, the whole chain ran in the wild: `the
stall at 00054BAE is shut — Paladin Danse went hungry and blames the
keeper` → rival bonds (the direct −0.1 plus the settlement echo) →
`X is feuding with Y` → feud gossip → `arcs: Titus Pratt cooled the
feud between …` (23 feuds and 127 mediation attempts in one stressed
session; a clean build restored 673 minds, 78 bonds, 10 stall-keepers,
4 children with zero errors). Two adapter-side fixes were needed on
top of the gate — the feud headline now fires on any crossing into
Enemy, and the feud is mediated at formation (the adapter's gossip
dies in ~4.5 s at its sim.memory.fade 0.2, so its once-per-day pass
could never find a mediator) — both adapter commits, no further core
changes needed. The verify sentence below is met.

**The finding (verified in-game, not theory):** the adapter refreshes
the market-closed world fact (`{ invalid, Trade }`) to full weight
**every second** while the market is shut, and Decide's Trade branch
suppresses MoveTo while that fact is remembered (`IsUnavailable` →
Explore). The suppression propagates within a second of the close —
and because Explore executes as a new game command, it *replaces* a
walk in flight: the settler stops approaching and wanders instead.
The design's "a walk already in flight still arrives" does not hold;
the straggler window is effectively zero. Observed in-game: after the
close, 555 walk sessions ended with closest-approach 200–1900 u and
zero arrivals; 47 arrivals occurred, all before the close.

**The ask — one line in Decide's Trade branch:** a mind whose hunger
is critical still chooses MoveTo to the remembered market even when
the Trade fact is unavailable. The arrival then lands on the closed
bench, the adapter reports `ReportOutcome({keeper, Social, Failure})`
(−0.1), the settlement echo spreads it, and the feud becomes real
and testable. This is the designed famine machinery too: when the
market cannot feed the settlement, refusals multiply and slights
compound.

**Suggested gate:** a hunger threshold, e.g. `sim.hunger.desperate`
(adapter default 0.2 — below ~20% hunger the closed sign is
ignored), input to Decide like the other tuning. Moderate hunger
still respects the shut door; only desperate minds push it.

**Verify sentence after the change:** set the market to close ~30
minutes of game time after session start (open 00:00, close 20:30),
with high hunger decay — the close lands mid-flow, in-flight walks
finish to the shut bench, and the log shows `is shut … and blames
the keeper`, a rival bond, and the feud arc.

## The core's 0.8.0 side is built — Scale (hand-over 2026-08-11)

Engine side of 0.8.0 is done and waiting (30/30 suites; design in
Docs/Design/Scale.md). No new components, no schema change — the
co-save record stays v2. Four moves for the adapter:

1. **sim.memory.cap** — a tuning key (default 0 = a mind remembers
   everything, unchanged behavior). Set it in the INI once you've
   measured your in-game memory growth; below it the lowest-weight
   event is evicted on insert, bounding Decide's scans and the fade
   pass. The engine's own soak tests use 64 (decade) and 32 (hour).
2. **FixedStep** — the timing-independent tick. Feed it your real
   frame deltas; it advances whole fixed steps (default 0.1s) and
   returns how many ran. Same seed + same steps = same world
   whatever your frame rate — that's what makes "a year without
   drift" provable on your side too. Update(delta) is unchanged if
   you prefer the raw loop.
3. **TickReport** — pass a non-null pointer to Update (or through
   FixedStep) and get per-pass counts + wall time: minds swept,
   events faded, pairs drifted, ms per pass. Log it once a minute
   in-game to see the cost of your settlement — the engine proves
   the shape, you own the number.
4. **Co-save: nothing to do.** The cap lives in tuning and the
   tick, not the record. Your existing serializers, versioning, and
   v2 record all stand.

The boundary reminder stays: the core is single-threaded and
deterministic by construction. If you take the tick off the game
thread, FixedStep's fixed cadence is what makes that safe — the sim
no longer depends on when you call it, only on how many steps ran.

## The re-roll fix — a field finding, now fixed (2026-08-12)

The adapter's ADR-0029 (its own DecisionLog, 2026-08-12) flagged a
real engine flaw: the tick's per-entity noise — the needs-decay rate
AND Decide's confidence jitter — was derived from the Rng's LIVE
state (`Derive` mixes `m_State` + key). The adapter legitimately
advances the same Rng it passes to Update (births draw between
Ticks), so every entity's noise re-rolled every frame. A mind with
near-tied Rest/Explore needs flipped its intent every tick — 22k
"decides" log lines in three minutes, 75% of the log, the drag
behind the frame hang. The adapter throttled its own log (its fix);
the root cause was the engine's to own.

**Fixed (2026-08-12): `Rng::StableDerive(key)`** — a child stream
anchored to the SEED, never the live state. Same seed + same entity
= same noise, every run, no matter how far the parent has moved.
Both engine call sites (needs decay, Decide jitter) now use it; the
parent advancing between ticks can no longer re-roll a settled mind.
`Derive` is unchanged (state-anchored, documented as such) for
anyone who wants a stream that follows the parent. The co-save is
unaffected — the Rng state in the record still resumes the parent
stream; per-entity noise is now a pure function of (seed, entity),
which is strictly more deterministic.

**Your verify sentence:** with your log throttling in place, watch a
settled mind near the Rest/Explore boundary — it should hold one
intent (Rest or Explore) tick after tick instead of alternating, and
the "decides" lines for it should stop being a per-frame flood. The
engine's proof is the new Jitter suite block: two same-seed worlds,
one whose parent advances three draws between every tick, stay
bit-identical in decay AND decision.

## Include-path reorganization (2026-08-12) — the Simulation folder grew subcategories

The flat `Include/LCE/Simulation/` pile (16 headers, 4 sources) was
split into category folders matching the engine's vocabulary —
`Entity/` (EntityId, EntityRegistry, RegistrySnapshot), `Mind/`
(Needs, Memory, Relationships, Goals), `Society/` (Groups, Traits),
`Decision/` (Behaviour, Outcome, Legacy), and `Substrate/` (Rng,
WorldTime). `Simulation.h` and `SimulationEvents.h` stay at the
Simulation root (the tick and its events — the paths everyone
includes stay put). **Every include path to a moved header changed**
— if your checkout predates this, the adapter's includes were
re-synced in the same change (src/ and Tests/main.cpp); the namespaces
are unchanged, only the file locations. The adapter builds green
(21/21) against the reorganized core.

## The surface is now guarded (2026-08-12)

Two additions protect the SDK surface you build against:

1. **HeaderMapTest** — the engine harness now carries the canonical
   public-header map. A header moved, deleted, or added without
   updating the map fails the harness with the exact path named, and
   every `LCE/...` include referenced anywhere in the engine must
   resolve. Your checkout includes against this map — if the engine's
   layout ever changes again, the harness fails first and the map
   tells you exactly what moved.
2. **LCE Doctor include-layout check** — point the doctor at the
   adapter and it verifies every `LCE/...` include resolves against
   the core it pins. Run it after a core upgrade to catch stale paths
   before the build does.

## The endgame plan — cuts verdicted, plan locked (2026-08-11)

The engine's path to 1.0.0 is locked (Docs/Design/Endgame.md): prove
the seven mod-type patterns as lean samples, freeze the API
mechanically, finish the docs, beta, release. The discipline is "no
new engine surface unless a pattern or an adapter finding names a
real need" (the Death precedent). The adapter's verdicts (from its
own repo's hand-off, same day):

1. **Threading the tick off the game thread** — VERDICT: measure
   first, thread only if the number says so (a TickReport log once a
   minute in-game decides it). Likely: the game thread stays.
2. **C ABI / Papyrus natives** — VERDICT: not before 1.0.0; the C++
   API is the surface.
3. **Networking / replication** — VERDICT: not needed (single-player).
4. **Multi-agent negotiation / deep planning** — VERDICT: deferred.
5. **LCE Studio (GUI)** — VERDICT: post-1.0; the CLI Doctor and the
   log are enough.
6. **Per-fact legacy decay** — VERDICT: only if the soak data or a
   mechanic demands it; nothing in the 0.9.0 plan needs fading
   legacies.
7. **MCM / radio audio** — VERDICT: both deferred past 1.0.0 (the
   INI delivers tuning; captions deliver the radio).
8. **New engine surface** — VERDICT: the disease/health mechanic
   needs **no engine change**. Health is adapter-owned: a co-save
   component (additive, like `name`), hold-then-recover driven by the
   adapter's own tick, the cost expressed through the existing
   Fatigue need (sick minds tire faster and rest). No NeedType::Health,
   no new goals — health is not a drive; every cause (radstorm, bad
   food, wounds, contagion) is already an edge read. The engine's
   Disease & Medicine sample (0.8.3) therefore proves the
   fact-plus-tick recipe with zero new surface.

**Locked from the adapter side:** the seven patterns are all core-side
lean samples (the engine owns the proof; the adapter lives in-game);
point releases run one at a time (0.8.1, then 0.8.2, …), each a
verifiable tag. The adapter's own mod-side plan (its ReleasePlan.md)
versions independently, as always: 0.7.1 Talk, 0.7.2 Rows, 0.7.3
Fights, 0.8.0 Trade with anyone, 0.8.1 Illness & Medicine, 0.9.0 its
release gate, 1.0.0 freeze and ship. The two tracks cross-sync only
through this document and the shared milestones (0.9.0 RC, 1.0.0).

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

The adapter repo carries its own working handoff — its
`Docs/AdapterProject.md` (adapter status, architecture, lifecycle) —
while this engine-side file is the handover record, per Canonical Copy
above.

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

## Adapter 0.7.6 → 0.8.0 — zero new surface (hand-over 2026-08-12)

The adapter closed 0.7.5 (fights + the subtitle path) and designed its
run to Illness (adapter repo, `Docs/Design/FutureStones.md`). The audit
against this repo's public surface is complete: **no Request D.** Five
stones — 0.7.6 fight-feel bug pass, 0.7.7 the birth lifecycle
(pregnancy → birth → growth as edge components on the existing
surface), 0.7.8 baby & kid items via an external mod (usable now,
editable on the author's permission), 0.7.9 bugs & polish, 0.8.0
Illness (the fact-plus-tick `Health` component already answered in the
adapter's Illness.md) — all ride `CreateEntity` / `DestroyEntity` /
`Remember` / `Update` / `GetComponent`, `ReportOutcome`, the EventBus,
Groups/Traits, Legacy, Rng, and WorldTime. The only touchpoints are
read-only: `TickReport` for the 0.7.9 perf sanity pass, and the
adapter's co-save staying additive. The engine tab is free to proceed
with its own 0.8.2+ samples; nothing here blocks or waits on it.

## Adapter 0.8.x → 0.9.x run — reordered, zero new surface (hand-over 2026-08-13)

The adapter closed 0.8.0 (Illness & Medicine) — **verified in-game on
a natural radstorm day**: 73 medicine buys, 4 sick-but-broke resters,
0 deaths; the market-cure fix (retune + hunger counter-toll + the
wired food vector) is committed (`c3e837b`). Its 0.8.1 co-save audit
caught and fixed a real gap of its own: `Health` (0.8.0) and
`Pregnancy`/`BirthDay` (0.7.7) were registered serializers but never
named in the co-save's stable-name table, so a mid-hold illness and
an in-progress pregnancy were both lost on save/load (`CompanionTag`
too, harmless — it re-derives). All four names now ride the adapter's
record; `MidOutbreakSaveTest` locks the round-trip. 25/25 adapter
suites green.

Its run to the Nexus beta is designed (`Docs/Design/Run080.md`) and
was reordered 2026-08-13 — the random-interactions trial moved ahead
of MCM so the tuning page is built once, with the interaction knobs
included if the trial proves:

```
0.8.1 illness field pass (in progress) → 0.8.2 burial →
0.8.3 sick household → 0.8.4 random interactions (the trial) →
0.8.5 MCM + Settings Manager → 0.8.6a the audit →
0.8.6b redefine & loose ends → 0.8.6c scale in the field →
0.9.1a dialog pools → 0.9.1b timings & weights →
0.9.1c babies implemented → 0.9.2a animations →
0.9.2b final touches → 0.9.2c beta on Nexus
```

**Request E: none — confirmed stone by stone.** Every stone rides the
existing contract — `CreateEntity` / `DestroyEntity` / `Remember` /
`Update` / `GetComponent`, `ReportOutcome`, the EventBus,
Groups/Traits, Legacy, Rng, WorldTime, and the shared wallet. The
only touchpoints with this repo are read-only: `TickReport` for
0.8.6c (scale, measured), and the adapter's co-save staying additive
(burial days, medicine stock, and interaction gates are all
adapter-owned state on the adapter's record). The 0.8.4 trial is the
one place a real need could surface — if unprompted interaction ever
needs a new valence it follows the Death precedent (an append-only
`InteractionKind`) and will be named here at that time. This run
supersedes the provisional adapter mapping in the Endgame section
above (2026-08-11), which predated the final 0.7.x ordering; the
engine tab is free to proceed — nothing here blocks or waits on it.

## Adapter freeze sign-off — Request E none at the line (2026-08-13)

The engine's 0.8.4 freeze work (personality tie-break, per-need
metabolism, the Fact kind + label, CompatPolicy) is audited from the
adapter side: **nothing new is needed — now or later.** Every
remaining adapter stone rides the existing contract, and the freeze
protects what the adapter persists (stable InteractionKind ordinals,
additive MemoryEvent fields):

- **0.8.2 burial** — game corpse refs, adapter-owned; no engine surface.
- **0.8.3 sick household** — medicine stock + family care on the
  shared wallet; no engine surface.
- **0.8.4 random interactions (the trial)** — proximity, Say/talk,
  ReportOutcome; any new valence rides Fact + label, no new kinds.
- **0.8.5 MCM** — the adapter builds its own Papyrus surface via
  CommonLibF4; the engine's C ABI / MCM cuts do not block it (no
  engine ask).
- **0.8.6c scale** — TickReport (read-only).
- **0.9.x content** — dialog pools, the birth journey, ESP
  animations: all adapter-side.
- **Post-beta roads** — legacy-fact roads (LeaveLegacy/ReadLegacy
  + weight re-push decay) and Fact + label gates; per-fact legacy
  decay stays cut.
- **Seasons** — SeasonOf (exists).

Adapter commitments at the freeze, for the record:

1. **InteractionKind stays append-only from this side too** — future
   fact-types (plague, blocked roads, market events) ride Fact + a
   label, never a new kind.
2. **MemoryEvent::Label** — the adapter's Memory serializer keeps not
   writing it today (restore yields empty, which is fine); writing it
   later is an adapter record bump, per the CompatPolicy table.
3. **The 0.8.4 tie-break and per-need metabolism are clean for us** —
   the seeded needs carry no exact ties, and the adapter's traits read
   on the hash path, so the JitteredTraits fix never touched our
   values.

The engine can freeze; nothing here blocks or waits on it.
