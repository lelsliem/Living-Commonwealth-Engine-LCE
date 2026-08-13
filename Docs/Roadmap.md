═══════════════════════════════════════════════

Living Commonwealth Engine (LCE)

Building living worlds through simulation.

═══════════════════════════════════════════════

Project Roadmap

═══════════════════════════════════════════════

Status

Current Version : 0.8.9-alpha

Current Stage   : 0.8.9 done — the trust story written, the Studio watching; the gate is next

Next Milestone  : 0.9.0 — the release gate (the doors open)

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
[✓] 0.8.7 — LCE Bench, done: Tools/LCEBench reproduces the Scale
    numbers on any machine — ms/tick at 1k/5k/20k minds with the
    per-pass breakdown, co-save bytes per mind (the documented 207,
    exactly), capture/restore time, determinism at scale
    (bit-identical or DIVERGED), the memory cap. --sanity smoke-runs
    in CI. Also: the docs now tell one story — the stale milestone
    log (it still said 0.8.1 was "in progress") folded into the
    changelog.
[✓] 0.8.8 — the packaging gate, done: Tools/scripts/consumer-test.sh
    builds and installs the engine, then proves BOTH Embedding paths
    end to end — FetchContent via the lce-doctor init scaffold (the
    doc and the tool agree by construction) and find_package(LCE)
    against the installed prefix. Each consumer builds, links, and
    runs. The gate earned its keep on the first run: it found the
    scaffold's missing LCE::Events import, the harness's
    CMAKE_SOURCE_DIR break when the engine is embedded (a moved
    header is a build break — but so is a wrong root), and the static-
    CRT ABI mismatch — LCE::Core now carries its /MT requirement as
    an INTERFACE option, so consumers link clean without knowing.
    CI runs the gate on every push.
[✓] 0.8.9 — the trust story + the Studio, done:

    The trust story (Docs/Design/TrustStory.md) — the beta's
    promise, structural not patched: remove the DLL, keep the saves;
    the co-save is a shadow (saves load clean with or without the
    mod, either direction); uninstall leaves nothing behind. The
    0.9.0 gate's rehearsal: the remove-the-DLL test, the
    save-round-trip test, the uninstall page.

    LCE Studio — the observation window (beta companion, scoped): a
    zero-dependency GUI — a tiny HTTP server on 127.0.0.1 and ONE
    embedded HTML page; the browser is the window. A live event feed
    (EntityCreated / IntentProduced / OutcomeRecorded /
    RelationshipChanged), an entity table, a mind inspector (needs /
    memory / relationships / intent / goal), and a tuning cockpit
    (sliders over the sim.* keys). Consumer-only through the public
    API — the same shape as the bench and the adapter — so zero core
    surface and the freeze untouched. --selftest is CI's smoke.
    The full vision (in-game attach, the co-save browser, the
    entity editor, the query workbench) stays post-1.0.
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
[ ] LCE Studio (GUI) — the scoped observation-window version is
    0.8.9 (consumer-only, public API only). The full vision — in-game
    attach, the co-save browser, the entity editor, the query
    workbench — waits for the audience beta builds (post-1.0)
