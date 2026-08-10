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
dead ones). This is the stone that lets the simulation ride inside a
game's save file.

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

---

## The Samples Teach (the 0.5.0 SDK)

Reading teaches structure; running teaches behaviour. The samples in
`Samples/` are the SDK's course, in three sizes:

1. **Sample-Farmer** — the smallest complete mind: one entity, one need,
   one goal, one loop. If a subsystem confuses you, this is where it is
   alone and visible.
2. **Sample-Village** — several minds and the relationships between them.
   Watch a grudge steer a villager away from the neighbour who wronged
   them — experience shapes behaviour, knowledge doesn't.
3. **Sample-Market** — the world: day-stamped memories, seasons, weather
   facts. The same market the Fallout 4 adapter runs for real.
4. **SampleHost** — the whole loop embedded in a host with *no game at
   all*: the money test live (cheated twice → trades elsewhere), the
   observation bus, and a save that keeps the lesson. This is the proof
   that any engine can embed LCE — read its `main.cpp` as the course.

And when you are ready to embed LCE in something of your own,
[Docs/SDK/Embedding.md](SDK/Embedding.md) is the recipe: the pinned
FetchContent path and the installed `find_package(LCE)` path, both
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
