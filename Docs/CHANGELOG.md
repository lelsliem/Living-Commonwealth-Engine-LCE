# Changelog

All notable changes to the Living Commonwealth Engine (LCE).
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
