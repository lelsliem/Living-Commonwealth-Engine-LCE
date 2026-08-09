//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐ //
// │                                                                         │ //
// │                       ██╗      ██████╗███████╗                          │ //
// │                       ██║     ██╔════╝██╔════╝                          │ //
// │                       ██║     ██║     █████╗                            │ //
// │                       ██║     ██║     ██╔══╝                            │ //
// │                       ███████╗╚██████╗███████╗                          │ //
// │                       ╚══════╝ ╚═════╝╚══════╝                          │ //
// │                                                                         │ //
// │            Building living worlds through simulation.                   │ //
// │                                                                         │ //
// │                 001100010010011110100001101101110011                    │ //
// │                                                                         │ //
// └─────────────────────────────────────────────────────────────────────────┘ //
//								       //
// Living Commonwealth Engine (LCE)					       //
//-----------------------------------------------------------------------------//
// File:								       //
//      README.md							       //
//								       //
// Purpose:								       //
//      Owns the lifetime of the Living Commonwealth Engine.		       //
//								       //
// Project:								       //
//      Living Commonwealth Engine (LCE)				       //
//								       //
// License:								       //
//      MIT License							       //
//								       //
// SPDX-License-Identifier: MIT		            	       	               //
//								       //
// Copyright 								       //
//      (c) 2026-present LCE Contributors				       //
//=============================================================================//
# Living Commonwealth Engine (LCE)
**Building living worlds through simulation.**

The first platform is Fallout 4. The core never knows.

[![License: MIT](https://img.shields.io/badge/License-MIT-emerald.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-emerald.svg)](https://en.cppreference.com/w/cpp/23)
[![Version](https://img.shields.io/badge/Version-0.3.0--alpha-emerald.svg)](#roadmap)
[![Status](https://img.shields.io/badge/Status-In%20Development-emerald.svg)](#roadmap)
[![Platform](https://img.shields.io/badge/Platform-Fallout%204-emerald.svg)](#architecture)

---

</div>

## What Is LCE?

LCE is **not a mod**. It is middleware — a simulation engine that brings persistent life, memory, relationships, and emergent behaviour to Bethesda Game Studios titles.

Instead of scripting every event, LCE **simulates** systems from which events naturally emerge. A farmer doesn't go to market because a quest script fired — they go because they're hungry, they know the merchant, and they understand the road.

> **Build for Fallout 4. Architect for every Bethesda game.**
>
> We don't build a Fallout 4 engine. We build a simulation engine with a Fallout 4 adapter. If Bethesda ships Fallout 5 tomorrow, only the adapter changes — the simulation stays the same.

---

## Philosophy

### The Cathedral Principle

LCE is built like a cathedral — stone by stone, with intention. Foundations are strengthened before new layers are added. We do not build the second floor until the first is load-bearing. Speed comes from never having to rebuild.

### Simulation Over Scripting

We don't create stories — we create a living world from which stories naturally emerge. The world is alive because its inhabitants are: they have hunger, homes, relationships, memory, and understanding.

### Six Design Laws

| Law | Rule |
|-----|------|
| **001** | Simple things should be simple. Complex things should be composed from simple things. |
| **002** | Architecture before implementation. |
| **003** | Teach through code. |
| **004** | Simulation over scripting. |
| **005** | Headers describe what. Source files describe how. Documentation describes why. |
| **006** | Own the interface, not the implementation. |

### The Four Questions

Before introducing any new class, subsystem, feature, or dependency, the project leads review:

1. **Can it be simpler?**
2. **Does it belong?**
3. **Do we need this at all?**
4. **Will this help build living worlds through simulation?**

---

## Architecture

LCE is a strict layered architecture. **Dependencies point downward only.** Nothing above knows what's below it, and nothing below reaches up.

```
┌─────────────────────────────────────────────┐
│  Applications                                │
│  Game · Test Harness · Standalone Tools      │
├─────────────────────────────────────────────┤
│  Platform Adapters                           │
│  Fallout 4 · Skyrim · Starfield              │
├─────────────────────────────────────────────┤
│  Engine                                      │
│  Coordinator — orchestrates, never owns      │
├─────────────────────────────────────────────┤
│  Runtime                                     │
│  ModuleLoader · Scheduler · EventBus          │
├─────────────────────────────────────────────┤
│  Services                                    │
│  Logger · Config · Clock · Registry           │
├─────────────────────────────────────────────┤
│  Foundation                                   │
│  Version · Utility · Detail                   │
└─────────────────────────────────────────────┘
```

> **Target architecture.** Built and tested so far: Foundation (Version), Services (Logging · Config · Clock), and Runtime (EventBus · Scheduler · Tasks · ServiceRegistry). The Engine Coordinator and platform adapters arrive with later milestones (0.4.0+); nothing above the services layer exists yet.

### What Core Knows

- ✅ Entities, events, time, simulation
- ✅ Logging, configuration, scheduling
- ✅ Standard C++ and LCE types only
- ✅ Its own interfaces and contracts

### What Core Never Knows

- ❌ Fallout 4, Skyrim, or any game
- ❌ F4SE, CommonLibF4, or game SDKs
- ❌ Rendering, audio, or asset formats
- ❌ Third-party library types

---

## Repository Structure

```
LCE/
├── Include/           → public headers
│   ├── LCE/              → core subsystems: Config · Events · Logging · Runtime · Scheduling · Tasks · Time · Version
│   └── Platforms/        → game adapters (Fallout 4 · Skyrim · Starfield) — placeholder, built in 0.4.0
├── Source/            → implementation, one folder per subsystem
├── Depends/           → third-party: spdlog (used). json, CommonLibF4, F4SE reserved for later milestones
├── Docs/              → architecture · decisions · philosophy · milestones
├── Tests/             → LCE-owned test harness (LCE.Core.Tests)
├── Samples/           → sample applications (Milestone 0.5.0)
└── Build/             → generated output, not part of the repository
```

---

## Core Runtime

One subsystem at a time. Each one designed, documented, implemented, built, and tested before the next begins.

### Built & Tested

| Subsystem | Namespace | Files | Description |
|----------|-----------|-------|-------------|
| **Version** | `LCE::Version` | `Version.h` | Header-only compile-time constants. No class, no functions, no macros — just the engine's identity. |
| **Logging** | `LCE::Logging` | `LogLevel.h` · `Logger.h` · `Logger.cpp` | Six free functions behind a clean API. spdlog hidden privately behind the implementation boundary. |
| **Events** | `LCE::Events` | `Event.h` · `EventBus.h` · `EventBus.cpp` | A polymorphic base contract and a bus that owns its handlers. No global state. |
| **Time** | `LCE::Time` | `Clock.h` · `Clock.cpp` | One method: `Elapsed()`. Each Clock owns its starting point. Reset removed — if you need a new starting point, create a new Clock. |
| **Scheduler** | `LCE::Scheduling` | `Scheduler.h` · `Scheduler.cpp` | Receives delta time, fires delayed callbacks. The simulation's heartbeat controller. |
| **Tasks** | `LCE::Tasks` | `Task.h` · `Task.cpp` | A unit of deferred work. Owns a callback and executes it on demand — safely, even when the callback is empty. |
| **Configuration** | `LCE::Config` | `Configuration.h` · `Configuration.cpp` | A simple runtime key/value store. Services read their settings from it. |
| **Service Registry** | `LCE::Runtime` | `ServiceRegistry.h` | Type-safe container for core services. Subsystems are given what they need — they never reach out to find it. Header-only, replaceable by design. |
| **Entity System** | `LCE::Simulation` | `EntityId.h` · `EntityRegistry.h` · `EntityRegistry.cpp` | An entity is an ID, not an object. Tagged `EntityId` with generational slots, a registry that owns entities and their components, and type-erased component stores. |
| **Simulation** | `LCE::Simulation` | `Needs.h` · `Memory.h` · `Relationships.h` · `Goals.h` · `Behaviour.h` · `Simulation.h` | The Mind: needs decay, memory fades, relationships drift, and behaviour decides one intent per mind. The farmer goes to market because he's hungry, he knows the merchant, and he understands the road. |

### Next

| Subsystem | Namespace | Description |
|----------|-----------|-------------|
| **Platform Integration** | `Include/Platforms/` | Milestone 0.4.0 — the Fallout 4 adapter: game lifecycle, co-save, thread marshalling, entity translation. The game becomes the real test. |

### Test Output

```
[info] Living Commonwealth Engine initialized.
[trace] Logging test: Trace
[debug] Logging test: Debug
[info] Logging test: Info
[warning] Logging test: Warning
[error] Logging test: Error
[critical] Logging test: Critical
[ RUN  ] Logging
[  OK  ] Logging
[ RUN  ] EventBus
[  OK  ] EventBus
[ RUN  ] Clock
[  OK  ] Clock
[ RUN  ] Scheduler
[  OK  ] Scheduler
[ RUN  ] Task
[  OK  ] Task
[ RUN  ] ServiceRegistry
[  OK  ] ServiceRegistry
[ RUN  ] EntityRegistry
[  OK  ] EntityRegistry
[ RUN  ] Needs
[  OK  ] Needs
[ RUN  ] Memory
[  OK  ] Memory
[ RUN  ] Relationships
[  OK  ] Relationships
[ RUN  ] Goals
[  OK  ] Goals
[ RUN  ] Behaviour
[  OK  ] Behaviour
[ RUN  ] SimulationTick
[  OK  ] SimulationTick

13/13 suites passed.
```

Every suite is a named, bool-returning function; the runner reports PASS/FAIL for each and a final count. The harness is a dev-time tool — from 0.4.0 on, the real test is Fallout 4 itself.

---

## Roadmap

The cathedral is built stone by stone. Each milestone is a complete, tested, documented layer before the next begins.

| Version | Milestone | Codename | Status |
|---------|-----------|----------|--------|
| `0.0.0` | Project Definition | The Blueprint | ✅ Complete |
| `0.0.1` | Foundation | The First Heartbeat | ✅ Complete |
| `0.1.0` | Alpha — Core Runtime | Services | ✅ Complete |
| `0.2.0` | Entity System | Entities | ✅ Complete |
| `0.3.0` | Simulation | Memory · Relationships | ✅ Complete |
| `0.4.0` | Platform Integration | Fallout 4 Adapter | ⬜ Next |
| `0.5.0` | Public Beta | SDK · Sample Modules | ⬜ Pending |
| `0.9.0` | Release Candidate | Polish · API Freeze | ⬜ Pending |
| `1.0.0` | Living Worlds | Release | ⬜ Pending |

---

## Development Charter

### The Build Rhythm

1. **Design** — Write the design document. Interface, responsibilities, dependencies, test plan.
2. **Review** — The four questions. Does it belong? Can it be simpler?
3. **Implement** — Headers first, then source. The document is the spec.
4. **Test** — Every public method tested. The harness runs on every build.
5. **Document** — Update the docs. The why must match the what.
6. **Merge** — Tests pass, docs written, design reviewed. The stone is set.

### Decision Discipline

Every architectural choice is an **ADR** (Architectural Decision Record). Once accepted, a decision only changes if a better architectural solution exists. The log is the institutional memory. See the full [Decision Log](Docs/DecisionLog.md) for all 34 recorded decisions.

---

## Getting Started

### Prerequisites

- **C++23** compatible compiler (MSVC 19.40+, Clang 18+, GCC 14+)
- **CMake** 3.28+
- **Fallout 4** (for platform integration testing)
- **F4SE** (Fallout 4 Script Extender)
- **CommonLibF4** (included in Depends/)

### Build

```bash
# Clone the repository
git clone https://github.com/LCE-Project/Living-Commonwealth-Engine.git
cd Living-Commonwealth-Engine

# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLCE_BUILD_TESTS=ON

# Build the engine
cmake --build build --config Release

# Run the test harness
./build/Bin/Release/LCE.Core.Tests.exe
```

### Project Layout for Module Authors

> This section describes the SDK, which ships with Milestone 0.5.0. Until then, the only consumer of LCE is the test harness.

If you're writing a module that consumes LCE:

1. Link against the LCE SDK library
2. Include only `LCE/` headers — never `Core/`, `Platform/`, or `Depends/`
3. Implement the module entry point
4. Register your systems with the Engine Coordinator
5. Simulate, don't script

---

## Key Architectural Decisions

| # | Decision | Summary |
|---|----------|---------|
| 0001 | Open Source | Fully open source under the MIT License. |
| 0003 | Platform Independence | Core never includes game headers. |
| 0004 | Service Registry | Core services provided through a registry — loose coupling, replaceable. |
| 0019 | Layered Dependency Rule | Dependencies point downward only. |
| 0021 | The Cathedral Principle | Foundations are strengthened before new layers are added. |
| 0025 | Header-Only Foundation Constants | Compile-time immutable data stays header-only. |
| 0026 | Free Functions Over Static Classes | When a class has no state, use free functions in a namespace. |
| 0027 | Hide Third-Party Libraries | Public headers expose only LCE and standard C++ types. |
| 0029 | Minimal Dependencies | Every external library must justify its maintenance cost. |
| 0030 | Own the Interface, Not the Implementation | If spdlog disappears, only Logger.cpp changes. |

See the full [Decision Log](Docs/DecisionLog.md) for all 34 decisions.

---

## Contributing

LCE is built together. We challenge ideas, not people.

### Before You Code

0. **Start here** — read the [Learning Path](Docs/LearningPath.md). It is the guided tour: what to read, what each subsystem teaches, and exercises to make it stick.
1. Read the [Philosophy](Docs/ProjectPhilosophy.md) and the [Development Charter](Docs/DevelopmentCharter.md)
2. Check the [Decision Log](Docs/DecisionLog.md) — your question may already be answered
3. Ask the four questions before proposing any new class, subsystem, or dependency
4. Follow the build rhythm: design → review → implement → test → document → merge

### Code Standards

- **C++23** — modern features are expected, not just allowed
- **Const correctness** — every non-mutating method is `const`
- **RAII** — resources are acquired in constructors, released in destructors
- **No global state** — no global variables, no static singletons
- **Forward declarations first** — prefer forward declarations over includes in headers
- **Namespaces mirror folders** — `LCE::Logging` lives in `Logging`, `LCE::Runtime` lives in `Runtime`

---

## License

Living Commonwealth Engine is licensed under the **MIT License**.

```
MIT License

Copyright (c) 2026-present LCE Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

<div align="center">

```
001100010010011110100001101101110011
```

*there is no magic. only systems.*

**Built to be understood. Built to be extended. Built to outlive its creators.**

Living Commonwealth Engine (LCE) · C++23 · MIT Licensed · (c) 2026-present LCE Contributors

</div>
