# Simulation (0.3.0) — "The Mind"

**Milestone:** 0.3.0 — Alpha · Simulation
**Status:** Implemented and tested
**Related ADRs:** ADR-0008 (Simulation Over Scripting), ADR-0024 (Adapters
Translate, Don't Simulate), ADR-0014 (No Global State), Law 001 (Simple
things; compose the complex)

---

## The Spec Is a Farmer

From the README:

> A farmer doesn't go to market because a quest script fired — they go
> because they're hungry, they know the merchant, and they understand the
> road.

That sentence is the test plan. 0.3.0 must make it literally true: the farmer
**is hungry** (a Need), **knows the merchant** (Memory), **likes them**
(Relationship), and **decides to go** (Behaviour) — and out comes an
**Intent** the Fallout 4 adapter can execute. No quest script anywhere.

This is the milestone where the engine proves its thesis.

---

## The Shape: Data + One Decision

Per Law 001, 0.3.0 is deliberately small. Four components carry data;
behaviour is one stateless function; the output is one small type.

```
Needs        — what the entity wants, urgently        (component)
Memory       — what the entity has experienced        (component)
Relationships— how the entity feels about others      (component)
Goals        — what the entity wants, long-term       (component)
Behaviour    — turns all of the above into one action (stateless function)
Intent       — the chosen action, handed to the game  (the output)
```

The entity (0.2.0) is the ID; these components are what lives on it; the
tick is what breathes.

---

## The Pieces

### 1. Needs — the urgent drives

```cpp
enum class NeedType { Hunger, Fatigue, Social, Safety, Comfort };

struct Need
{
    NeedType Type;
    float Value = 1.0f;    // 1 = satisfied ... 0 = deprived
    float DecayRate;       // per second of simulation time
};

struct Needs
{
    std::vector<Need> List;
};
```

- Needs **decay over time**. Hunger falls; the farmer gets hungry.
- The core defines *generic* need types; the adapter maps them to game
  mechanics (Hunger → an in-game need stat, or a custom one).
- The entity with no `Needs` component is not simulated. A rock has no
  needs. That is the ECS principle: data presence decides membership.

### 2. Memory — experience

```cpp
enum class InteractionKind { Trade, Combat, Aid, Social, Wronged };

struct MemoryEvent
{
    EntityId Other;          // who/what it was about (invalid = world fact)
    InteractionKind Kind;    // what happened — core reasons over these
    float Weight;            // how much it matters (salience)
};

// No timestamp in 0.3.0: salience (Weight) is the sole aging mechanism.
// Time returns when "entities remember decades" needs it.

struct Memory
{
    std::vector<MemoryEvent> Events;
};
```

- Memory **decays**: salience fades over time; events below a threshold are
  forgotten. Reinforcement (a repeat interaction) restores salience.
- Memory is the raw material of decisions. No memory of the merchant → no
  reason to choose their stall.
- `InteractionKind` is core-defined so the *simulation* can reason over it
  (a Trade memory raises trust); the adapter translates the events.

### 3. Relationships — feelings toward others

```cpp
struct Relationship
{
    EntityId Other;
    float Disposition;    // -1 (hate) ... +1 (love), 0 neutral
    float Trust;          // how reliable they have proven to be
};

struct Relationships
{
    std::unordered_map<EntityId, Relationship> ByEntity;
};
```

- Relationships **drift toward baseline** over time and shift with
  remembered interactions: repeated Trade memories raise Trust; a Wronged
  memory sinks Disposition.
- The farmer chooses the merchant they *know and trust* — not a stranger.

### 4. Goals — the long horizon

```cpp
enum class GoalType { AcquireFood, ReachSafety, Socialize, Prosper };

struct Goal
{
    GoalType Type;
    float Urgency;        // grows while unsatisfied
};

struct Goals
{
    std::vector<Goal> List;
};
```

- Needs are urgent and short-horizon (hungry *now*). Goals are persistent
  (become prosperous *this season*). Urgency grows while a goal goes
  unserved and feeds the decision function.
- **This is the most cuttable piece of the milestone.** If the four
  questions say simpler, Goals shrinks to one active goal per entity or is
  deferred wholesale — needs alone can drive convincing behaviour.

### 5. Behaviour — the decision (stateless)

```cpp
enum class ActionType { MoveTo, Rest, Socialize, Explore, Work };
// Trimmed to what Decide can produce today. Flee arrives with danger
// awareness (a later stone); AcquireFood is expressed as MoveTo a source.

struct Intent
{
    EntityId Actor;
    ActionType Action;
    EntityId Target;      // may be invalid
    float Confidence;     // how strongly the entity wants this
};
```

The decision function:

```cpp
// Reads the entity's components and returns the action it most wants.
std::optional<Intent> Decide(const EntityRegistry& registry, EntityId id);
```

- Each candidate action is **scored**: how urgent the need it satisfies,
  what memory supports it, which relationship favours it, which goal it
  serves. Highest score wins.
- A small **noise term** breaks ties, so two identical farmers choose
  differently — personality from one random number.
- `Decide` is stateless (ADR-0026): no class, just a function of data.
- The core never names a game action — `MoveTo` a target the adapter
  resolves into "walk along the road to town."

### 6. The tick

```cpp
// One simulation step: decay needs, fade memory, drift relationships,
// refresh goals, then decide one Intent per simulated entity.
void Update(EntityRegistry& registry, double deltaSeconds);
```

The adapter calls `Update` each game tick (0.4.0); tests call it directly.
Time is an input, never global state (ADR-0014).

---

## The Farmer's Evening (walkthrough)

1. `Update` decays the farmer's `Hunger` below 0.4 — hungry.
2. Memory fades nothing important: the merchant at the market is still
   remembered (Weight high).
3. The relationship with the merchant drifts slightly toward baseline but
   stays positive from a season of fair trades.
4. `Decide` scores candidates: *Stay home* (low — hunger urgent),
   *Go to market* (high — satisfies hunger + memory says the market has
   food + trust favours the merchant), *Flee* (zero — no danger).
5. Out comes `Intent{ farmer, MoveTo, merchant, 0.82 }`.
6. The adapter walks the farmer to the market. No script fired anywhere.

The test for this walkthrough is the milestone's proof.

---

## Prerequisite: the registry must be sweepable

`Decide` and `Update` must *find* the entities that have components. Today
the registry can `GetComponent(id)` but cannot enumerate. 0.3.0 needs a
small extension:

```cpp
// Visit every entity that has a component of type T.
template <typename T, typename F>
void EntityRegistry::ForEachWithComponent(F&& function);
```

The component stores already hold the data; this exposes it. A tiny
addition to 0.2.x — but a hard prerequisite for simulation.

---

## Test Plan (and the harness upgrade)

The suites are named after the story:

| Suite | Proves |
|-------|--------|
| `NeedsTest` | Decay over ticks; no `Needs` component → untouched. |
| `MemoryTest` | Events recorded; salience fades; forgotten below threshold; reinforcement restores. |
| `RelationshipsTest` | Disposition shifts with remembered interactions; drift toward baseline. |
| `GoalsTest` | Urgency grows while unsatisfied; feeds `Decide`. |
| `BehaviourTest` | **The farmer test**: hungry + memory + trust → `Intent{MoveTo, merchant}`. Plus: no memory → weaker/different choice; noise breaks ties. |
| `SimulationTickTest` | One `Update` steps all systems coherently. |

**Harness upgrade (your ask):** the runner currently prints six log lines
and one "All passed." By 0.3.0 it will print every suite by name:

```
[ RUN  ] LoggingTest
[  OK  ] LoggingTest
[ RUN  ] BehaviourTest
[ FAIL ] BehaviourTest — expected MoveTo, got Rest
...
7/7 suites passed.
```

Each suite becomes a named entry; failures name the suite and the reason.
This is a small change to `TestRunner.cpp` and it makes the harness worth
reading — because from 0.4.0 on, the *real* test is Fallout 4, and the
harness must teach before the game does.

---

## Discussion — What's Missing, What's Not Needed

### Missing (proposed additions to the checklist)

1. **Intent** — behaviour must *output* something. The roadmap's five items
   are all inputs; the output is the handshake with the adapter. Without
   it, Behaviour is a thought with no action.
2. **Registry iteration** — systems must find their subjects. A
   prerequisite, not a luxury.
3. **Time as an input** — needs decay "over time"; the tick needs a delta.
   Design consequence, not a component.
4. **Noise** — one random term gives personality and breaks determinism
   deadlocks. One line, large payoff.
5. **World context** — memory alone is blind: the farmer also needs to
   know *the market is open today*. Proposal: the adapter feeds world
   facts into memory as events (a core-neutral channel). The core never
   queries the world directly — the adapter pushes facts in.
6. **Harness output upgrade** — named suites, per-suite results.

### Not needed (or cuttable)

1. **Goals** — the most cuttable. Needs alone produce credible behaviour;
   Goals add long-horizon direction. Keep a minimal version (one active
   goal + urgency) or defer the richness to a later milestone.
2. **Relationships as a derived view** — a relationship is arguably just
   aggregated memory (count trades, weight wrongs). Deriving it would
   remove a component. *Verdict: keep it as a component.* Cached and
   trivial to query; derivation is an optimization we can earn later.
3. **Locations in core** — `MoveTo` targets an entity, not coordinates.
   The adapter resolves the road. Keeping coordinates out of the core
   preserves the boundary; the core never needs to know where town is.

---

## Decisions Made (reviewed with the project leads)

- **Goals: kept minimal** — one active goal + urgency. Richer goal
  planning is deferred.
- **Intent added** as the behaviour output — and as a component: an entity
  with a `Needs` component is a mind, and each tick its `Intent` is
  recomputed (or removed). The adapter reads it.
- **Registry iteration added** (`ForEachWithComponent<T>`) — systems must
  be able to find their subjects. Prerequisite, now named.
- **Harness upgrade added** — named suites, per-suite PASS/FAIL.
- **World facts enter through memory** — the adapter pushes them in; the
  core never queries the world.
- **No locations in core** — intents target entities; the adapter resolves
  the road.
