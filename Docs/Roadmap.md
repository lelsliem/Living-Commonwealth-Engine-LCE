═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.8.6-alpha

Current Stage   : CI live — every push builds and tests on MSVC, GCC, and Clang

Next Milestone  : 0.9.0 — Release Candidate

═══════════════════════════════════════════════

0.9.0 — Release Candidate · "The Freeze"

═══════════════════════════════════════════════

The API freezes — the moment modders can build without fear. The
real testing need is here, not at 0.5.0: 0.0.0 through 0.8.0 is us.
The endgame plan is locked (Docs/Design/Endgame.md); the adapter
verdicted every cut (AdapterProject.md) — six of the seven patterns
need zero new engine work and disease needs none either.

[~] 0.8.1 — Housekeeping & the re-roll fix (engine side done,
    2026-08-12; version bumped to 0.8.1-alpha). Three items landed:
    (1) the field fix for ADR-0029 — `Rng::StableDerive` anchors
    per-entity noise to the seed, never the live state, so the
    adapter's births between ticks can no longer re-roll a settled
    mind; (2) the Simulation folders reorganized into category
    subfolders (Entity/Mind/Society/Decision/Substrate — every
    include re-synced, engine + adapter both green); (3) the public
    surface now guarded — HeaderMapTest (the canonical header map
    frozen in the harness) + an LCE Doctor include-layout check.
    31/31 engine suites. The open gate is still the adapter's
    in-game verification of 0.8.0 Scale (its Illness & Medicine
    plan is next on its side).
[✓] 0.8.2 — the four proven patterns as lean samples, all done:
    Economy (prices are memories — dynamic pricing, supply chains,
    trade routes, market events), Legacy (death, inheritance, the
    name that outlives the voice), Weather (seasons on the calendar,
    weather as day-stamped facts shaping needs), Children (family
    as a group: birth, inherited attitudes, per-child traits) —
    each zero new engine surface. One engine fix fell out: the
    JitteredTraits RNG path was re-deriving the child per trait and
    taking only its first draw, so every trait of an entity was
    identical — now one child stream advances per trait.
[✓] 0.8.3 — the three harder patterns, all done: Faction Wars
    (groups are the map, dispositions the loyalty, InheritGroupAttitudes
    the indoctrination), Disease (a quarantine door-fact + a Fatigue
    toll — the world owns Health, the core owns memory and need),
    Roads (routes as LegacyStore facts whose weight is condition,
    maintained by traffic, forgotten by neglect)
[✓] 0.8.4 — the API Freeze, done. Landed: the personality
    tie-break (near-tied needs resolve by a per-need seeded draw, not
    list order) and per-need metabolism (each need decays at its own
    seeded rate), both keyed on need type — the seam traits multiply
    into; the Fact kind + label string (the world's vocabulary rides
    MemoryEvent::Label; the enum stops growing because labels absorb
    it); the compat policy doc (stable / append-only / adapter-owned /
    breaks-when); the surface-stability test (SurfaceTest — every
    public enum ordinal, struct field type, and function signature
    pinned by static_assert: a drifted surface cannot even compile,
    and the error names the declaration that moved; the suite provably
    caught a simulated ordinal shift); the public-header audit — every
    header read fresh and checked against its implementation (the
    Goals seam now documented honestly: Decide reads needs only, goals
    influence through the world; the Logger.h copy-paste Purpose lie
    fixed; seven malformed banners regenerated to the uniform template;
    /// comment stragglers converted). The freeze is now in force —
    nothing in the surface changes without failing the harness and the
    changelog. 32/32 suites, eleven samples green.
[✓] 0.8.5 — the docs, done: LearningPath extended through the whole
    simulation layer (the tour now runs Version → Scale: 19 stops,
    eleven samples, thirteen exercises); the full audit written up
    (Docs/Design/Audit.md — every header read fresh, what held, what
    was fixed, what was deliberately not changed); Embedding.md gained
    the 0.8.0 runtime recipe — one loop (FixedStep + TickReport +
    sim.memory.cap) so a non-Fallout embedder starts from one doc.
    32/32 suites, eleven samples green
[✓] 0.8.6 — CI, live: a GitHub Actions workflow builds and runs all
    32 suites on three toolchains every push — MSVC (/W4 /WX, warnings
    are errors), GCC, and Clang (portability). The freeze is the point:
    a drifted API must fail on EVERY compiler, and the SurfaceTest size
    guards must hold cross-compiler. ctest wiring, the LCE.Bench smoke
    run, and the installed-package version fixed (it still said 0.5.0 —
    find_package consumers were told a lie; now 0.8.x, matching
    Version.h). Embedding pins moved to the released v0.8.5 tag.
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
