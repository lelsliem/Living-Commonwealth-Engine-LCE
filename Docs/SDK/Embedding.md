# Embedding LCE in Your Project

Two paths, one exported target: **LCE::Core**. Everything below was
verified end to end on 2026-08-10 — an installed package, a consumer
with nothing on its path but one `CMAKE_PREFIX_PATH`, a build, and a
running binary that printed its own version.

---

## Path 1 — FetchContent (the primary path)

The engine repo is your dependency. This is the path the Fallout 4
adapter uses, and the one to prefer while you are developing against
the SDK: you get the sources, the tests, and the samples in one
configure.

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    lce
    GIT_REPOSITORY https://github.com/lelsliem/Living-Commonwealth-Engine-LCE.git
    GIT_TAG        v0.9.1
)
FetchContent_MakeAvailable(lce)

add_executable(myworld main.cpp)
target_link_libraries(myworld PRIVATE LCE::Core)
```

LCE fetches its own spdlog (pinned) — you never see it. Pin the `GIT_TAG`
to the released version you build against.

## Path 2 — find_package (the installed path)

For prebuilt consumption — a binary you ship or an install you share —
`cmake --install` the engine, then find it:

```bash
cmake --install Build --config Release --prefix <install-prefix>
```

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LCE 0.8 REQUIRED)

add_executable(myworld main.cpp)
target_link_libraries(myworld PRIVATE LCE::Core)
```

Configure with the install prefix on the search path:

```bash
cmake -S . -B Build -DCMAKE_PREFIX_PATH=<install-prefix>
```

That is the whole path: one variable, one `find_package`, one target.

## What the package brings

The installed package is **self-contained**. LCE.Core is a static
library, so its private link to spdlog is part of your link line —
spdlog therefore rides in the same export set and installs beside the
core. `find_package(LCE)` also resolves the `Threads` dependency spdlog
carries. Nothing else needs to be on your path, and you must *not* fetch
your own spdlog when you use this path — the package already provides
the exact one LCE.Core was built against.

## MSVC note

LCE is built with the static CRT. The package config sets
`CMAKE_MSVC_RUNTIME_LIBRARY` to the matching static runtime for you, so
`find_package(LCE)` links clean on MSVC without extra flags.

## The stable surface

Install ships `Include/LCE` and nothing else: `Config`, `Events`,
`Logging`, `Scheduling`, `Simulation`, `Tasks`, `Time`, `Version`.
What is in there is public; what is not in there is not public.

## What NOT to do

- **Do not** mix the paths: FetchContent *and* `find_package(LCE)` in
  one project defines the `spdlog` target twice and breaks the export.
  Choose the path that matches how you consume LCE.
- **Do not** vendor LCE's headers into your tree. Embed the repo
  (Path 1) or install it (Path 2); drift starts the day you copy.
- **Do not** call into internals. If a header is not under
  `Include/LCE`, it is not yours to call.

## The runtime recipe (0.8.0) — a complete minimal host

Packaging gets you the library; this gets you a *living world*. The
whole runtime contract, in one place — the loop an embedder actually
runs:

```cpp
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"
#include "LCE/Events/EventBus.h"

using namespace LCE::Simulation;

// The inputs — nothing is global (ADR-0014):
EntityRegistry registry;          // entities + components
Rng rng{ 0xC0FFEE };              // the seeded stream; save State() in the co-save
EventBus events;                  // push channel for observations

// Tuning from the modder's text file — known keys override defaults,
// broken values keep the default, unknown keys are ignored:
//   sim.memory.cap 64        -> each mind holds at most 64 events
//   sim.jitter 0.15          -> per-mind metabolism spread
//   sim.memory.fade 0.2      -> salience lost per second
SimulationTuning tuning = SimulationTuning::FromConfiguration(config);

tick:
// FixedStep: real frame deltas in, whole fixed steps out. Same seed +
// same steps = same world at any frame rate.
FixedStep step;   // Step = 0.1s sim cadence
TickReport report;

// Each frame:
const std::size_t steps = step.Advance(
    frameDelta, registry, tuning, &events, &rng, &report);

// steps = how many intents were produced this frame. React to them via
// the bus (push, not poll) and feed the results back:
//   IntentProducedEvent  -> execute in the game world
//   OutcomeRecordedEvent -> ReportOutcome already recorded it
// RelationshipChangedEvent -> the world's bond lines crossed

// The cost of a settlement is knowable, not guessed:
//   report.TotalMs, report.Entities, report.MemoryEvents, ...
// Print it once a minute — that is the 0.9.0 scale gate, earned here.

// Save/load (co-save): register serializers ONCE at init for every
// component you want persisted, then:
//   RegistrySnapshot snap = registry.Capture();   // pure data
//   registry.Restore(snap);                        // ids preserved exactly
//   registry.Clear();                              // blank, serializers kept
// Persist rng.State() beside it — one number resumes the stream.
```

**Three keys to know first.** `sim.memory.cap` bounds each mind's
history (the hot path stays bounded); `sim.jitter` breaks the herd
(each mind metabolizes at its own rate); `FixedStep` is the cadence
that makes a year of sim time pass without drift whatever your frame
rate. That is the whole 0.8.0 onboarding: one loop, three knobs, and
the bus.

The Fallout 4 adapter (The-Commonwealth-Lives) is the living example
of this recipe — the same loop, the game as the executor.

## The proof

The 0.5.0 packaging verification: `cmake --install` into a scratch
prefix; a consumer project with only the Path 2 recipe above; configure,
build, run. Output:

```
Consumer linked against 0.5.0-alpha
```

Both paths now have homes in this repository: the Fallout 4 adapter is
the living FetchContent example, and the find_package path above is the
installed-package example. Pick the one that matches your project.
