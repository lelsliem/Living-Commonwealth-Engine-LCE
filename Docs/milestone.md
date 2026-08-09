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

Next: Milestone 0.3.1 — Simulation Polish

World-fact channel test · Safety/Flee · Configuration wiring

Then: Milestone 0.4.0 — Platform Integration

Fallout 4 Adapter · F4SE · CommonLibF4 · Save/Load · Serialization
