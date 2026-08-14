# Contributing to LCE

The engine is small, opinionated, and frozen. Read these first:

- [Development Charter](Docs/DevelopmentCharter.md) — how LCE is built
- [Project Philosophy](Docs/ProjectPhilosophy.md) — why it's built that way
- [Decision Log](Docs/DecisionLog.md) — the ADRs; the why of everything

## The freeze is the contract

The public surface is frozen ([CompatPolicy](Docs/Design/CompatPolicy.md)).
`SurfaceTest` and `HeaderMapTest` pin it at compile time: a drifted API
cannot build. Changing the surface is a major-version event with a
migration path — never a silent edit. If your change touches
`Include/LCE/`, expect to justify it as a freeze event in the changelog.

## Build and test

```bash
cmake -S . -B Build -DLCE_BUILD_TESTS=ON -DLCE_BUILD_SAMPLES=ON -DLCE_BUILD_TOOLS=ON
cmake --build Build --config Debug
ctest --test-dir Build -C Debug --output-on-failure
```

Every CI leg — MSVC, GCC, Clang, macOS — must stay green. Before push,
reproduce the ubuntu-gcc leg locally (it has failed only on Linux
before): `bash Tools/scripts/linux-gcc-loop.sh` (requires Docker).

## Before you open a PR

- Run the full suite locally (ctest above), not just your own test.
- Keep the surface untouched unless the change *is* a freeze event.
- Update the changelog: what changed, why, dated.
- Keep the docs honest — if behaviour changed, a doc changed with it.

## Filing issues

Bug reports and feature requests have templates — use them. Security
issues go through [private vulnerability reporting](SECURITY.md),
never a public issue.
