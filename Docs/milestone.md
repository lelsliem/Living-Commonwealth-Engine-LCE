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

Next: Milestone 0.3.0 — Simulation

Memory · Relationships · Behaviour · Goals · Needs
