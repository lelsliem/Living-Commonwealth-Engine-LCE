═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.8.0-alpha

Current Stage   : Scale — engine side shipped; adapter verification pending

Next Milestone  : 0.9.0 — Release Candidate

═══════════════════════════════════════════════

0.9.0 — Release Candidate · "The Freeze"

═══════════════════════════════════════════════

The API freezes — the moment modders can build without fear. The
real testing need is here, not at 0.5.0: 0.0.0 through 0.8.0 is us.
The endgame plan is locked (Docs/Design/Endgame.md); the adapter
verdicted every cut (AdapterProject.md) — six of the seven patterns
need zero new engine work and disease needs none either.

[~] 0.8.1 — adapter verifies 0.8.0 Scale in-game; the engine lands
    whatever that surfaces (field-fixes only). First field fix landed
    2026-08-12: `Rng::StableDerive` — per-entity noise anchors to the
    seed, not the live state, so the adapter's births between ticks
    can no longer re-roll a settled mind (ADR-0029). Adapter
    verification of 0.8.0 Scale is still the open gate.
[ ] 0.8.2 — the four proven patterns as lean samples: Economy,
    Legacy, Weather, Children
[ ] 0.8.3 — the three harder patterns: Faction Wars, Disease
    (fact-plus-tick, zero new surface), Roads
[ ] 0.8.4 — the API Freeze: the surface-stability test (the harness
    fails if the public API changes), the compat policy doc (what is
    stable, what is append-only, what breaks and when), the
    public-header audit — warts fixed before the freeze is in force
[ ] 0.8.5 — the docs: LearningPath complete, the full audit,
    Embedding.md extended with the 0.8.0 onboarding recipe
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
