# Learning Path: What LCE Teaches

**Design Law 003: Teach through code.**

This repository is a working engine, but its real product is understanding.
Every subsystem is a lesson: a small, complete example of a C++ technique,
an architectural decision, and the reason for both. Read in this order and
you will learn more than how LCE works — you will learn how to build
something like it.

The code is the teacher. The comments say *why*, the headers say *what*,
and the docs say *why it exists at all*.

---

## The Big Ideas

Five mental models appear again and again. Learn these and every subsystem
becomes familiar:

1. **Dependencies point downward.** Each layer uses only the layers below it.
   Nothing reaches up, nothing looks sideways. This single rule makes
   circular dependencies impossible (ADR-0019).
2. **Own the interface, not the implementation.** The public API is pure LCE.
   Third-party code hides behind one implementation file. If spdlog
   disappeared, only `Logger.cpp` would change (ADR-0030).
3. **No global state.** No singletons, no reachable statics. Everything is
   owned explicitly and given to whoever needs it (ADR-0014, ADR-0015).
4. **Type erasure.** *Store or dispatch anything, without naming its type.*
   The EventBus keys handlers by `std::type_index`; the ServiceRegistry
   stores services as `std::shared_ptr<void>`. The container forgets the
   type; the caller remembers it.
5. **Composition.** Complex behaviour is built from simple pieces (Law 001).
   The Scheduler is a vector of tasks and a countdown. The EventBus is a map
   of type → handlers. Nothing here is clever; everything is assembled.
6. **Templates — one function, any type.** A template is a function with a
   blank where a type goes: `template <typename T>` means *"the caller tells
   me the type."* `AddComponent<T>` is one piece of code that becomes a real,
   typed function for every component type you use. T is exactly what the
   caller says it is — nothing more. This is the *compile-time* half of type
   erasure; `type_index` and virtuals are the *runtime* half.

---

## The Tour (read in this order)

### 1. Version — `Include/LCE/Version/Version.h`
The whole subsystem is a few `constexpr` constants. No class, no source
file, nothing to link.
**Teaches:** compile-time constants, `inline constexpr`, `std::string_view`,
and header-only as a deliberate zero-cost choice (ADR-0025).

### 2. Logging — `LogLevel.h`, `Logger.h`, `Logger.cpp`
Six free functions. spdlog appears in exactly one `.cpp`.
**Teaches:** the interface/implementation boundary, free functions over
static classes (ADR-0026), and why a public API must never leak its backend.

### 3. Clock — `Clock.h`, `Clock.cpp`
One method: `Elapsed()`. The constructor captures `now()` — the clock owns
its starting point.
**Teaches:** RAII, `std::chrono`, and why there is no `Reset()` — a new
starting point is a new Clock. Simpler, and impossible to get wrong.

### 4. Events — `Event.h`, `EventBus.h`, `EventBus.cpp`
**Teaches:** the observer pattern, type erasure via `std::type_index`,
exact-type dispatch, and reentrancy — handlers are delivered from a
*snapshot*, so a handler may subscribe while an event is being published.

### 5. Scheduler — `Scheduler.h`, `Scheduler.cpp`
**Teaches:** `std::function`, move semantics, and the classic
iterator-invalidation trap. Due callbacks are collected first and invoked
*after* the pass, so a callback that schedules new work cannot corrupt the
loop.

### 6. Task — `Task.h`, `Task.cpp`
The simplest possible unit of deferred work.
**Teaches:** wrapping a `std::function`, moving it into the member, and
guarding against an empty callback.

### 7. Configuration — `Configuration.h`, `Configuration.cpp`
A map of strings to strings.
**Teaches:** `std::string_view` lifetimes. `Get()` returns a *view* into the
map — the map must outlive the view. (What happens if you `Set()` a key and
then read an old view? The map can rehash and the view dangles. That is the
lesson.)

### 8. Service Registry — `Include/LCE/Runtime/ServiceRegistry.h`
The keystone. Header-only, dependency-free.
**Teaches:** template type erasure, why `static_pointer_cast` back to the
real type is safe here, dependency injection, and replaceability. Read
`Docs/Architecture/ServiceRegistry.md` and ADR-0034 alongside it.

### 9. Test Runner — `Tests/Core/TestRunner.cpp`
A hand-rolled harness: every suite is a bool-returning function.
**Teaches:** why the project refuses a test framework (ADR-0010,
ADR-0029 — minimal dependencies), and how little a harness really needs.

### 10. Entity System — `EntityId.h`, `EntityRegistry.h`, `EntityRegistry.cpp`
An entity is an ID, not an object. All data lives in components.
**Teaches:** tagged types (an `EntityId` can never be confused with an
ordinary integer — the compiler rejects the mistake), generational indices
(a stale ID can never alias a reused slot), and templates — `T` is whatever
the caller says it is, and the registry stores components it has never
heard of, erasing the type at runtime (`type_index` + virtuals) whilethe caller keeps typed access at compile time (`T`).

### 11. Registry Snapshot — `RegistrySnapshot.h`, `EntityRegistry.h`
Save/load substrate: the whole registry becomes pure data and back.
**Teaches:** type erasure applied a *third* time — components stay erased,
so the adapter registers a `ComponentSerializer<T>` per type (compile-time
type in, runtime blob out) and the registry never interprets the bytes.
Also: identity as a contract (the snapshot preserves index + generation
exactly, so a save/load can never alias entities), and the free list as a
source of truth (restored slots are alive; new entities reuse only the
dead ones). This is the stone that lets the simulation ride insidea game's save file.

### 12. The Mind — Needs, Memory, Relationships, Goals (`Simulation/Mind/`)
Four components, one idea: **everything a mind is, is data the world can
read and write.**

- **Needs** — drives whose value decays toward 0; the lower the value,
  the more urgent.
- **Memory** — experiences as events. `Weight` is salience: it fades
  each second and is forgotten below the threshold. `Day` stamps when
  it happened. A world fact has an invalid Other — it shapes no
  relationship, but shuts a door (Trade/Social) *while it is
  remembered*.
- **Relationships** — two floats: Disposition (valence) and Trust
  (reliability). Both drift toward neutral; experience refreshes them.
- **Goals** — one active ambition, urgency growing while unserved.

**Teaches:** generic vocabulary (the core never knows what "hunger"
means), components as plain data, and the door-fact trick — the world
communicates availability by *what it remembers*, not by flags.

### 13. Behaviour — Decide (`Simulation/Decision/Behaviour.h`)
Needs become one Intent per mind, every tick. Confidence = urgency +
personality jitter. Near-tied needs resolve by a per-need seeded draw,
not list order — the personality tie-break (0.8.4). Targets come from
memory (ChooseTarget scores remembered others by weight plus
relationship); danger from the strongest remembered wrong
(FindThreat); closed doors from remembered world facts (IsUnavailable).

**Teaches:** a pure function of data (ADR-0026), the "decide" leg of
the loop, and determinism as a feature — same seed + same entity =
same mind.

### 14. The Tick — `Simulation/Simulation.h`
`Update` decays, fades, drifts, grows, and decides — five passes, one
call. `Remember` records an experience; `ReportOutcome` reports how an
executed intent went and closes the loop: **decide → act → observe →
remember → decide**.

**Teaches:** statelessness — time and tuning are inputs, never global
state (ADR-0014); and why the observe leg (outcomes) is what turns a
walk into a learning settler.

### 15. Substrate — Rng, WorldTime (`Simulation/Substrate/`)
Rng: splitmix64, one word of state — save the number, resume the
world. `Derive(key)` gives order-independent child streams;
`StableDerive(key)` anchors to the seed, so a mind's personality can
never be re-rolled by other draws. WorldTime: a day counter and the
seasons derived from it — the calendar memory stamps stand on.

**Teaches:** determinism as a design goal (same seed, same steps, same
world), and derivation over state.

### 16. Society — Groups, Traits (`Simulation/Society/`)
GroupId: an opaque number the world assigns meaning to — a family, a
settlement, a faction. Membership is a component;
`InheritGroupAttitudes` seeds a newcomer with the group's mean
disposition — trust is never inherited, it is earned. Traits: named
floats varied deterministically per entity by `JitteredTraits`; the
world's behaviour tables read them, the core never interprets the
names.

**Teaches:** the vocabulary boundary (the core carries names, never
interprets them), and the echo rule (feelings travel through groups;
trust does not).

### 17. Legacy — Bequeath, InheritMemory, LegacyStore (`Simulation/Decision/Legacy.h`)
Death is three functions and a fact. `Bequeath` passes salient
memories to heirs, fainter. `InheritMemory` passes selected stories a
generation later, aged. `LegacyStore` keeps a registry-level fact
forever — the promise that outlives its maker.

**Teaches:** the day-stamp as the substrate of memory ("the feud is
decades old"), and registry-level state beside entity-level state.

### 18. Observation & Query (`Simulation/SimulationEvents.h`, `EntityRegistry.h`)
The push channel: EntityCreated, IntentProduced, OutcomeRecorded,
RelationshipChanged — games react without polling. Restore does not
re-announce (a loaded world is not a flood). `QueryWhere<T>` reads
deterministically — ascending EntityId order, always.

**Teaches:** push over poll, and iteration order as a contract — the
determinism the save-compat stands on.

### 19. Scale — FixedStep, TickReport, MemoryCap (`Simulation/Simulation.h`)
FixedStep: real frame deltas in, whole fixed steps out — same seed +
same steps = same world at any frame rate. TickReport: the cost of a
settlement, knowable instead of guessed. MemoryCap: a mind can only
hold so much — the lowest-weight event is evicted on insert.

**Teaches:** bounding the hot path by construction, and measurement as
a first-class input.

---

## Exercises (learn by doing)

1. **Dependency injection.** Register a `Configuration` service in a
   `ServiceRegistry` and read a value from a test. Then register a *second*
   Configuration and prove `Get<Configuration>()` returns the newest one —
   replaceable implementations in action.
2. **Reentrancy.** Write an EventBus handler that subscribes while an event
   is being published. Does it see the current event? (No — snapshot.)
   Trace why that is the *safe* behaviour.
3. **Iterator invalidation.** In `Scheduler::Update`, move `callback()`
   back inside the loop and run the reentrancy test. Watch it corrupt —
   then restore the fix.
4. **The boundary.** Replace spdlog with `std::cout` in `Logger.cpp` only.
   Nothing else changes. That is ADR-0030, proven.
5. **Composition.** Build, in a test only, a *delayed task that runs once*:
   a `Scheduler` that fires a `Task`. Two simple things, one behaviour
   (Law 001).
6. **The warning discipline.** Remove `/WX` from `CMakeLists.txt` and write
   code with a warning. Feel the safety net disappear, then restore it.
7. **Templates.** Write a third component type — `Position { float X, Y, Z; }`
   — attach it to an entity and read it back. Notice you wrote zero new
   registry code: the template generated it. That is `T` in action.
8. **Snapshot.** Register a serializer for your `Position` component from
   exercise 7 (three floats — 12 bytes) and prove a round-trip: capture,
   restore into a fresh registry, read the position back. Then remove the
   serializer and capture again — `Position` silently vanishes from the
   snapshot. Data presence decides membership, everywhere in LCE.
9. **Query.** Attach Needs to five entities, decay two of them below the
   rest, and `QueryWhere` "everyone hungry" — prove the result comes
   back in ascending ID order, twice.
10. **Observation.** Subscribe to `IntentProducedEvent` and run one tick:
    every fresh decision arrives on the bus without polling.
11. **Determinism.** Two registries, same seed, same steps via FixedStep:
    capture both, flatten, and prove the snapshots are byte-identical.
12. **The door fact.** Remember `{ invalid, Trade }` on a mind with
    urgent Hunger: Decide should refuse the trip (Explore). Fade the fact
    below the threshold — the market reopens, no script fired.
13. **Traits into decisions.** Give two identical minds different traits
    and bias their needs' decay before the tick (the Weather/Disease
    channel) — watch the same seed diverge into different choices. That
    is the seam 0.8.4's personality tie-break exists for.

---

## The Samples Teach (the SDK course)

Reading teaches structure; running teaches behaviour. The samples in
`Samples/` are the SDK's course, from the smallest mind to the whole
loop:

1. **Sample-Farmer** — the smallest complete mind: one entity, one need,
   one loop. If a subsystem confuses you, this is where it is alone and
   visible.
2. **Sample-Village** — several minds and the relationships between them.
   Watch a grudge steer a villager away from the neighbour who wronged
   them — experience shapes behaviour, knowledge doesn't.
3. **Sample-Market** — the world: day-stamped memories, seasons, weather
   facts. The same market the Fallout 4 adapter runs for real.
4. **Sample-Economy** (0.8.2) — prices are memories: a blight is a fact,
   a delivery is a fact, and the price of bread is a pure function of
   remembered facts. Dynamic pricing with no price field, no ledger, no
   script.
5. **Sample-Legacy** (0.8.2) — death is three functions and a fact:
   Bequeath, InheritMemory, LeaveLegacy — the name that outlives the
   voice.
6. **Sample-Weather** (0.8.2) — a sky that behaves: seasons from the day
   counter; a radstorm makes Safety the loudest voice, so the farmer
   flees the remembered raiders. The sky never tells the mind what to
   do.
7. **Sample-Children** (0.8.2) — a family is a group: inherited
   dispositions, per-child traits, trust earned personally. The sample
   that caught the JitteredTraits bug.
8. **Sample-FactionWars** (0.8.3) — groups are the map, dispositions are
   loyalty: a wrong from a comrade and a kindness from an enemy flip a
   membership, and the new faction's grudges become her own.
9. **Sample-Disease** (0.8.3) — the quarantine loop: a door-fact closes
   the market, the fever takes the appetite, rest becomes the loudest
   voice, and the settlement remembers the outbreak as a fading,
   day-stamped fact.
10. **Sample-Roads** (0.8.3) — routes as legacies: a road's weight IS
    its condition — traffic maintains it, neglect decays it, a storm
    reroutes the caravans.
11. **SampleHost** — the whole loop embedded in a host with *no game at
    all*: the money test live (cheated twice → trades elsewhere), the
    observation bus, and a save that keeps the lesson. This is the proof
    that any engine can embed LCE — read its `main.cpp` as the course.

And when you are ready to embed LCE in something of your own,
[Docs/SDK/Embedding.md](SDK/Embedding.md) is the recipe: the pinned
FetchContent path, the installed `find_package(LCE)` path, and the
0.8.0 runtime recipe (FixedStep + TickReport + sim.memory.cap) — all
verified.

---

## How a New Stone Gets Built

The build rhythm (from the Development Charter):

1. **Design** — write the design document: interface, responsibilities,
   dependencies, test plan.
2. **Review** — the four questions: Can it be simpler? Does it belong?
   Do we need this at all? Will this help build living worlds through
   simulation?
3. **Implement** — headers first, then source. The document is the spec.
4. **Test** — every public method tested; the harness runs on every build.
5. **Document** — update the docs. The why must match the what.
6. **Merge** — tests pass, docs written, design reviewed.

Every accepted decision becomes an ADR — the log records *why* LCE looks the
way it does. Read the Decision Log as history, not rules: each entry is a
lesson about a tradeoff someone already faced.

When you can propose the next stone yourself — design doc written, four
questions answered, tests planned — you are no longer reading LCE. You are
building it.

---

*there is no magic. only systems. — and systems can be understood.*
