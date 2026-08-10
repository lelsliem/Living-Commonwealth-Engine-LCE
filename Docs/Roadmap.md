═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.4.0-alpha

Current Stage   : Platform Integration — complete

Next Milestone  : 0.5.0 — SDK & Samples

═══════════════════════════════════════════════

0.0.0 — Project Definition · "The Blueprint"

═══════════════════════════════════════════════

STATUS: COMPLETE ✅

[✓] Vision
[✓] Philosophy
[✓] Architecture
[✓] Development charter established
[✓] Decision log started
[✓] The cathedral's blueprints are drawn.

═══════════════════════════════════════════════

0.0.1 — Foundation · "The First Heartbeat"

═══════════════════════════════════════════════

Goal: a modern, documented, standalone engine core.

STATUS: COMPLETE ✅

[✓] CMake build system (C++23, MSVC v143, static runtime)
[✓] Zero warnings (/W4 /WX)
[✓] Logger (spdlog hidden behind the LCE API)
[✓] Configuration
[✓] Version (header-only constants)
[✓] Documentation foundation
[✓] First successful build

Completion criteria met: engine initializes, engine shuts down,
documentation complete, zero compiler warnings.

═══════════════════════════════════════════════

0.1.0 — Alpha · Core Runtime (Services)

═══════════════════════════════════════════════

STATUS: COMPLETE ✅

[✓] Event Bus
[✓] Clock
[✓] Scheduler
[✓] Task System
[✓] Configuration
[✓] Logging
[✓] Service Architecture (ServiceRegistry)
[✓] Test harness — six suites, all green

═══════════════════════════════════════════════

0.2.0 — Entity System

═══════════════════════════════════════════════

The first stone of the simulation layer.

STATUS: COMPLETE ✅

[✓] Entity IDs — tagged EntityId (index + generation)
[✓] Components — type-erased per-type stores
[✓] Entity Registry — slots, free list, component access
[✓] Lifetime Management — generational reuse, stale-ID safety

═══════════════════════════════════════════════

0.3.0 — Simulation

═══════════════════════════════════════════════

STATUS: COMPLETE ✅

[✓] Needs — the urgent drives
[✓] Memory — experience, salience, forgetting
[✓] Relationships — feelings toward others
[✓] Goals — minimal: one active goal + urgency
[✓] Behaviour — Decide + Intent
[✓] Registry iteration — ForEachWithComponent
[✓] Harness upgrade — named suites, 13/13 green

Proof: the farmer goes to market. The Behaviour suite shows a hungry
farmer who knows and trusts the merchant deciding to move to them —
no quest script fired.

═══════════════════════════════════════════════

0.3.1 — Simulation Polish

STATUS: COMPLETE ✅

Agreed at the 0.3.0 review — three honest candidates that strengthen
everything 0.4.0 leans on. All shipped and tested.

[✓] World-fact channel — a remembered world fact (invalid Other)
    blocks its kind; the market reopens when the fact is forgotten.
    The adapter's only channel into the mind, now proven by test.
[✓] Safety need — danger awareness (a wronged/combat memory) produces
    a Flee intent, completing all five needs.
[✓] Tuning as input — SimulationTuning replaces hardcoded constants;
    the Configuration wiring pattern is proven by test.

═══════════════════════════════════════════════

0.4.0 — Platform Integration

STATUS: CORE SIDE COMPLETE ✅ (adapter in progress, separate project)

═══════════════════════════════════════════════

The boundary design is the stone: Docs/Architecture/PlatformIntegration.md.
The adapter itself is a separate project (GPL) — this repo carries only
the core side (MIT).

Core side (this repo):

[✓] Boundary design — PlatformIntegration.md
[✓] Registry snapshot/restore + registered serializers
[✓] Snapshot round-trip tests — 14/14 suites green

Adapter project (separate repo, born this milestone):

[ ] F4SE plugin scaffold (F4SEPlugin_Load)
[ ] Entity ↔ form translation
[ ] Intent → game action executor
[ ] Co-save integration
[ ] The real test: settlers with needs in Fallout 4

═══════════════════════════════════════════════

0.5.0 — SDK & Samples · "The Consumable Engine"

═══════════════════════════════════════════════

The engine becomes something a developer can actually pick up.
An internal milestone — the doors open to GitHub, not to testers.
Public beta is 0.9.0. Discord is dropped; feedback runs through
GitHub issues and Nexus comments, async and unsocial by design.

The complete boundary contract — the loop the adapter walks:

[✓] Outcome channel — ReportOutcome: the adapter reports how an
    executed intent went; the sim records memory, scales
    relationships by result (a failed trade loses trust), serves
    the active goal, and consumes the intent.
    Decide → Act → Observe → Remember → Decide (proven by the
    Outcome suite — a cheated settler learns to trade elsewhere).
[ ] Observation events — EntityCreated, EntityDestroyed,
    IntentProduced, FactRemembered, OutcomeRecorded on the EventBus:
    push, not poll.
[ ] Query surface — "everyone hungry", "all settlers who remember
    the raid": filtered queries, documented iteration order.
[✓] Tuning ergonomics — SimulationTuning::FromConfiguration: the
    modder's knob. Known keys override defaults, broken values keep
    the default, unknown keys are ignored — one text file sets the
    world's personality (proven by the Tuning suite).
[ ] Seeded RNG + determinism — randomness with a seed, snapshot-able,
    so a restored save resumes the exact same world.
[ ] World calendar + memory timestamps — memories anchored to world
    time (day counter, seasons, age of a fact). The substrate
    Legacy stands on.

The SDK:

[ ] Sample Host — a runnable non-game host driving the full loop:
    the proof any game can embed LCE, and the sample every modder
    reads first.
[ ] Sample Modules — the teaching patterns: the farmer, a village,
    a market; the seven mod-type patterns as samples.
[ ] LCE Doctor (CLI) — point it at a project; it validates structure
    and pinning against the SDK contract, prints a plain ✓/✗ log,
    and detects the toolchain (CMake / xmake / MSVC), telling the
    developer exactly what's missing.
[ ] Packaging — install targets, find_package, the pinned
    FetchContent recipe made official; a stable public header surface.
[ ] First GitHub releases — engine (MIT) and mod (GPL), two repos,
    tagged with release notes. Releases are artifacts with visible
    history, not a support commitment.

[ ] Documentation Review — the LearningPath and samples teach.

The adapter project (separate repo) is the living demo: translation
and the intent executor are in; settlers walk because they decide to.

═══════════════════════════════════════════════

0.6.0 — Society · "The Bonds Between Minds"

═══════════════════════════════════════════════

Groups — the layer between the individual and the world. The stone
Faction Wars and Children of the Commonwealth both stand on.

[ ] Group component + membership — a settler belongs to a family,
    a settlement, a faction
[ ] Group-level memory and attitude — feelings inherit from the
    group's experiences, then diverge
[ ] Trust shaped by membership — "they wronged my brother"
[ ] Personality / traits — identical needs produce different
    individuals (bold vs cautious); variation within and across
    groups

Proof: a faction rallies — every member of a settlement turns cold
to the merchant who cheated one of them. One event, many minds.

═══════════════════════════════════════════════

0.7.0 — Legacy · "The Debt to the Past"

═══════════════════════════════════════════════

Birth, death, inheritance — what survives the entity. The stone
Memory & Legacy stands on, and the calendar makes it real: entities
remember decades.

[ ] Death lifecycle — what an entity bequeaths as it goes
[ ] Generational handoff — descendants inherit memory, selectively
[ ] Legacy as world fact — the promise that outlives its maker

Proof: a settler's grandson carries the feud to the market and
refuses the merchant — the memory outlived its owner.

═══════════════════════════════════════════════

0.8.0 — Scale · "The Settlement Survives"

═══════════════════════════════════════════════

The engine must hold a living Commonwealth, not a village.

[ ] Tick budgets and profiling — documented cost per entity per
    second
[ ] Iteration efficiency — not every entity, every tick
[ ] Soak tests — simulate years of sim time in minutes; find the
    drift
[ ] Save/load at population scale, round-tripped
[ ] Determinism verified at scale

Proof: a settlement of hundreds ticks inside a frame budget; a year
of sim time passes without drift. The game-thread decision (0.4.0)
is revisited here — F4SE's TaskInterface is already waiting.

═══════════════════════════════════════════════

0.9.0 — Release Candidate · "The Freeze"

═══════════════════════════════════════════════

The API freezes — the moment modders can build without fear. The
real testing need is here, not at 0.5.0: 0.0.0 through 0.8.0 is us.

[ ] API Freeze
[ ] Bug Fixes
[ ] Performance
[ ] Polish
[ ] Full documentation audit — LearningPath complete
[ ] The seven mod-type patterns shipped as sample modules
[ ] Public Beta — the doors open: the mod on Nexus, the engine via
    GitHub; feedback through Nexus comments and GitHub issues.
    Async, unsocial, by design.

═══════════════════════════════════════════════

1.0.0 — Living Commonwealth Engine Release

═══════════════════════════════════════════════

The promise, made good.

[ ] GitHub + Nexus Mods
[ ] SDK, Documentation, Examples — final
[ ] MIT Licensed
[ ] The gate: API frozen and all seven mod types provably possible —
    as patterns in the samples, and as The Living Commonwealth,
    live in Fallout 4.

═══════════════════════════════════════════════

Consciously Deferred — decisions, not oversights

═══════════════════════════════════════════════

[ ] C ABI / Papyrus natives — a real interface when a real consumer
    asks (YAGNI — the 0.4.0 lesson)
[ ] Networking / replication — single-player mods don't need it
[ ] Multi-agent negotiation / planning — deep AI, beyond the
    engine's promise; deferred, not forgotten
[ ] LCE Studio (GUI) — the LCE Doctor wrapped in a window, for
    developers who want app-like onboarding; built when there is an
    audience (post-1.0)
