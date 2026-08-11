═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.6.0-alpha

Current Stage   : Society — bonds, groups & traits complete

Next Milestone  : 0.7.0 — Legacy

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

STATUS: COMPLETE ✅ (core + adapter; the adapter is verified in-game)

═══════════════════════════════════════════════

The boundary design is the stone: Docs/Architecture/PlatformIntegration.md.
The adapter itself is a separate project (GPL) — this repo carries only
the core side (MIT).

Core side (this repo):

[✓] Boundary design — PlatformIntegration.md
[✓] Registry snapshot/restore + registered serializers
[✓] Snapshot round-trip tests — 14/14 suites green

Adapter project (separate repo — The-Commonwealth-Lives — born this
milestone, all verified in-game):

[✓] F4SE plugin scaffold (F4SEPlugin_Load) — the heartbeat logs in-game
[✓] Entity ↔ form translation — settler-faction actors become minds
[✓] Intent → game action executor — settlers walk because they decide to
[✓] Co-save integration — 637 entities saved/restored; record v4
    (entities + Rng stream + stall-keepers + memory world-days)
[✓] The real test: settlers with needs in Fallout 4 — needs decay,
    goals grow urgent, MoveTo intents walk them to their own
    settlement's market; the exchange is physical (cap pouches) and
    the market has hours and weather (verified in-game)

═══════════════════════════════════════════════

0.5.0 — SDK & Samples · "The Consumable Engine"

═══════════════════════════════════════════════

STATUS: COMPLETE ✅ — both repos live, tagged, and released; the
adapter verified the full loop in-game.

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
[✓] Observation events — EntityCreated, IntentProduced,
    OutcomeRecorded on the EventBus: push, not poll. Restore does
    NOT re-announce created entities (a loaded world is no flood).
    (EntityDestroyed/FactRemembered deferred until a consumer asks.)
[✓] Query surface — QueryWhere<T>(predicate): "everyone hungry",
    "all settlers who remember the raid". Filtered reads with
    documented iteration order (ascending EntityId) — the
    determinism hook seeded RNG and save-compat stand on.
[✓] Tuning ergonomics — SimulationTuning::FromConfiguration: the
    modder's knob. Known keys override defaults, broken values keep
    the default, unknown keys are ignored — one text file sets the
    world's personality (proven by the Tuning suite).
[✓] Seeded RNG + determinism — Rng (splitmix64): one word of state,
    so capture/restore is a single number in the co-save. Derive(key)
    gives order-independent per-entity noise — the tick's iteration
    order can never leak into results. Same seed resumes the exact
    same world (proven by the Rng suite).
[✓] World calendar + memory timestamps — WorldTime (day counter) +
    SeasonOf (90-day seasons); MemoryEvent::Day stamped by
    Remember/ReportOutcome (caller-set days win). The age of a
    fact is now.Day - event.Day — the substrate Legacy stands on
    (proven by the WorldCalendar suite: stamp, seasons, snapshot
    round-trip).
[✓] Per-mind decay jitter — the herd, broken at the source: under a
    seeded Rng each mind's needs decay at its own rate, derived from
    its ID (same seed + same entity = same metabolism, every run;
    the parent stream never advances). sim.jitter (default 0.15) is
    the knob; 0 turns the spread off; without an Rng behavior is
    unchanged (proven by the Jitter suite: identical minds diverge,
    identical worlds stay bit-identical).

The SDK:

[✓] Sample Host — Samples/SampleHost: a runnable non-game host driving
    the full loop — the money test, live: one farmer, two merchants;
    fair twice, cheated twice, then Bellamy. A save at Day 4 proves the
    lesson survives the round-trip. Deterministic, host.ini wired.
[✓] Sample Modules — Samples/Sample-Farmer, Sample-Village,
    Sample-Market: the minimal mind; relationships with a grudge that
    steers a villager; a day-stamped market with seasons and weather.
[✓] LCE Doctor (CLI) — Tools/LCEDoctor: validates structure and
    pinning against the SDK contract, prints a plain ✓/✗ log, detects
    the toolchain (CMake / xmake / MSVC) — and reads the core's version
    straight out of Version.h. Proven by the Doctor suite.
[✓] Packaging — install targets + find_package(LCE), verified end to
    end: a consumer project with one CMAKE_PREFIX_PATH configures,
    links LCE::Core, and runs. The pinned FetchContent recipe is now
    official (Docs/SDK/Embedding.md); the package is self-contained
    (spdlog rides in the export set).
[✓] First GitHub releases — engine (MIT) and mod (GPL), two repos,
    both live and public 2026-08-10 (Living-Commonwealth-Engine-LCE-
    and The-Commonwealth-Lives). v0.5.0 tags pushed on both repos;
    release notes live on the adapter (0.5.0-beta and 0.6.0-beta).

[✓] Documentation Review — Docs/SDK/Embedding.md is the embedding
    course (both paths, what not to do, verified); the LearningPath
    gains "The Samples Teach". The README tells the truth end to end.

The adapter project (separate repo) is the living demo — complete
through 0.5.0 and verified in-game: settlers walk to their own
settlement's market because they're hungry, trade with a stall-keeper
for caps, remember the merchant, stop at nightfall, and the whole
world (637 minds, Rng stream, stall-keepers, memory world-days)
survives save/load.

═══════════════════════════════════════════════

0.6.0 — Society · "The Bonds Between Minds"

═══════════════════════════════════════════════

STATUS: COMPLETE ✅ — core and adapter shipped together; the adapter
released 0.6.0-beta 2026-08-11 and verified bonds, households, gossip,
grief, and births in-game. The feud arc waits on 0.7.0's conflicts.

[✓] Bond thresholds + RelationshipChanged (stone 08, SHIPPED
    2026-08-10) — the watch-list the world names
    (sim.bond.threshold.<name>); an experience crossing a line
    publishes RelationshipChangedEvent, edge-triggered — bond
    formation and bond souring are events, not polls. The adapter's
    Request A, built (proven by the BondThreshold suite).

Groups — the layer between the individual and the world. The stone
Faction Wars and Children of the Commonwealth both stand on.

[✓] Group component + membership (stone 09, SHIPPED 2026-08-10) —
    GroupId + the Groups component; a settler belongs to a family,
    a settlement, a faction at once
[✓] Group-level memory and attitude (stone 09) —
    InheritGroupAttitudes: feelings inherit from the group's
    experiences (the mean disposition), then the newcomer's own
    experiences diverge them
[✓] Trust shaped by membership (stone 09) — the echo: a wrong done
    to one settler cools every member of their settlement toward the
    wrongdoer at sim.group.inheritance strength; trust stays
    personal
[✓] Personality / traits (stone 09) — the Traits substrate:
    JitteredTraits turns a base template into per-entity variation,
    deterministic under the seeded RNG; the influence is the world's
    to apply (the same boundary as the species split)

Proof (proven by the Groups suite): a wrong done to one settler —
the settlement turns cold to the merchant who cheated them, and every
member's bond crossing publishes. One outcome, many minds.

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
