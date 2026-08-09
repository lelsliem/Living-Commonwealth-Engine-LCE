# Living Commonwealth Engine (LCE)

**Building living worlds through simulation.**

The first platform is Fallout 4. The core never knows.

[![License: MIT](https://img.shields.io/badge/License-MIT-emerald.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-emerald.svg)](https://en.cppreference.com/w/cpp/23)
[![Version](https://img.shields.io/badge/Version-0.3.0--alpha-emerald.svg)](#roadmap)

---

## What is LCE?

LCE is **not a mod**. It is middleware — a simulation engine that brings
persistent life, memory, relationships, and emergent behaviour to Bethesda
Game Studios titles.

Instead of scripting every event, LCE **simulates** systems from which
events naturally emerge:

> A farmer doesn't go to market because a quest script fired — they go
> because they're hungry, they know the merchant, and they understand the
> road.

**Build for Fallout 4. Architect for every Bethesda game.** LCE is a
simulation core with a game adapter. Only the adapter knows the game — the
core never includes game headers, by compile-time guarantee (ADR-0003,
ADR-0023).

## How it works, in one breath

Entities are **IDs**, not objects, with **components** attached through the
registry (0.2.0). The simulation gives a **mind** to any entity with
*Needs*: memory fades, relationships drift, goals grow urgent, and a
stateless `Decide` turns drives and experience into one **Intent** per tick
(0.3.0). The adapter executes intents in the game — and pushes world facts
back in as memories. No quest script anywhere.

## Quick start

Requirements: a C++23 compiler, CMake 3.28+.

```bash
cmake -S . -B Build -G "Visual Studio 17 2022" -A x64 -DLCE_BUILD_TESTS=ON
cmake --build Build --config Debug
./Build/Bin/Debug/LCE.Core.Tests.exe
```

The harness reports every suite by name:

```
[ RUN  ] Behaviour
[  OK  ] Behaviour
[ RUN  ] SimulationTick
[  OK  ] SimulationTick

13/13 suites passed.
```

## Repository map

```
Include/LCE/   public headers — the SDK surface: Config, Events, Logging,
               Runtime, Scheduling, Simulation, Tasks, Time, Version
Source/        implementation, one folder per subsystem
Tests/         the LCE test harness (a dev tool; it never ships)
Docs/          philosophy, decisions, design documents, learning path
Depends/       third-party: spdlog (used); F4SE, CommonLibF4, json (banked)
```

## Learn

- [Learning Path](Docs/LearningPath.md) — read the code as a course: what
  each subsystem teaches, plus hands-on exercises.
- [Design documents](Docs/Architecture/) — how each stone was designed and
  why.
- [Decision Log](Docs/DecisionLog.md) — 34 ADRs; the why of everything.
- [Philosophy](Docs/ProjectPhilosophy.md) and
  [Development Charter](Docs/DevelopmentCharter.md) — how LCE is built.
- [Roadmap](Docs/Roadmap.md) and [Milestone log](Docs/milestone.md) —
  where it's going, what's done.

## Roadmap

| Version | Milestone | Status |
|---------|-----------|--------|
| 0.0.0 | Project Definition | ✅ |
| 0.0.1 | Foundation | ✅ |
| 0.1.0 | Core Runtime (Services) | ✅ |
| 0.2.0 | Entity System | ✅ |
| 0.3.0 | Simulation | ✅ |
| 0.4.0 | Platform Integration — Fallout 4 Adapter | ⬜ next |
| 0.5.0 | Public Beta (GitHub + Discord) | ⬜ |
| 0.9.0 / 1.0.0 | Release Candidate / Living Worlds | ⬜ |

## License

MIT — see [LICENSE](LICENSE).
