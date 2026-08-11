═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Milestone Log

One stone at a time. Each milestone is complete, tested,
and documented before the next begins.

═══════════════════════════════════════════════

Milestone 0.0.0 — Project Definition

STATUS: COMPLETE ✅

• Vision
• Philosophy
• Architecture
• Brand identity
• Coding standards
• Development charter
• Roadmap
• Decision log

═══════════════════════════════════════════════

Milestone 0.0.1 — Foundation

STATUS: COMPLETE ✅

• CMake build system — C++23, MSVC v143, static runtime
• Version system (header-only constants)
• Logging API and implementation (spdlog hidden)
• Configuration
• Documentation foundation
• LCE.Core.lib produced
• Zero compiler warnings (/W4 /WX)

First heartbeat:

[Info] Living Commonwealth Engine initialized.

═══════════════════════════════════════════════

Milestone 0.1.0 — Core Runtime (Services)

STATUS: COMPLETE ✅

Subsystem 01 — Logging ✅
Subsystem 02 — Event Bus ✅
Subsystem 03 — Clock ✅
Subsystem 04 — Scheduler ✅
Subsystem 05 — Task System ✅
Subsystem 06 — Configuration ✅
Subsystem 07 — Service Architecture (ServiceRegistry) ✅

Version: 0.1.0-alpha.

Test harness: six suites, all green. Verified from a clean
rebuild (Build/ deleted, reconfigured, rebuilt, ran).

[info] Living Commonwealth Engine initialized.
[trace] Logging test: Trace
[debug] Logging test: Debug
[info] Logging test: Info
[warning] Logging test: Warning
[error] Logging test: Error
[critical] Logging test: Critical

All LCE Core tests passed.

Design documents: Docs/Architecture/ServiceRegistry.md
Decisions: ADR-0034 (Service Registry shape and placement)

═══════════════════════════════════════════════

Milestone 0.2.0 — Entity System

STATUS: COMPLETE ✅

Stone 01 — EntityId ✅ (tagged type: index + generation)
Stone 02 — Component stores ✅ (type-erased, per-type)
Stone 03 — EntityRegistry ✅ (slots, free list, stores)
Stone 04 — Lifetime management ✅ (generational reuse, stale-ID safety)

Test harness: seven suites, all green.

Design document: Docs/Architecture/EntitySystem.md

═══════════════════════════════════════════════

Milestone 0.3.0 — Simulation

STATUS: COMPLETE ✅

Stone 01 — Needs ✅ (the urgent drives)
Stone 02 — Memory ✅ (experience, salience, forgetting)
Stone 03 — Relationships ✅ (feelings toward others)
Stone 04 — Goals ✅ (minimal: one active goal + urgency)
Stone 05 — Behaviour ✅ (Decide + Intent)
Stone 06 — Registry iteration ✅ (ForEachWithComponent)
Stone 07 — Harness upgrade ✅ (named suites)

Proof: the farmer goes to market — hungry, knowing the merchant,
trusting them, and understanding the road. The Behaviour suite is
that proof, and the harness reports 13/13 suites green.

Design document: Docs/Architecture/Simulation.md

═══════════════════════════════════════════════

Milestone 0.3.1 — Simulation Polish

STATUS: COMPLETE ✅

Stone 01 — World-fact channel ✅ (deficits: a remembered fact blocks
its kind; the market reopens when it is forgotten)
Stone 02 — Safety need ✅ (danger awareness → Flee intent)
Stone 03 — Tuning as input ✅ (SimulationTuning; Configuration
wiring pattern proven by test)

Test harness: 13/13 suites green — the farmer's story now includes
a closed market, a fleeing villager, and a world that forgets.

═══════════════════════════════════════════════

Milestone 0.4.0 — Platform Integration (core side)

STATUS: COMPLETE ✅

Stone 01 — Boundary design ✅ (PlatformIntegration.md; the adapter is a
client of the public API; Interfaces stubs deleted per Decision #1)
Stone 02 — Registry snapshot ✅ (RegistrySnapshot, RegisterSerializer,
Capture / Restore / Clear)
Stone 03 — Round-trip tests ✅ (Snapshot suite; the farmer still goes to
market after a save and a load)

Test harness: 14/14 suites green.

The Fallout 4 adapter is a separate project (GPL), born with this
milestone: F4SE plugin · form translation · intent executor · co-save.

═══════════════════════════════════════════════

Milestone 0.5.0 — SDK & Samples · "The Consumable Engine"

STATUS: COMPLETE ✅ — engine 0.5.0-alpha; tags and release notes live on both repos

Stone 01 — Tuning ergonomics ✅ (SimulationTuning::FromConfiguration;
known keys override defaults, broken values keep the default, unknown
keys ignored — the adapter's checklist item "tuning from the
Configuration service" is now a one-liner. Proven by the Tuning suite.)

Stone 02 — Outcome channel ✅ (ReportOutcome: the observe leg of the
living loop. Memory recorded; relationship effects scaled by result —
Success builds trust, Failure loses it, a wrong is a wrong; the active
goal is served by Success, halved by Partial; the intent is consumed so
the next tick decides fresh. Proven by the Outcome suite — the money test: a settler who is cheated twice learns to trade with the other
   merchant, no script.)

Stone 03 — Observation events ✅ (push, not poll: EntityCreated on
CreateEntity — and deliberately NOT on snapshot restore, so a co-save
load of 637 minds is no creation flood; IntentProduced on the tick's
every fresh decision; OutcomeRecorded on ReportOutcome. The bus is an
input, never global state (ADR-0014). Proven by the Observation suite.)

Stone 04 — Query surface ✅ (QueryWhere<T>(predicate): filtered reads
with deterministic iteration order — ascending EntityId::Value(), so
the same query returns the same result every run; cross-component
filters by capturing the registry; const — a query reads, never
mutates. "Everyone hungry" and "who remembers the raid" proven by
test.)

Stone 05 — Seeded RNG + determinism ✅ (Rng: splitmix64, one word of
state — State/SetState makes save/load a single number. Derive(key)
creates order-independent per-entity child streams without advancing
the parent, so the tick's unordered iteration can never leak into
results: same seed + same entity = same jitter, every run. Decide and
Update take an optional Rng (defaulted — existing callers untouched);
nullptr keeps the deterministic id-hash fallback. Proven by the Rng
   suite — including two identical worlds under one seed producing
   bit-identical intents.)

Stone 06 — World calendar + memory timestamps ✅ (WorldTime: a day
counter the adapter drives from the game clock, and SeasonOf — four
90-day seasons. MemoryEvent::Day anchors a memory to the world day it
happened; Remember/ReportOutcome stamp it from the passed WorldTime
and a caller-set day wins. The age of a fact is now.Day - event.Day —
the substrate 0.7.0 Legacy stands on. Proven by the WorldCalendar suite: stamping, caller-priority, season boundaries, and the timestamp
    surviving a snapshot round trip. 20/20 green.)

Stone 07 — Per-mind decay jitter ✅ (the herd, broken at the source:
under a seeded Rng each entity's needs decay at its own rate, derived
from its ID — same seed + same entity = same metabolism, every run,
and the parent stream never advances. sim.jitter (default 0.15) is the
modder's knob; 0 turns the spread off; without an Rng the rate is
exactly 1.0, so no existing caller is affected. Proven by the Jitter
suite: identical minds diverge under one seed, identical worlds stay
bit-identical, and the knob demonstrably widens the gap. 21/21 green.)

The core-side boundary contract is COMPLETE — the decide → act →
observe → remember loop is closed and proven (tuning, outcome channel,
observation events, query surface, seeded RNG + determinism, world
calendar + memory timestamps, per-mind decay jitter; 25/25 suites
green, counting stone 08's BondThreshold and stone 09's Groups and
Traits suites).

The SDK side of 0.5.0 is DONE too: the Sample Host (Samples/SampleHost
— the non-game proof; the money test runs live: fair twice, cheated
twice, then Bellamy, and the save round-trip keeps the lesson), Sample
Modules (farmer, village, market — Samples/), LCE Doctor
(Tools/LCEDoctor — CLI validation of the SDK contract, 5/5 checks on a
real target), and packaging (install targets + find_package(LCE),
verified end to end by a consumer build; Docs/SDK/Embedding.md is the
official recipe). Both GitHub repos are live and public.

0.5.0 is CLOSED: the v0.5.0 tags are pushed on both repos and the
release notes are live — the adapter ships 0.5.0-beta and 0.6.0-beta
on GitHub.

Milestone 0.6.0 — Society · "The Bonds Between Minds"

STATUS: COMPLETE ✅ — engine 0.6.0-alpha; shipped with
The-Commonwealth-Lives 0.6.0-beta and verified in-game 2026-08-11.

Stone 08 — Bond thresholds + RelationshipChangedEvent ✅ (the
adapter's Request A): the world names its own bond lines
(sim.bond.threshold.*); an experience crossing one publishes the
moment, edge-triggered and drift-quiet. Proven by the BondThreshold
suite.)

Stone 09 — Society: Groups & Traits ✅ (GroupId + the Groups membership
component; the echo — trust is earned personally, disposition travels
to group-mates at sim.group.inheritance — one wrong, a settlement
turns cold; InheritGroupAttitudes — feelings inherit from the group's
experiences, then diverge; and the Traits substrate — JitteredTraits,
deterministic under the seeded RNG, influence adapter-side. Proven by
the Groups + Traits suites.)

The death fact ✅ — InteractionKind::Death lets adapters record deaths
in memory; a fact, never a door (Decide gates only Trade and Social).

The adapter's side, verified in-game: bonds form, couples marry into
households, deaths spread as gossip, sim-only children are born and
grow. The feud arc waits on 0.7.0's conflicts. 25/25 suites green.

0.7.0 Legacy (birth, death, inheritance) · 0.8.0 Scale (a settlement,
not a village) · 0.9.0 Release Candidate + Public Beta (the freeze;
Nexus + GitHub, no Discord) · 1.0.0 Release (the promise, made good).
