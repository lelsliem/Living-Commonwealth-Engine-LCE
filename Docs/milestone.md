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

Next: Milestone 0.5.0 — SDK & Samples · "The Consumable Engine"

Stone 01 — Tuning ergonomics ✅ (SimulationTuning::FromConfiguration;
known keys override defaults, broken values keep the default, unknown
keys ignored — the adapter's checklist item "tuning from the
Configuration service" is now a one-liner. Proven by the Tuning suite.)

Stone 02 — Outcome channel ✅ (ReportOutcome: the observe leg of the
living loop. Memory recorded; relationship effects scaled by result —
Success builds trust, Failure loses it, a wrong is a wrong; the active
goal is served by Success, halved by Partial; the intent is consumed so
the next tick decides fresh. Proven by the Outcome suite —the money
   test: a settler who is cheated twice learns to trade with the other
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
nullptr keeps the deterministic id-hash fallback.Proven by the Rng
   suite — including two identical worlds under one seed producing
   bit-identical intents.)

Stone 06 — World calendar + memory timestamps ✅ (WorldTime: a day
counter the adapter drives from the game clock, and SeasonOf — four
90-day seasons. MemoryEvent::Day anchors a memory to the world day it
happened; Remember/ReportOutcome stamp it from the passed WorldTime
and a caller-set day wins. The age of a fact is now.Day - event.Day —
the substrate 0.7.0 Legacy stands on. Proven by the WorldCalendar
suite: stamping, caller-priority, season boundaries, and the timestamp
surviving a snapshot round trip. 20/20 green — the boundary contract
is complete.)

The complete boundary contract — the decide → act → observe → remember
loop: outcome channel, observation events, query surface, seeded RNG +
determinism, world calendar + memory timestamps.

The SDK: Sample Host (the non-game proof), Sample Modules (the seven
mod-type teaching patterns), LCE Doctor (CLI validation), packaging
(install targets, find_package, pinned recipe), first GitHub releases
for engine and mod — two repos, tagged, source public.

Then: 0.6.0 Society (groups + traits) · 0.7.0 Legacy (birth, death,
inheritance) · 0.8.0 Scale (a settlement, not a village) · 0.9.0
Release Candidate + Public Beta (the freeze; Nexus + GitHub, no
Discord) · 1.0.0 Release (the promise, made good).
