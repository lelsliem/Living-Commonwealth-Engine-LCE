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
enum class InteractionKind
{
    Trade, Combat, Aid, Social, Wronged,
    // Weather facts (0.5.x): the sky on a given day — labels, never
    // doors (Decide gates only Trade and Social; invalid Other, no
    // relationship shaped).
    WeatherClear, WeatherOvercast, WeatherRain,
    WeatherFog, WeatherMisty, WeatherRadstorm
};

struct MemoryEvent
{
    EntityId Other;          // who/what it was about (invalid = world fact)
    InteractionKind Kind;    // what happened — core reasons over these
    float Weight;            // how much it matters (salience)
    std::uint64_t Day = 0;   // the world day remembered (0.5.0 WorldTime stamp)
};

// Day anchors facts in time ("day 12 was rainy") — the substrate the
// Legacy stone ("entities remember decades") stands on. Until then the
// tick fades uniformly: Day is a stamp, not a schedule.

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
enum class ActionType { MoveTo, Rest, Socialize, Explore, Work, Flee };
// AcquireFood is expressed as MoveTo a source; Flee means running from
// a remembered threat (0.3.1).

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

---

## 0.3.1 — The Three Honest Candidates (shipped)

Agreed at the 0.3.0 review and proven by test, in the existing suites:

1. **World facts are deficits.** A memory event with an *invalid* Other is
   a world fact: it declares interactions of its kind unavailable *while
   it is remembered*. "The market is closed today" is
   `{ invalid, Trade, weight }`. While it is remembered, a hungry mind
   does not head to a trader; when it fades below the forget threshold,
   the market reopens. The adapter controls duration by re-pushing facts.
   This is the channel the 0.4.0 adapter will use — now proven, not just
   promised.
2. **Safety completes the mind — Flee.** The strongest remembered Wronged
   or Combat event names the threat; the mind flees it. Safety with no
   remembered threat produces no decision — you can't flee from nothing.
3. **Tuning is an input.** The magic numbers moved out of the `.cpp` into
   `SimulationTuning`, a documented struct carrying the 0.3.0 defaults.
   `Update` and `Remember` take it (defaulted, so existing callers are
   untouched); the adapter builds it from the Configuration service via
   `SimulationTuning::FromConfiguration` (0.5.0) — known keys override
   defaults, broken values keep the default, unknown keys are ignored.
   The tick stays stateless — tuning is an argument, never global state
   (ADR-0014). Tests prove the wiring pattern with a real
   `Configuration` instance (Tuning suite, 0.5.0).

---

## 0.5.0 — The Outcome Channel (shipped)

The living loop's final leg, agreed in the 0.5.0 design and proven by
test:

1. **`Outcome` is the structured report.** `OutcomeResult`
   (`Success`/`Partial`/`Failure`) plus the counterparty and kind. The
   adapter calls `ReportOutcome(registry, id, outcome, tuning)` after
   acting on an `Intent` — it answers "how did it actually go?"
2. **Relationships are scaled by result.** For the positive kinds
   (Trade, Aid, Social) a Success builds trust/affection while a Failure
   loses it — the merchant proved unreliable. For the negative kinds
   (Wronged, Combat) a wrong is a wrong: full loss whatever the result
   claims.
3. **Goals get their first consumer.** A Success clears the active goal
   the kind serves (Trade feeds AcquireFood/Prosper, Social/Aid feed
   Socialize, Combat/Wronged feed ReachSafety); a Partial halves its
   urgency; a Failure leaves it to the tick's natural growth.
4. **The intent is consumed.** The executed action concludes; the next
   tick decides fresh with the outcome's memory in place. The loop
   closes: decide → act → observe → remember → decide.

World outcomes (invalid `Other`) record memory only — no relationship
is shaped, no goal is served, same rule as `Remember`'s facts. The
money test in the Outcome suite proves the learning: a settler who
trades well with one merchant prefers them; after being cheated twice,
the decision function chooses the other stall. No script fired.

---

## 0.5.0 — Observation Events (shipped)

Push, not poll. Three event types on the EventBus
(`LCE/Simulation/SimulationEvents.h`):

- **`EntityCreatedEvent`** — published by `CreateEntity`. Snapshot
  restore uses a private path and does **not** publish: loading a
  637-entity co-save is a restore, not a creation flood.
- **`IntentProducedEvent`** — published by the tick for every fresh
  decision; the adapter executes it without polling.
- **`OutcomeRecordedEvent`** — published by `ReportOutcome`; the
  adapter can react immediately (a robbed settler, a failed trade).

The bus is an *input*, never global state (ADR-0014): `Update` and
`ReportOutcome` take an optional `EventBus*` (nullptr = silent),
`EntityRegistry` holds a sink set via `SetEventSink`. All defaulted —
existing callers are untouched.

---

## 0.6.0 — Bond Thresholds & RelationshipChanged (stone 08, shipped)

The fourth observation event — the one the world configures itself.
`SimulationTuning` carries a **bond watch-list**: every
`sim.bond.threshold.<name>` tuning key draws one line across
*disposition* (`sim.bond.threshold.friend = 0.3`,
`sim.bond.threshold.enemy = -0.6`). The default watch-list is empty —
the world must name its own lines; a default would invent vocabulary
(the core knows nothing about "friendship", only that a line it was
told about was crossed). Broken values are ignored; names are sorted so
that several crossings in one mutation arrive in a stable order
(determinism, stone 05).

When an experience — `Remember` or `ReportOutcome` — moves a
relationship's disposition across a listed line, the core publishes
`RelationshipChangedEvent`: `subject`, `other`, `disposition`, `trust`,
`threshold` (the line's name), `day` (the world day of the crossing).

Edge-triggered, not level-triggered: the event fires the *moment* a
line is crossed — a bond formed, a bond soured — and stays silent
while the relationship rests on either side. Crossing is strict
(resting exactly on a line and drifting away is not a crossing), and
drift is deliberately quiet: a bond cooling below a line over time is a
dissolve, not an event — the adapter re-derives bonds from the
relationship state it always has. `Remember` gained a trailing
defaulted `EventBus*` (existing callers are untouched). Proven by the
BondThreshold suite: payloads, both directions, multi-crossing order,
quiet drift, world facts never firing.

---

## 0.6.0 — Society: Groups & Traits (stone 09, shipped)

The layer between the individual and the world. Both pieces stay
world-agnostic; both build on the stone 08 event channel.

**Groups.** `GroupId` — an opaque id the world assigns (a family, a
settlement, a faction) — and the `Groups` membership component. The
component query (`QueryWhere<Groups>`) finds a group's members; there
is no separate group registry, no new global state (ADR-0014). Two
behaviours rest on membership:

- **The echo — trust is earned personally; disposition travels.**
  When an experience (`Remember` or `ReportOutcome`) shapes a
  relationship, every mind sharing a group with the subject feels a
  fainter version of the same feeling toward the Other, at
  `sim.group.inheritance` strength (default 0.5). "They wronged my
  brother." The echo shapes feelings, not memory — memory is
  personal, and the `RelationshipChangedEvent` is how a mate learns
  the news. A wrong done to one settler turns a whole settlement
  cold, and every crossing publishes: one outcome, many minds.
- **`InheritGroupAttitudes`** — the function the world calls when an
  entity joins a group: the newcomer's disposition toward everyone
  the group collectively knows becomes the group's *mean*
  disposition — then their own experiences diverge it. Personal
  knowledge always beats inherited (an existing relationship is left
  untouched); trust is never inherited; quiet by design (seeding is
  not an event, the same rule as drift). The mean is derived from
  the members' stores, never stored separately.

**Traits.** The personality substrate. `Traits` is a named-float
component — "boldness", "sociability", the vocabulary is the
world's; the core only carries it. `JitteredTraits(base, id, rng,
spread)` derives per-entity variation from the seeded RNG: same seed
+ same entity = same personality, every run, the parent stream never
advances; zero spread reproduces the base exactly. The *influence* of
a trait is the world's business (its behaviour tables read the
component) — the same boundary as the species split; the core's
`Decide` stays vocabulary-free. A co-save component like any other:
persisted, queried, restored.

Proven by the Groups suite (the rally: one wrong, every member's
crossing fires; trust never echoing; inheritance means and
boundaries) and the Traits suite (divergence, determinism, fallback,
identity, round trip) — 28/28 suites total.

---

## 0.5.0 — Query Surface (shipped)

`EntityRegistry::QueryWhere<T>(predicate)` — filtered reads with
**documented iteration order**: the returned `std::vector<EntityId>` is
sorted ascending by `EntityId::Value()`. The underlying store is an
unordered map that promises nothing about order; the query promises
everything, so the same query returns the same result on every run —
the determinism hook seeded RNG and save-compat stand on.

The predicate receives `(EntityId, const T&)` — the id lets
cross-component filters reach the registry by capture ("settlers who
remember the raid" queries Memory and checks Needs). The component is
`const`: a query reads, never mutates. Empty store or no match →
empty vector. Stateless and pure: a query is a function of the
registry and the predicate (ADR-0014).

---

## 0.5.0 — Seeded RNG + Determinism (shipped)

`LCE::Simulation::Rng` — a splitmix64 generator whose **entire state
is one 64-bit word**. That single word is the save/load contract: the
adapter persists `State()` in its co-save record and a restored world
resumes the exact same randomness. Same seed, same sequence, forever.

The subtle piece is `Derive(key)`: it returns a **child stream** mixed
from the current state and the key **without advancing the parent**.
The tick derives each entity's personality jitter from its ID, so the
unordered store iteration can never leak into the results — same seed
+ same entity = same jitter, every run, whatever order the store
visits. `Decide` and `Update` take an optional `const Rng*` (defaulted
— existing callers are untouched); nullptr keeps the deterministic
id-hash fallback, so behaviour without a seed is unchanged.

Why this matters: the tick is now deterministic *under a seed*. Two
identical worlds with the same seed produce bit-identical intents
(proven by the Rng suite); a different seed changes personality but
never the action a hungry farmer chooses. This is the substrate
save/load determinism and, later, 0.6.0 traits stand on.

---

## 0.5.0 — World Calendar + Memory Timestamps (shipped)

The last stone of the boundary contract. `WorldTime` is a day counter
the adapter drives from the game's clock; `SeasonOf(day)` derives the
season (four 90-day seasons in a 360-day year). `MemoryEvent::Day`
anchors a memory to the world day it happened: `Remember` and
`ReportOutcome` stamp it from the passed `WorldTime`, and **a
caller-set day wins** — the adapter can report a historical event
while passing today. The age of a fact is `now.Day - event.Day`.

The timestamp is data, so it must survive a save: the Memory
serializer now carries `Day` (the adapter's co-save record must too —
its format version bumps). This is the substrate 0.7.0 Legacy stands
on: "entities remember decades", not ticks.

---

## 0.5.0 — Per-Mind Decay Jitter (shipped)

The herd, broken at the source. Identical minds used to share one
clock — the same `DecayRate`, the same start value, the same
threshold — so a settlement marched to the bench in lockstep. Under a
seeded `Rng`, each entity's needs now decay at its own rate, derived
from its ID:

```cpp
const auto rate = rng->Derive(id.Value()).NextFloat(
    1.0f - tuning.NeedJitter, 1.0f + tuning.NeedJitter);
need.Value -= need.DecayRate * delta * rate;
```

Same seed + same entity = same metabolism, every run; the parent
stream never advances (the adapter owns world-level randomness); one
number in the co-save resumes it. `sim.jitter` (default 0.15) is the
modder's knob for how strongly minds diverge — `0` turns the spread
off entirely. Without an `Rng` the rate is exactly 1.0, so no
existing caller is affected.

Proven by the Jitter suite: two identical minds under one seed
diverge over ten ticks; two identical worlds under the same seed stay
bit-identical (determinism untouched); no Rng → behavior unchanged;
`sim.jitter = 0` → no divergence even with a seed.
