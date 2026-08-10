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
    GIT_REPOSITORY https://github.com/lelsliem/Living-Commonwealth-Engine-LCE-.git
    GIT_TAG        v0.5.0
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

find_package(LCE 0.5 REQUIRED)

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
