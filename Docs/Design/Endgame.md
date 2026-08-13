# The Endgame — the engine's locked path to 1.0.0

Locked 2026-08-11. The adapter's verdicts are in (canonical in
Docs/AdapterProject.md); the cuts are final; the point-release ladder
is the working plan. **1.0.0 is a discipline milestone, not a feature
milestone** — six of the seven mod-type patterns need zero new engine
work, and the seventh (disease) needs none either: the adapter owns
health as an edge component and expresses its cost through the
existing Fatigue need.

## The principle

> No new engine surface unless a pattern or an adapter finding names a
> real need. (The Death precedent, applied to the whole endgame.)

Everything between here and 1.0.0 is proving, freezing, and
documenting what exists — not building more.

## The seven patterns, all core-side lean samples

Each is a ~100-line `Sample-<Pattern>` (SampleHost-style, no game)
proving the substrate composes. The engine owns the proof; the adapter
keeps living in-game as the real test. Mapping to shipped substrate:

| Pattern | Substrate (shipped) | New engine surface |
|---|---|---|
| Economy | Trade, Trust, ChooseTarget, market facts, ReportOutcome (0.5.0) | none |
| Legacy | Remember, Bequeath, InheritMemory, LegacyStore (0.7.0) | none |
| Faction Wars | Groups, echo, InheritGroupAttitudes, feud arc (0.6.0) | none |
| Weather | weather facts → Decide gating (0.5.x) | none |
| Children | Birth, Bequeath/InheritMemory, household bonds (0.6.0/0.7.0) | none |
| Disease & Medicine | fact-plus-tick recipe: a health fact + the Fatigue need | none (adapter owns the Health component) |
| Roads | road quality as facts, ChooseTarget, adapter pathing | none |

## The cuts (verdicted — final)

1. **Threading the tick off the game thread** — cut from 1.0.0. The
   core stays single-threaded and deterministic; FixedStep makes
   game-thread ticking cheap and timing-independent. The adapter
   measures first (a TickReport log once a minute in-game); thread
   only if the number says so. Likely never.
2. **C ABI / Papyrus natives** — cut. The C++ API is the surface.
3. **Networking / replication** — cut. Single-player.
4. **Multi-agent negotiation / deep planning AI** — cut. Beyond the
   promise.
5. **LCE Studio (GUI)** — cut. The CLI Doctor and the log are enough.
6. **Per-fact legacy decay** — cut, with a standing condition: revisit
   only if the soak data or a mechanic demands it.
7. **MCM / radio audio** — cut. The INI delivers tuning; captions
   deliver the radio; audio is an asset question, post-1.0.
8. **New InteractionKinds / components / events** — the door stays
   closed unless a pattern or a finding names a real need.

## The point-release ladder (one at a time, each a verifiable tag)

- **0.8.1** — Housekeeping & the re-roll fix (landed 2026-08-12,
  engine side done, 31/31): the ADR-0029 field fix
  (`Rng::StableDerive` — per-entity noise anchors to the seed, never
  the live state), the Simulation folders reorganized into category
  subfolders, and the public surface guarded (HeaderMapTest + LCE
  Doctor include-layout check). Remaining gate: the adapter's
  in-game verification of 0.8.0 Scale.
- **0.8.2** — the four proven patterns as samples, all done: Economy
  (prices are memories), Legacy (death, inheritance, the name that
  outlives the voice), Weather (seasons and day-stamped weather facts
  shaping needs), Children (family as a group: inherited attitudes and
  per-child traits) — each zero new surface, plus the JitteredTraits
  per-trait fix the Children sample surfaced.
- **0.8.3** — the three harder patterns, all done: Faction Wars
  (groups as the map, dispositions as loyalty, InheritGroupAttitudes
  as indoctrination), Disease (quarantine door-fact + Fatigue toll;
  the world owns Health), Roads (LegacyStore routes whose weight is
  condition, maintained by traffic and forgotten by neglect).
- **0.8.4** — the API Freeze, done: the personality
  tie-break and per-need metabolism (the seam traits multiply into),
  the Fact kind + label string (the enum stops growing; labels
  absorb it), the compat policy doc (what is stable, what is
  append-only — InteractionKind ordinals, snapshot schema — and
  what breaks when), the surface-stability test (SurfaceTest:
  every public enum ordinal, struct field type, and function
  signature pinned by static_assert — the freeze is enforced at
  COMPILE time, so a drifted surface cannot even build), and the
  public-header audit (every header read fresh against its
  implementation; the Goals seam documented honestly; seven
  malformed banners regenerated; zero API change). The freeze is
  now in force — surface changes fail the harness and need the
  changelog.
- **0.8.5** — the docs, done: LearningPath complete (the tour now
  runs Version → Scale: 19 stops, eleven samples, thirteen
  exercises), the full audit written up (Docs/Design/Audit.md), and
  Embedding.md extended with the 0.8.0 onboarding recipe (FixedStep +
  TickReport + sim.memory.cap) so a non-Fallout embedder starts from
  one doc.
- **0.8.6** — CI, done: GitHub Actions builds and tests on MSVC, GCC,
  and Clang every push — the freeze enforced cross-compiler, the
  ctest wiring, the LCE.Bench smoke, and the installed-package
  version fixed (0.5.0 → 0.8.x; find_package told the truth).
- **0.8.7** — LCE Bench, done: the Scale numbers measured on any
  machine (ms/tick at population with the per-pass breakdown, co-save
  bytes per mind, round-trip, determinism, the memory cap) — the
  0.9.0 gate becomes measurable, not asserted. The stale milestone
  log folded into the changelog: one story.
- **0.9.0 — RC "The Freeze"** — the freeze *in force*: everything
  from here is fixes-only. Public beta live (the mod on Nexus, the
  engine via GitHub), feedback through Nexus comments and GitHub
  issues — async and unsocial by design. Soak for feedback.
- **1.0.0** — the gate proven: the samples show all seven mod types,
  The Living Commonwealth is live in Fallout 4, MIT, the promise made
  good.

## The freeze gate (decided)

The surface-stability test **prevents** API change during 0.9.0 and
records it before 0.8.4 — so the freeze audit can still fix warts, and
once the freeze is in force, breaking the surface fails the harness.

## The two tracks

The adapter versions its mod independently (its ReleasePlan.md: 0.7.1
Talk, 0.7.2 Rows, 0.7.3 Fights, 0.8.0 Trade with anyone, 0.8.1 Illness
& Medicine, 0.9.0 its release gate, 1.0.0). The two tracks cross-sync
only through Docs/AdapterProject.md and the shared milestones (0.9.0
RC, 1.0.0).
