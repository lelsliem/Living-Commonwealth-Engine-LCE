═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.3.1-alpha

Current Stage   : Simulation — complete

Next Milestone  : 0.4.0 — Platform Integration

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

═══════════════════════════════════════════════

[ ] Fallout 4 Platform Adapter
[ ] CommonLibF4
[ ] F4SE
[ ] Serialization
[ ] Save / Load

═══════════════════════════════════════════════

0.5.0 — Public Beta

═══════════════════════════════════════════════

> **Reminder:** before 0.5.0 ships, discuss 0.6.0, 0.7.0, 0.8.0, 0.9.0,
> and the 1.0.0 public release in detail.

[ ] SDK
[ ] Sample Modules
[ ] Module Loader
[ ] Engine Coordinator
[ ] GitHub Beta Release — public builds + tracked issue feedback
[ ] Discord Community — announcements, support, live feedback
[ ] Performance Profiling
[ ] Documentation Review

═══════════════════════════════════════════════

0.9.0 — Release Candidate

═══════════════════════════════════════════════

[ ] Bug Fixes
[ ] Performance
[ ] Polish
[ ] API Freeze

═══════════════════════════════════════════════

1.0.0 — Living Commonwealth Engine Release

═══════════════════════════════════════════════

[ ] GitHub
[ ] Nexus Mods
[ ] SDK
[ ] Documentation
[ ] Examples
[ ] MIT Licensed
