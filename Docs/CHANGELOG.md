# Changelog

All notable changes to the Living Commonwealth Engine (LCE).

## [0.8.2] — 2026-08-13 — The Economy sample

**The Economy (SAMPLE 4).** The first of the 0.8.2 pattern samples —
proof that a whole living economy needs zero new engine surface.
`LCE.SampleEconomy` shows dynamic pricing, supply chains, trade
routes, and market events as pure memory: the price of bread is what
the market remembers about last harvest. A delivery is a memory
(supply, cheaper); a blight is a memory (scarcity, dearer); the price
is a pure function of remembered facts, so it spikes with the blight
(9→14 caps) and fades back as the memory dies — no ledger, no price
field, no script. The route to market is Trust, remembered (six fair
trades, 0.63 Trust). The supply chain is a chain of memories.

Built entirely on the public surface: Memory facts, ReportOutcome,
relationships, QueryWhere — the same loop every adapter walks.

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
