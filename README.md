# Living Commonwealth Engine (LCE)

**Building living worlds through simulation.**

The first platform is Fallout 4. The core never knows.

[![License: MIT](https://img.shields.io/badge/License-MIT-emerald.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-emerald.svg)](https://en.cppreference.com/w/cpp/23)
[![Version](https://img.shields.io/badge/Version-0.7.0--alpha-emerald.svg)](#roadmap)
[![Changelog](https://img.shields.io/badge/Changelog-Docs%2FCHANGELOG.md-emerald.svg)](Docs/CHANGELOG.md)

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

## What LCE does today

Entities are **IDs**, not objects, with **components** attached through the
registry. The simulation gives a **mind** to any entity with *Needs*:
memory fades, relationships drift, goals grow urgent, and a stateless
`Decide` turns drives and experience into one **Intent** per tick. The
adapter executes intents in the game — and pushes world facts back in as
memories. No quest script anywhere.

The 0.5.0 boundary contract — the decide → act → observe → remember loop —
is complete and proven (28/28 test suites green):

- **Tuning** — the modder's knob: one text file sets the world's
  personality.
- **Outcome channel** — `ReportOutcome`: the settler *learns* from what
  actually happened (cheated twice → trades with the other merchant).
- **Observation events** — push, not poll: hear creations, intents, and
  outcomes without polling (a co-save load of 637 minds is one event, not
  a flood).
- **Query surface** — `QueryWhere<T>`: filtered reads with a guaranteed
  iteration order — "everyone hungry", "who remembers the raid".
- **Seeded RNG** — splitmix64, one word of state: a single number in the
  save resumes the exact randomness.
- **World calendar** — memories anchored to world days and seasons — the
  substrate 0.7.0 Legacy now stands on (Bequeath, InheritMemory, the
  legacy store).
- **Per-mind decay jitter** — every mind has its own metabolism: no two
  settlers get hungry on the same clock (`sim.jitter` is the knob).

The Fallout 4 adapter — [The Commonwealth Lives](https://github.com/lelsliem/The-Commonwealth-Lives),
a separate project — already translates settlers into entities, walks
them to market, and saves 637 minds through the co-save.

## What LCE will do

**The 0.5.0 SDK side is done:** the Sample Host (`Samples/SampleHost` — a
runnable non-game host: the money test live, fair twice then cheated
twice, save at Day 4 and the lesson survives), Sample Modules (farmer,
village with a grudge, day-stamped market), LCE Doctor (CLI validation:
point it at a project, get a plain pass/fail log), and packaging —
`cmake --install`, then a consumer links `LCE::Core` through
`find_package(LCE)` with nothing else on the path. The official recipe is
[Docs/SDK/Embedding.md](Docs/SDK/Embedding.md). The two GitHub repos are
live and tagged (v0.5.0, v0.6.0, v0.7.0). The 0.6.0 Society
milestone is done — bond thresholds + `RelationshipChangedEvent`
(the adapter's ask, stone 08) and the Society layer (groups, the
settlement rally, inherited attitudes, the traits substrate, stone
09) — and the adapter released 0.6.0-beta (2026-08-11), verified
in-game. The 0.7.0 Legacy milestone is done too — Bequeath,
InheritMemory, the legacy store, and the desperate-hunger gate —
with the feud chain (shut stall → blame → rival → mediated grudge)
verified in-game by the adapter's 0.7.0 release.

**The ladder beyond:**

| Version | Milestone |
|---------|-----------|
| 0.6.0 | Society — groups and traits |
| 0.7.0 | Legacy — birth, death, inheritance |
| 0.8.0 | Scale — a settlement, not a village |
| 0.9.0 | Release Candidate + Public Beta (Nexus + GitHub) |
| 1.0.0 | Release — the promise, made good |

**The point of it all — the mods this makes possible:** living economies
(dynamic pricing, supply chains, trade routes), memory & legacy (entities
remember decades; legends emerge), faction wars (territory, sieges,
diplomacy), living weather (weather that shapes behaviour and travel),
children of the Commonwealth (birth, inheritance, generational memory),
disease & medicine (outbreaks, immunity, healers, cemeteries that grow),
and living roads (caravans prefer maintained routes). Not scripts — the
same simulated loop, turned up.

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

28/28 suites passed.
```

## Repository map

```
Include/LCE/   public headers — the SDK surface: Config, Events, Logging,
               Runtime, Scheduling, Simulation, Tasks, Time, Version
Source/        implementation, one folder per subsystem
Tests/         the LCE test harness (a dev tool; it never ships)
Docs/          philosophy, decisions, design documents, learning path
```

Third-party: spdlog only, fetched at configure time via FetchContent
(never vendored, never part of the public API). Game dependencies live in
each adapter's own project, never in the core.

## Learn

- [Learning Path](Docs/LearningPath.md) — read the code as a course: what
  each subsystem teaches, plus hands-on exercises.
- [Design documents](Docs/Architecture/) — how each stone was designed and
  why.
- [Decision Log](Docs/DecisionLog.md) — the ADRs; the why of everything.
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
| 0.3.1 | Simulation Polish | ✅ |
| 0.4.0 | Platform Integration — Fallout 4 Adapter | ✅ |
| 0.5.0 | SDK & Samples — "The Consumable Engine" | ✅ |
| 0.6.0 | Society — groups & traits | ✅ |
| 0.7.0 | Legacy — birth, death, inheritance | ✅ |
| 0.8.0 | Scale — a settlement, not a village | ⬜ |
| 0.9.0 | Release Candidate + Public Beta (Nexus + GitHub) | ⬜ |
| 1.0.0 | Release | ⬜ |

## License

MIT — see [LICENSE](LICENSE).
