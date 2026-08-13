# Changelog

All notable changes to the Living Commonwealth Engine (LCE).

## [0.8.4] — 2026-08-13 — The freeze work begins: personality into decisions

The first stones of the API Freeze, each a schema or behaviour decision
made deliberately while the surface can still change.

**The personality tie-break.** A mind with two near-tied needs was
decided by list order — whichever need the world happened to push
first. `Decide` now resolves needs within a small band (0.05) of the
most urgent by a per-need seeded draw: same seed + same entity + same
needs = same choice, every run, and the choice is the same whatever
order the needs are listed in (the QueryWhere discipline). This is the
seam a world's traits multiply into — a bold mind's Safety can win its
attention over a barely-more-urgent Hunger. Proven by a BehaviourTest
block: the same entity with the same needs in both list orders makes
the same decision, and the same seed re-rolls identically.

**Per-need metabolism.** Needs decayed at one shared rate per entity;
now each need decays at its own seeded rate, keyed on the need TYPE —
Hunger and Safety metabolize differently. Same seed + same entity +
same need = same rate, every tick; the key is never the list index,
so identical needs listed differently metabolize identically. Without
an Rng the rate is exactly 1.0 — behaviour unchanged for every
existing caller.

**The Fact kind.** `InteractionKind::Fact` + `MemoryEvent::Label` —
the world's own name for a fact ("radstorm", "plague", "the old road
must hold") rides the event; the core fades it, forgets it, and never
interprets it. The weather kinds and Death stay in the enum (the
adapter's co-save writes raw ordinals — append-only, never removed);
new fact-types become labels, not enum entries. Proven by a Snapshot
round-trip: a labeled Fact survives capture and restore when the
world's serializer chooses to carry the label.

**Compat policy** (`Docs/Design/CompatPolicy.md`): what is stable,
what is append-only, what the world owns, and what breaks when — the
freeze contract written down.

**The surface-stability test (SurfaceTest, the freeze's teeth).**
HeaderMapTest froze the FILE map (every path a consumer can include);
SurfaceTest freezes the DECLARATIONS inside those headers. Every
public enum ordinal (InteractionKind, ActionType, NeedType, GoalType,
OutcomeResult, Season, LogLevel — the ordinals the adapter's co-save
writes raw), every public struct field type, and every public member
and free-function signature — each pinned by a `static_assert` that
names the declaration in its message. The freeze is enforced at
compile time: the harness cannot even BUILD against a drifted surface,
and the error says exactly what moved. Size guards pin the
ABI-stable co-save structs (Intent, Outcome, Need, Goal, Relationship,
WorldTime); string-bearing structs are pinned by field type, with
additive change governed by the append-only compat policy. The suite
was proven live: a simulated `InteractionKind::Fact` ordinal shift
failed the build with `static_assert failed: 'InteractionKind::Fact
ordinal (co-save critical - append-only)'`, then restored green.

**The public-header audit (the box's last item).** Every header in
`Include/LCE/` was read fresh and every doc claim checked against its
implementation. The behaviour docs all held — Update's pass order,
Remember's stamping, ReportOutcome's four steps, Bequeath/InheritMemory,
FromConfiguration's keys. What the audit found and fixed:

- **The Goals doc lie.** Goals.h claimed urgency "feeds the decision
  function" — but `Decide` never reads Goals. Now documented honestly
  from both sides (Goals.h and Behaviour.h): Decide reads needs only;
  goals influence through the world's planning layer biasing needs
  before the tick, the same channel Weather and Disease use.
  ReportOutcome serves and frustrates; the ambition is the world's to
  own. Zero code change — the behaviour was already the contract.
- **Logger.h's copy-paste Purpose lie** — it claimed to "provide
  compile-time version information" (Version.h's text). Now describes
  the logging interface; the doubled separator is gone.
- **Seven malformed banners regenerated** to the uniform template
  (Event, Logger, LogLevel, Version, Clock, Task, Configuration):
  quote lines that overflowed the box (the 99-bugs lyric, the
  Michelangelo line, the Covey attribution) now fit on one line, the
  doubled-quote in Event.h is gone, and the trailing whitespace is
  cleaned. New one-liners: "99 little bugs in the code — and they're
  all mine.", "Every block of stone has a statue inside it.", "The
  key is in not spending time, but in investing it.", "Simple things
  should be simple; complex things composed from them.", "My life is a
  bad config file — full of defaults I never agreed to."
- **`/// <summary>` stragglers** (Event.h, LogLevel.h) converted to
  the house `//-----` style.

Zero API change — the freeze surface (SurfaceTest) is untouched. The
freeze is now in force. **32/32 suites, eleven samples clean.**

## [0.8.3] — 2026-08-13 — The three harder pattern samples

**Faction Wars (SAMPLE 8).** Territory, sieges, and diplomacy as
groups and dispositions. `LCE.SampleFactionWars` shows a wrong from a
comrade and a kindness from an enemy accumulating into a crossing:
mara's disposition toward the ally sinks (-0.50) while the enemy
diplomat rises (+0.20), the world reads the crossing, the membership
flips, and InheritGroupAttitudes makes the new faction's grudges her
own — the decision follows the feelings (Socialize toward the
former enemy).

**Disease (SAMPLE 9).** Outbreaks as facts and ticks, honoring the
adapter's 0.8.0 verdict (Health is adapter-owned). `LCE.SampleDisease`
shows the loop the core supplies: a quarantine is a world fact with
an invalid Other — the Trade door closes while it is remembered and
reopens the moment it fades (no script ordered a halt); the sick
mind's toll is Fatigue held urgent (the fever takes the appetite,
so rest becomes the loudest voice); recovery is rest, and the
settlement remembers the outbreak as a fading, day-stamped fact.

**Roads (SAMPLE 10).** Routes that improve with traffic and degrade
with neglect, as LegacyStore facts. `LCE.SampleRoads` shows a road's
Weight IS its condition: use maintains it, weather wears it, the day
stamp moves only with use — so the neglected road decays and the
wrecked road falls out of the world's books while the young road
grows under the rerouted caravans.

All three zero new engine surface. **31/31 suites, eleven samples
clean.** Docs updated (roadmap, endgame, handoff).

## [0.8.2] — 2026-08-13 — The four pattern samples

**The Economy (SAMPLE 4).** Proof that a whole living economy needs
zero new engine surface. `LCE.SampleEconomy` shows dynamic pricing,
supply chains, trade routes, and market events as pure memory: the
price of bread is what the market remembers about last harvest. A
delivery is a memory (supply, cheaper); a blight is a memory
(scarcity, dearer); the price is a pure function of remembered facts,
so it spikes with the blight (9→14 caps) and fades back as the
memory dies — no ledger, no price field, no script. The route to
market is Trust, remembered (six fair trades, 0.63 Trust). The
supply chain is a chain of memories.

**Legacy (SAMPLE 5).** Death is three functions and a fact.
`LCE.SampleLegacy` shows Bequeath (salient facts pass to the heir,
fainter — a passing kindness below the floor dies with its owner),
LeaveLegacy (the old bridge survives the keeper), and InheritMemory
(a generation later, only the recent and the wanted travel — the old
feud and the grief stay with the dead).

**Weather (SAMPLE 6).** A sky that behaves. `LCE.SampleWeather`
shows the calendar (seasons derived from the day alone) and weather
as day-stamped facts that shape need: a radstorm makes safety the
loudest voice, so the farmer flees the remembered raiders; clear
skies leave hunger speaking, so the farmer walks to market. The sky
never tells the mind what to do — it changes which need is urgent.

**Children (SAMPLE 7).** A family is a group. `LCE.SampleChildren`
shows birth (join the family group), InheritGroupAttitudes (the child
inherits the family's mean disposition — trust is never inherited),
JitteredTraits (per-child personality from the seeded RNG), and
personal experience beating inherited (one kind act warms the
inherited distrust; one fair trade earns the first trust).

**One engine fix fell out of the Children sample.** `JitteredTraits`'
RNG path re-derived the child stream per trait and took only its first
draw, so every trait of an entity came out identical. It now derives
one child stream and advances it per trait — boldness and sociability
are different draws again. Regression assertion added to TraitsTest.

All four built entirely on the public surface: Memory facts,
ReportOutcome, relationships, groups, traits, the calendar, Bequeath
— the same loop every adapter walks.

## [0.8.1] — 2026-08-12 — Housekeeping & the re-roll fix

Three field fixes and housekeeping changes landed after 0.8.0, each
verified before the next began; the version is bumped to 0.8.1-alpha.

**The re-roll fix (field finding from the adapter).** The adapter's
in-game hunt (ADR-0029) found the tick's per-entity noise followed the
Rng's LIVE state, so its births — legitimately drawn from the same
Rng it passes to Update — re-rolled every mind's metabolism and
confidence every frame. A near-tied Rest/Explore mind flipped its
intent every tick (22k log lines in three minutes, the drag behind
the frame hang). The adapter throttled its log; the engine fixed the
root cause.

- `Rng::StableDerive(key)` — a child stream anchored to the SEED,
  never the live state. Same seed + same entity = same noise every
  run, however far the parent has advanced. `Derive` is unchanged
  (state-anchored, documented as such).
- Both per-entity call sites (needs-decay rate, Decide's confidence
  jitter) now use `StableDerive` — a settled mind rests, not
  re-rolls. Backward compatible: with a parent that never advances
  (all engine tests) the two are byte-identical.
- Proven by the new Jitter block: two same-seed worlds, one whose
  parent advances three draws between every tick, stay bit-identical
  in decay AND decision. 30/30 suites green at the time.

**Simulation folder reorganized.** The flat `Include/LCE/Simulation/`
and `Source/Simulation/` piles were split into category subfolders:
`Entity/` (EntityId, EntityRegistry, RegistrySnapshot), `Mind/`
(Needs, Memory, Relationships, Goals), `Society/` (Groups, Traits),
`Decision/` (Behaviour, Outcome, Legacy), and `Substrate/` (Rng,
WorldTime). `Simulation.h` / `SimulationEvents.h` stay at the
Simulation root — the tick and its events keep their paths. No
namespaces changed, no API changed — only file locations and the
include paths to them. Every engine and adapter reference re-synced;
the adapter rebuilt green (21/21) against the reorganized core.

**The surface is now guarded.** Two additions make the SDK's public
surface self-checking — the mechanical teeth the 0.8.4 API freeze
stands on:

- **HeaderMapTest (new suite, 31/31)** — the canonical public-header
  map is now frozen in the harness. Any header moved, deleted, or
  added without updating the map fails the run with the exact path
  named; a second sweep resolves every `LCE/...` include referenced
  anywhere in the engine.
- **LCE Doctor: include layout check** — the doctor now verifies that
  every `LCE/...` include a project references resolves to a real
  header in the core it pins (a moved header is a build break the
  doctor names first). The SDK contract grew to six checks.

## [0.8.0] — 2026-08-11 — Scale · "The Settlement Survives"

Engine side shipped (design locked the same day —
Docs/Design/Scale.md); the adapter's in-game verification is the
remaining gate.

- TickReport (stone 13) — the cost of a settlement is knowable:
  per-pass counts and wall time, opt-in; nullptr (the default)
  measures nothing. The documented model: per tick ~Σ over minds of
  (O(events) + O(relationships)) + O(population).
- sim.memory.cap (stone 14a) — a mind can only hold so much: the
  lowest-weight event is evicted on insert (ties → oldest),
  bounding Decide's scans and the fade pass. Default 0 = unbounded,
  unchanged behavior; the world tunes it.
- FixedStep (stone 14b) — the timing-independent tick: real frame
  deltas in, whole fixed steps out. Same seed + same steps = same
  world at any frame rate. Update(delta) stays the raw primitive.
- Soak tests (stone 15) — a decade at day-steps and an hour at the
  real cadence: no NaN, no entity creep, memory bounded.
- Save/load at scale (stone 16) — 5000 minds round-trip
  byte-exactly; 207 bytes per mind documented.
- Determinism at scale (stone 17) — two worlds, same seed, 1000
  minds: flattened snapshots byte-for-byte identical.
- 30/30 test suites green. Backward compatible: Update gained a
  defaulted TickReport* parameter; every existing caller builds
  untouched.

## [0.7.0] — 2026-08-11 — Legacy · "The Debt to the Past"

Engine and adapter shipped together and verified in-game
(2026-08-11): the feud chain ran in the wild — shut stalls, desperate
arrivals, blame, rival bonds, mediated grudges.

- Bequeath (stone 10) — what an entity bequeaths as it goes: the
  world names the heirs; the core keeps facts at or above
  sim.legacy.bequestFloor, scaled by sim.legacy.inheritanceScale,
  their own world day intact. Append, never overwrite;
  deterministic heir order. Proven by the Bequeath suite.
- InheritMemory (stone 11) — descendants inherit memory,
  selectively: the world's predicate selects; the core scales and
  ages (sim.legacy.maxAgeDays). The grandson proof is the
  Inheritance suite's integration block — the feud outlived its
  owner.
- Legacy as world fact (stone 12) — LegacyFact + LegacyStore:
  LeaveLegacy/ReadLegacy/ForgetLegacy, permanent until the world
  deletes, riding the co-save through the world's serializer
  (snapshot schema v2). Proven by the WorldLegacy suite.
- sim.hunger.desperate (field fix, the adapter's handover `81cfe48`)
  — below the threshold a remembered Trade world fact no longer
  blocks the trip: a starving mind pushes the shut door, so an
  arrival can land on a closed market and the refusal can happen.
  Default 0.0 = never desperate, existing behavior untouched; the
  adapter sets 0.2 in its INI. Proven by the Behaviour and Tuning
  suites and verified in-game by the adapter (the feud's gate, now
  live).
- 28/28 test suites green.
Each entry is a released milestone — see Docs/Roadmap.md for the full
arc and Docs/milestone.md for the in-flight story.

## [0.6.0] — 2026-08-11 — Society · "The Bonds Between Minds"

Shipped with The-Commonwealth-Lives 0.6.0-beta (the adapter), verified
in-game the same day: bonds form, couples marry into households, deaths
spread as gossip, sim-only children are born and grow.

- Bond thresholds + RelationshipChangedEvent (stone 08, the adapter's
  Request A) — the world names its own bond lines
  (sim.bond.threshold.<name>); an experience crossing one publishes the
  moment, edge-triggered and drift-quiet; payload: subject, other,
  disposition, trust, threshold name, world day. Proven by the
  BondThreshold suite.
- Configuration::ForEach — lets tuning discover the threshold list.
- Groups (stone 09) — GroupId + the Groups membership component; the
  echo: trust is earned personally, disposition travels to group-mates
  at sim.group.inheritance; InheritGroupAttitudes: feelings inherit
  from the group, then diverge. Proven by the Groups suite.
- Traits (stone 09) — named-float personality component with
  JitteredTraits: deterministic per-entity variation under the seeded
  RNG; the influence is the world's to apply. Proven by the Traits
  suite.
- InteractionKind::Death — the core names the fact so adapters can
  record deaths in memory; a fact, never a door (Decide gates only
  Trade and Social).
- Version bumped to 0.6.0-alpha. 25/25 test suites green; Remember
  gained a defaulted EventBus* — every existing caller builds
  untouched.

## [0.5.0] — 2026-08-10 — SDK & Samples · "The Consumable Engine"

- The complete boundary contract — the decide → act → observe →
  remember loop: the Outcome channel (ReportOutcome); observation
  events (EntityCreated, IntentProduced, OutcomeRecorded); the query
  surface (QueryWhere<T>, ascending EntityId order);
  SimulationTuning::FromConfiguration; the seeded Rng (splitmix64,
  Derive for order-independent noise); WorldTime with seasons and
  memory day stamps; per-mind decay jitter (sim.jitter).
- SDK: the Sample Host (the money test live — fair twice, cheated
  twice, then Bellamy), Sample Modules (farmer, village, market), LCE
  Doctor (the CLI), packaging (install targets + find_package(LCE),
  verified end to end), and Docs/SDK/Embedding.md as the official
  recipe.
- Version bumped to 0.5.0-alpha; both GitHub repos live and tagged
  v0.5.0. The adapter (The-Commonwealth-Lives) verified the full loop
  in-game.

## [0.4.0] — Platform Integration — the Fallout 4 adapter

- The co-save substrate: RegistrySnapshot — Capture/Restore/Clear with
  registered component serializers; the durable record's type names and
  versioning are the adapter's to own. Proven by the Snapshot suite.
- The adapter (separate repo): the F4SE plugin scaffold, entity ↔ form
  translation, the intent executor, co-save integration (record v4),
  and the first in-game test — settlers with needs in Fallout 4.

## [0.3.0] — Simulation

- EntityRegistry, Needs, Memory, Relationships, Goals, Behaviour (the
  Decide loop), and SimulationTick — the seven suites that made the
  tick real. 0.3.1 was the polish pass: banner headers, doc truth, and
  the first honest commit discipline.

## [0.2.0] — Entity System

- The component architecture: the EntityRegistry and the components
  the simulation composes from (the T-templated adapter pattern).

## [0.1.0] — Core Runtime (Services)

- The service layer: Configuration, Logging (spdlog), EventBus, Clock,
  Scheduler, Task, ServiceRegistry, and compile-time Version — the
  substrate every later milestone stands on.

## [0.0.x] — Foundation

- The blueprint: vision, philosophy, architecture, the development
  charter, and the decision log (ADR-0001 onward).
