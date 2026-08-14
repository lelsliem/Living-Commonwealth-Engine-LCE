# Changelog

All notable changes to the Living Commonwealth Engine (LCE).

## [0.9.1] — 2026-08-14 — the release automation

The engine surface is unchanged — this is the shipping infrastructure.
Push-time CI now runs Debug AND Release on four platforms (Windows
MSVC, Ubuntu GCC, Ubuntu Clang, macOS Clang), CodeQL scans every push
and weekly, the nightly covers Release overnight, and the Release
workflow builds, installs, and publishes the per-platform SDK archives
when a version tag is pushed — gated on the packaging gate running in
Release mode (consumer-test.sh takes the config as its second
argument). The README carries CI/nightly/release/code-scan badges,
issue templates and a security policy are in place, the doctor and
Embedding.md pins moved to v0.9.1, and a local Linux+GCC container dev
loop (Dockerfile + Tools/scripts/linux-gcc-loop.sh) reproduces the
ubuntu-gcc leg before push. v0.9.1 shipped via the new Release
workflow — the first release the automation made itself. A GitHub
Pages workflow renders the docs (README, LearningPath, the design
and decision documents) as a site — one-time setup: Settings →
Pages → Source: "GitHub Actions".

## [0.9.0] — 2026-08-13 — public beta: the doors open

The API freeze is in force, every suite green (32/32), and the engine
is released for anyone to embed. The public beta is the release gate:
feedback runs through GitHub issues (engine) and Nexus comments (the
mod), async and unsocial by design. Since v0.8.5, the ladder was
climbed in full — CI on three toolchains, the benchmark, the
packaging gate, the trust story, the Studio — and the version moves
from alpha to **beta**. The mod (The Commonwealth Lives, a separate
repo) releases alongside.

CI is green on all three legs for real now — the first run's only
passing job was ubuntu-clang. The suites were never running: ctest
found nothing because `enable_testing()` lived in Tests/ instead of
at the project root (the top-level CTestTestfile.cmake is what
`ctest --test-dir Build` reads), so every leg silently ran zero
tests. With it moved, HeaderMap needed its engine root compiled in
(LCE_SOURCE_ROOT) — its cwd-walk fails from the build tree ctest
runs in. The tool smokes now match the platform's executable suffix
(LCE.Bench vs LCE.Bench.exe — MSVC was dying with exit 127 on an
empty path), the packaging gate sets a build type (CMake ≥ 4 only
emits an install-export's per-config file when one is set; without
it, `find_package(LCE)` consumers fail with "IMPORTED_LOCATION not
set") and reads the version with sed instead of GNU-only
`grep -oP`, and the GCC leg's build failure was GCC 13+'s -Wchanges-meaning,
now an error by default: the frozen API names members after their
types (`IntentProducedEvent::Intent`, `OutcomeRecordedEvent::Outcome`)
— a pattern Clang and MSVC accept silently — so the diagnostic is
silenced for GCC only and the API stays untouched. The repo's
canonical URL dropped its trailing hyphen
(Living-Commonwealth-Engine-LCE); every reference — Embedding
recipe, doctor scaffold, git remote — points at the renamed repo.

A CI status badge now sits in the README, a nightly workflow builds
the Release configuration (optimized, asserts off) on all three
toolchains, the frozen naming convention is written down in
CompatPolicy (the member-named-after-its-type idiom, `Intent Intent;`
/ `Outcome Outcome;`), the build and tool-smoke steps were extracted
into shared scripts (Tools/scripts/ci-build.sh, ci-tool-smokes.sh),
and a local Linux+GCC dev loop (Dockerfile +
Tools/scripts/linux-gcc-loop.sh) reproduces the ubuntu-gcc leg in a
container before push. Push-time CI now runs Release alongside Debug
on all three toolchains (six legs), and a Release workflow builds,
installs, and publishes the per-platform SDK archives when a version
tag (v*) is pushed.

## [0.8.9] — 2026-08-13 — the trust story and the Studio

**The trust story** (`Docs/Design/TrustStory.md`) — the beta's
promise, structural rather than patched, and the 0.9.0 gate's
rehearsal. Three promises: remove the DLL and keep the saves; the
co-save is a shadow (a save loads clean with or without the mod, in
either direction — the sim is a pure data layer that never writes to
the game's own state); uninstall leaves nothing behind. The remove-
the-DLL test, the save-round-trip test, and the uninstall page are
checked before the doors open.

**LCE Studio** (`Tools/LCEStudio`, `LCE.Studio`) — the observation
window, scoped to be the beta companion. The GUI is deliberately
zero-dependency: a tiny HTTP server on 127.0.0.1 (std sockets, with a
winsock/POSIX shim) and ONE embedded HTML page — the browser is the
window. A village of thirty settlers + a trader ticks at 20 Hz on one
thread; the page watches:

- **The event feed** — EntityCreated / IntentProduced /
  OutcomeRecorded / RelationshipChanged pushed from the bus, no
  polling (a bounded ring buffer, cursor-polled by the page).
- **The entity table** — every mind, live needs, current intent and
  confidence; click a row for the full picture.
- **The mind inspector** — one settler, everything: needs with decay
  rates, memories with days and labels, relationships (disposition
  + trust), the current intent, the active goal.
- **The tuning cockpit** — sliders over the sim.* keys
  (sim.memory.fade, sim.drift.rate, sim.jitter, ...), POSTing live
  into the running tick.

Consumer-only through the public API — the same shape the bench
proved — so zero core surface and the freeze untouched. Threading is
taught in the source: one mutex makes the world safe to watch.
`--selftest` starts on an ephemeral port, makes one internal request,
and exits — CI's smoke that the tool runs on every toolchain.

Version 0.8.9-alpha. The ladder's last rungs are climbed — **0.9.0,
the release gate, is next.**

## [0.8.8] — 2026-08-13 — the packaging gate, automated

**The gate** (`Tools/scripts/consumer-test.sh`) — builds and installs
the engine, then proves BOTH Embedding paths from
`Docs/SDK/Embedding.md` end to end:

- **Path 1 — FetchContent via `lce-doctor init`** — the doctor now
  scaffolds a minimal embedder (CMakeLists.txt + main.cpp + host.ini)
  straight from the recipe: the doc and the tool agree by
  construction. A generated project that compiles and runs IS the
  proof the recipe is real.
- **Path 2 — `find_package(LCE)`** — the recipe verbatim against the
  freshly installed prefix.

Each consumer builds, links, and runs. The gate runs in CI on every
push.

**What the gate found on its first run** — three real bugs, all
fixed:

1. **The scaffold's missing import** — the generated main.cpp used
   `EventBus` (LCE::Events) but only imported LCE::Simulation; a
   broken recipe that no reader would catch until compile time.
2. **The harness's `CMAKE_SOURCE_DIR` break** — the Doctor suite and
   the Doctor tool resolved `Tools/` against the top-level source
   dir, which is the *consumer's* root when the engine is embedded
   via FetchContent. The engine's own tests failed for embedders.
   Both now resolve against their own tree (`CMAKE_CURRENT_SOURCE_DIR`),
   so the harness builds wherever the engine does.
3. **The static-CRT ABI mismatch** — the core compiles with the static
   runtime (MTd), so any consumer linking `LCE::Core` must too; a
   default MSVC consumer uses the dynamic runtime and died with
   LNK2038. Rather than pushing that burden onto every embedder, the
   package now carries its own requirement: `LCE::Core` propagates
   `/MT[d]` as an INTERFACE compile option (MSVC consumers only —
   GCC/Clang untouched), so linking `LCE::Core` just works through
   both FetchContent and the installed package.

**LCE Doctor** grew the `init` subcommand (scaffold an embedder from
the recipe), harness-tested in the Doctor suite — including that the
scaffold passes the doctor's own checks, and that bad names are
refused.

## [0.8.7] — 2026-08-13 — LCE Bench: the Scale numbers, measured

**LCE Bench** (`Tools/LCEBench`, `LCE.Bench`) — the Scale numbers,
reproduced on any machine. Same seed (2026), same scenarios, same
methodology as the Scale suite: settler-shaped minds (needs + three
memories + one relationship) around one trader. It measures:

- **Tick cost at population** — ms/tick averaged over 200 whole steps
  at 1000 / 5000 / 20000 minds, with the per-pass breakdown
  (needs / memory / relationships / goals / decide). The decide pass
  dominates, as documented — the number is the 0.9.0 gate, measured
  on the target box instead of asserted.
- **The co-save** — bytes per mind (the documented ~207: at 5001
  entities the tool prints exactly 1,040,032 bytes / 207 per mind),
  capture and restore wall time, and whether the round-trip is exact.
- **Determinism at scale** — two worlds, same seed, 1000 minds,
  1000 steps: bit-identical, or the tool says DIVERGED.
- **The memory cap** — forty remembers into a cap: every mind stays
  at or under it.

`--sanity` runs a tiny scenario and exits — CI's smoke check that
the tool runs on every toolchain (timing is machine-dependent by
nature; the tool prints, the gate reads).

**The docs tell one story now.** The milestone log (milestone.md) was
folded into this changelog and retired — it had drifted behind (it
still said 0.8.1 was "in progress"), and the changelog already held
the same milestones, more current. References updated: the README's
docs list now points here.

Version bumped to 0.8.7-alpha. 32/32 suites, eleven samples green.

## [0.8.6] — 2026-08-13 — CI: three toolchains, every push

The engine's first CI. A GitHub Actions workflow (`.github/workflows/ci.yml`)
builds and runs every suite on three toolchains on every push to main
and every pull request:

- **windows-msvc** — `/W4 /WX` (warnings are errors), the same
  discipline as local development.
- **ubuntu-gcc** and **ubuntu-clang** — portability: what one
  toolchain accepts is not the contract until all three do.

The freeze is the point. `SurfaceTest` pins every enum ordinal, struct
field type, and function signature at compile time — CI makes that
fail on EVERY compiler, and the size guards on the ABI-stable co-save
structs must hold cross-compiler. The suites run through ctest (new
`enable_testing` + `add_test` wiring); samples and tools build too;
and `LCE.Bench --sanity` smoke-runs after the suites, so the Scale
numbers at least run on every toolchain even though their timing is
machine-dependent.

**Two truths restored along the way.** The installed package version
(`project(VERSION ...)` in the root CMakeLists) still said 0.5.0 — a
`find_package(LCE)` consumer was being told a version the engine
hasn't been for six milestones; it now tracks Version.h (0.8.x). And
Embedding.md's pins pointed at `v0.5.0`; they now point at the
released `v0.8.5` tag with `find_package(LCE 0.8 REQUIRED)`.

Version bumped to 0.8.6-alpha. 32/32 suites, eleven samples green.

## [0.8.5] — 2026-08-13 — The docs: the full audit, the complete LearningPath, the Embedding recipe

The milestone that makes the engine *readable* — everything since
0.5.0 finally taught as a course.

**LearningPath complete.** The tour previously stopped at the
snapshot; it now runs Version → Scale in 19 stops — the whole
simulation layer added: the Mind (Needs, Memory, Relationships,
Goals), Behaviour/Decide, the tick (Update/Remember/ReportOutcome),
the Substrate (Rng, WorldTime), Society (Groups, Traits), Legacy,
Observation & Query, and Scale (FixedStep, TickReport, MemoryCap) —
each with its lesson. All eleven samples listed with their one-line
teaching (Economy, Legacy, Weather, Children, Faction Wars, Disease,
Roads joined Farmer, Village, Market, Host); the exercises grew from
8 to 13 (query order, observation, byte-identical determinism, the
door fact, traits into decisions).

**The full audit** (`Docs/Design/Audit.md`). The 0.8.4 pass written
up: method (doc vs behaviour, namespace vs folder, header-only
claims, format), what held (the behaviour docs are true; zero
namespace-folder mismatches; the reorg held), what was fixed (the
Goals doc lie, Logger.h's copy-paste Purpose, seven malformed
banners, `///` stragglers), and what was deliberately NOT changed
(the weather kinds stay for co-save ordinals; Goals stays
adapter-owned; string-bearing structs are append-only, not
size-pinned).

**Embedding.md: the 0.8.0 runtime recipe.** The doc previously
covered packaging; it now carries a complete minimal host — one
loop (FixedStep + TickReport + sim.memory.cap) with the three keys
an embedder needs first, the bus, and the co-save three-liner. A
non-Fallout embedder starts from one doc.

Version bumped to 0.8.5-alpha. 32/32 suites, eleven samples green.

## [0.8.4] — 2026-08-13 — The freeze work begins: personality into decisions

The first stones of the API Freeze, each a schema or behaviour decision
made deliberately while the surface can still change.

**The personality tie-break.** A mind with two near-tied needs was
decided by list order — whichever need the world happened to push
first. `Decide` now resolves needs within a small band (0.05) of the
most urgent by a per-need seeded draw: same seed + same entity + same
needs = same choice, every run, and the choice is the same whatever
order the needs are listed in (the QueryWhere discipline). This is the
seam a world's traits multiply into — a bold mind's Safety can win its
attention over a barely-more-urgent Hunger. Proven by a BehaviourTest
block: the same entity with the same needs in both list orders makes
the same decision, and the same seed re-rolls identically.

**Per-need metabolism.** Needs decayed at one shared rate per entity;
now each need decays at its own seeded rate, keyed on the need TYPE —
Hunger and Safety metabolize differently. Same seed + same entity +
same need = same rate, every tick; the key is never the list index,
so identical needs listed differently metabolize identically. Without
an Rng the rate is exactly 1.0 — behaviour unchanged for every
existing caller.

**The Fact kind.** `InteractionKind::Fact` + `MemoryEvent::Label` —
the world's own name for a fact ("radstorm", "plague", "the old road
must hold") rides the event; the core fades it, forgets it, and never
interprets it. The weather kinds and Death stay in the enum (the
adapter's co-save writes raw ordinals — append-only, never removed);
new fact-types become labels, not enum entries. Proven by a Snapshot
round-trip: a labeled Fact survives capture and restore when the
world's serializer chooses to carry the label.

**Compat policy** (`Docs/Design/CompatPolicy.md`): what is stable,
what is append-only, what the world owns, and what breaks when — the
freeze contract written down.

**The surface-stability test (SurfaceTest, the freeze's teeth).**
HeaderMapTest froze the FILE map (every path a consumer can include);
SurfaceTest freezes the DECLARATIONS inside those headers. Every
public enum ordinal (InteractionKind, ActionType, NeedType, GoalType,
OutcomeResult, Season, LogLevel — the ordinals the adapter's co-save
writes raw), every public struct field type, and every public member
and free-function signature — each pinned by a `static_assert` that
names the declaration in its message. The freeze is enforced at
compile time: the harness cannot even BUILD against a drifted surface,
and the error says exactly what moved. Size guards pin the
ABI-stable co-save structs (Intent, Outcome, Need, Goal, Relationship,
WorldTime); string-bearing structs are pinned by field type, with
additive change governed by the append-only compat policy. The suite
was proven live: a simulated `InteractionKind::Fact` ordinal shift
failed the build with `static_assert failed: 'InteractionKind::Fact
ordinal (co-save critical - append-only)'`, then restored green.

**The public-header audit (the box's last item).** Every header in
`Include/LCE/` was read fresh and every doc claim checked against its
implementation. The behaviour docs all held — Update's pass order,
Remember's stamping, ReportOutcome's four steps, Bequeath/InheritMemory,
FromConfiguration's keys. What the audit found and fixed:

- **The Goals doc lie.** Goals.h claimed urgency "feeds the decision
  function" — but `Decide` never reads Goals. Now documented honestly
  from both sides (Goals.h and Behaviour.h): Decide reads needs only;
  goals influence through the world's planning layer biasing needs
  before the tick, the same channel Weather and Disease use.
  ReportOutcome serves and frustrates; the ambition is the world's to
  own. Zero code change — the behaviour was already the contract.
- **Logger.h's copy-paste Purpose lie** — it claimed to "provide
  compile-time version information" (Version.h's text). Now describes
  the logging interface; the doubled separator is gone.
- **Seven malformed banners regenerated** to the uniform template
  (Event, Logger, LogLevel, Version, Clock, Task, Configuration):
  quote lines that overflowed the box (the 99-bugs lyric, the
  Michelangelo line, the Covey attribution) now fit on one line, the
  doubled-quote in Event.h is gone, and the trailing whitespace is
  cleaned. New one-liners: "99 little bugs in the code — and they're
  all mine.", "Every block of stone has a statue inside it.", "The
  key is in not spending time, but in investing it.", "Simple things
  should be simple; complex things composed from them.", "My life is a
  bad config file — full of defaults I never agreed to."
- **`/// <summary>` stragglers** (Event.h, LogLevel.h) converted to
  the house `//-----` style.

Zero API change — the freeze surface (SurfaceTest) is untouched. The
freeze is now in force. **32/32 suites, eleven samples clean.**

## [0.8.3] — 2026-08-13 — The three harder pattern samples

**Faction Wars (SAMPLE 8).** Territory, sieges, and diplomacy as
groups and dispositions. `LCE.SampleFactionWars` shows a wrong from a
comrade and a kindness from an enemy accumulating into a crossing:
mara's disposition toward the ally sinks (-0.50) while the enemy
diplomat rises (+0.20), the world reads the crossing, the membership
flips, and InheritGroupAttitudes makes the new faction's grudges her
own — the decision follows the feelings (Socialize toward the
former enemy).

**Disease (SAMPLE 9).** Outbreaks as facts and ticks, honoring the
adapter's 0.8.0 verdict (Health is adapter-owned). `LCE.SampleDisease`
shows the loop the core supplies: a quarantine is a world fact with
an invalid Other — the Trade door closes while it is remembered and
reopens the moment it fades (no script ordered a halt); the sick
mind's toll is Fatigue held urgent (the fever takes the appetite,
so rest becomes the loudest voice); recovery is rest, and the
settlement remembers the outbreak as a fading, day-stamped fact.

**Roads (SAMPLE 10).** Routes that improve with traffic and degrade
with neglect, as LegacyStore facts. `LCE.SampleRoads` shows a road's
Weight IS its condition: use maintains it, weather wears it, the day
stamp moves only with use — so the neglected road decays and the
wrecked road falls out of the world's books while the young road
grows under the rerouted caravans.

All three zero new engine surface. **31/31 suites, eleven samples
clean.** Docs updated (roadmap, endgame, handoff).

## [0.8.2] — 2026-08-13 — The four pattern samples

**The Economy (SAMPLE 4).** Proof that a whole living economy needs
zero new engine surface. `LCE.SampleEconomy` shows dynamic pricing,
supply chains, trade routes, and market events as pure memory: the
price of bread is what the market remembers about last harvest. A
delivery is a memory (supply, cheaper); a blight is a memory
(scarcity, dearer); the price is a pure function of remembered facts,
so it spikes with the blight (9→14 caps) and fades back as the
memory dies — no ledger, no price field, no script. The route to
market is Trust, remembered (six fair trades, 0.63 Trust). The
supply chain is a chain of memories.

**Legacy (SAMPLE 5).** Death is three functions and a fact.
`LCE.SampleLegacy` shows Bequeath (salient facts pass to the heir,
fainter — a passing kindness below the floor dies with its owner),
LeaveLegacy (the old bridge survives the keeper), and InheritMemory
(a generation later, only the recent and the wanted travel — the old
feud and the grief stay with the dead).

**Weather (SAMPLE 6).** A sky that behaves. `LCE.SampleWeather`
shows the calendar (seasons derived from the day alone) and weather
as day-stamped facts that shape need: a radstorm makes safety the
loudest voice, so the farmer flees the remembered raiders; clear
skies leave hunger speaking, so the farmer walks to market. The sky
never tells the mind what to do — it changes which need is urgent.

**Children (SAMPLE 7).** A family is a group. `LCE.SampleChildren`
shows birth (join the family group), InheritGroupAttitudes (the child
inherits the family's mean disposition — trust is never inherited),
JitteredTraits (per-child personality from the seeded RNG), and
personal experience beating inherited (one kind act warms the
inherited distrust; one fair trade earns the first trust).

**One engine fix fell out of the Children sample.** `JitteredTraits`'
RNG path re-derived the child stream per trait and took only its first
draw, so every trait of an entity came out identical. It now derives
one child stream and advances it per trait — boldness and sociability
are different draws again. Regression assertion added to TraitsTest.

All four built entirely on the public surface: Memory facts,
ReportOutcome, relationships, groups, traits, the calendar, Bequeath
— the same loop every adapter walks.

## [0.8.1] — 2026-08-12 — Housekeeping & the re-roll fix

Three field fixes and housekeeping changes landed after 0.8.0, each
verified before the next began; the version is bumped to 0.8.1-alpha.

**The re-roll fix (field finding from the adapter).** The adapter's
in-game hunt (ADR-0029) found the tick's per-entity noise followed the
Rng's LIVE state, so its births — legitimately drawn from the same
Rng it passes to Update — re-rolled every mind's metabolism and
confidence every frame. A near-tied Rest/Explore mind flipped its
intent every tick (22k log lines in three minutes, the drag behind
the frame hang). The adapter throttled its log; the engine fixed the
root cause.

- `Rng::StableDerive(key)` — a child stream anchored to the SEED,
  never the live state. Same seed + same entity = same noise every
  run, however far the parent has advanced. `Derive` is unchanged
  (state-anchored, documented as such).
- Both per-entity call sites (needs-decay rate, Decide's confidence
  jitter) now use `StableDerive` — a settled mind rests, not
  re-rolls. Backward compatible: with a parent that never advances
  (all engine tests) the two are byte-identical.
- Proven by the new Jitter block: two same-seed worlds, one whose
  parent advances three draws between every tick, stay bit-identical
  in decay AND decision. 30/30 suites green at the time.

**Simulation folder reorganized.** The flat `Include/LCE/Simulation/`
and `Source/Simulation/` piles were split into category subfolders:
`Entity/` (EntityId, EntityRegistry, RegistrySnapshot), `Mind/`
(Needs, Memory, Relationships, Goals), `Society/` (Groups, Traits),
`Decision/` (Behaviour, Outcome, Legacy), and `Substrate/` (Rng,
WorldTime). `Simulation.h` / `SimulationEvents.h` stay at the
Simulation root — the tick and its events keep their paths. No
namespaces changed, no API changed — only file locations and the
include paths to them. Every engine and adapter reference re-synced;
the adapter rebuilt green (21/21) against the reorganized core.

**The surface is now guarded.** Two additions make the SDK's public
surface self-checking — the mechanical teeth the 0.8.4 API freeze
stands on:

- **HeaderMapTest (new suite, 31/31)** — the canonical public-header
  map is now frozen in the harness. Any header moved, deleted, or
  added without updating the map fails the run with the exact path
  named; a second sweep resolves every `LCE/...` include referenced
  anywhere in the engine.
- **LCE Doctor: include layout check** — the doctor now verifies that
  every `LCE/...` include a project references resolves to a real
  header in the core it pins (a moved header is a build break the
  doctor names first). The SDK contract grew to six checks.

## [0.8.0] — 2026-08-11 — Scale · "The Settlement Survives"

Engine side shipped (design locked the same day —
Docs/Design/Scale.md); the adapter's in-game verification is the
remaining gate.

- TickReport (stone 13) — the cost of a settlement is knowable:
  per-pass counts and wall time, opt-in; nullptr (the default)
  measures nothing. The documented model: per tick ~Σ over minds of
  (O(events) + O(relationships)) + O(population).
- sim.memory.cap (stone 14a) — a mind can only hold so much: the
  lowest-weight event is evicted on insert (ties → oldest),
  bounding Decide's scans and the fade pass. Default 0 = unbounded,
  unchanged behavior; the world tunes it.
- FixedStep (stone 14b) — the timing-independent tick: real frame
  deltas in, whole fixed steps out. Same seed + same steps = same
  world at any frame rate. Update(delta) stays the raw primitive.
- Soak tests (stone 15) — a decade at day-steps and an hour at the
  real cadence: no NaN, no entity creep, memory bounded.
- Save/load at scale (stone 16) — 5000 minds round-trip
  byte-exactly; 207 bytes per mind documented.
- Determinism at scale (stone 17) — two worlds, same seed, 1000
  minds: flattened snapshots byte-for-byte identical.
- 30/30 test suites green. Backward compatible: Update gained a
  defaulted TickReport* parameter; every existing caller builds
  untouched.

## [0.7.0] — 2026-08-11 — Legacy · "The Debt to the Past"

Engine and adapter shipped together and verified in-game
(2026-08-11): the feud chain ran in the wild — shut stalls, desperate
arrivals, blame, rival bonds, mediated grudges.

- Bequeath (stone 10) — what an entity bequeaths as it goes: the
  world names the heirs; the core keeps facts at or above
  sim.legacy.bequestFloor, scaled by sim.legacy.inheritanceScale,
  their own world day intact. Append, never overwrite;
  deterministic heir order. Proven by the Bequeath suite.
- InheritMemory (stone 11) — descendants inherit memory,
  selectively: the world's predicate selects; the core scales and
  ages (sim.legacy.maxAgeDays). The grandson proof is the
  Inheritance suite's integration block — the feud outlived its
  owner.
- Legacy as world fact (stone 12) — LegacyFact + LegacyStore:
  LeaveLegacy/ReadLegacy/ForgetLegacy, permanent until the world
  deletes, riding the co-save through the world's serializer
  (snapshot schema v2). Proven by the WorldLegacy suite.
- sim.hunger.desperate (field fix, the adapter's handover `81cfe48`)
  — below the threshold a remembered Trade world fact no longer
  blocks the trip: a starving mind pushes the shut door, so an
  arrival can land on a closed market and the refusal can happen.
  Default 0.0 = never desperate, existing behavior untouched; the
  adapter sets 0.2 in its INI. Proven by the Behaviour and Tuning
  suites and verified in-game by the adapter (the feud's gate, now
  live).
- 28/28 test suites green.
Each entry is a released milestone — see Docs/Roadmap.md for the full
arc. This changelog is the single record: the former milestone log
(Docs/milestone.md) was folded here (0.8.7) so the story lives in one
place.

## [0.6.0] — 2026-08-11 — Society · "The Bonds Between Minds"

Shipped with The-Commonwealth-Lives 0.6.0-beta (the adapter), verified
in-game the same day: bonds form, couples marry into households, deaths
spread as gossip, sim-only children are born and grow.

- Bond thresholds + RelationshipChangedEvent (stone 08, the adapter's
  Request A) — the world names its own bond lines
  (sim.bond.threshold.<name>); an experience crossing one publishes the
  moment, edge-triggered and drift-quiet; payload: subject, other,
  disposition, trust, threshold name, world day. Proven by the
  BondThreshold suite.
- Configuration::ForEach — lets tuning discover the threshold list.
- Groups (stone 09) — GroupId + the Groups membership component; the
  echo: trust is earned personally, disposition travels to group-mates
  at sim.group.inheritance; InheritGroupAttitudes: feelings inherit
  from the group, then diverge. Proven by the Groups suite.
- Traits (stone 09) — named-float personality component with
  JitteredTraits: deterministic per-entity variation under the seeded
  RNG; the influence is the world's to apply. Proven by the Traits
  suite.
- InteractionKind::Death — the core names the fact so adapters can
  record deaths in memory; a fact, never a door (Decide gates only
  Trade and Social).
- Version bumped to 0.6.0-alpha. 25/25 test suites green; Remember
  gained a defaulted EventBus* — every existing caller builds
  untouched.

## [0.5.0] — 2026-08-10 — SDK & Samples · "The Consumable Engine"

- The complete boundary contract — the decide → act → observe →
  remember loop: the Outcome channel (ReportOutcome); observation
  events (EntityCreated, IntentProduced, OutcomeRecorded); the query
  surface (QueryWhere<T>, ascending EntityId order);
  SimulationTuning::FromConfiguration; the seeded Rng (splitmix64,
  Derive for order-independent noise); WorldTime with seasons and
  memory day stamps; per-mind decay jitter (sim.jitter).
- SDK: the Sample Host (the money test live — fair twice, cheated
  twice, then Bellamy), Sample Modules (farmer, village, market), LCE
  Doctor (the CLI), packaging (install targets + find_package(LCE),
  verified end to end), and Docs/SDK/Embedding.md as the official
  recipe.
- Version bumped to 0.5.0-alpha; both GitHub repos live and tagged
  v0.5.0. The adapter (The-Commonwealth-Lives) verified the full loop
  in-game.

## [0.4.0] — Platform Integration — the Fallout 4 adapter

- The co-save substrate: RegistrySnapshot — Capture/Restore/Clear with
  registered component serializers; the durable record's type names and
  versioning are the adapter's to own. Proven by the Snapshot suite.
- The adapter (separate repo): the F4SE plugin scaffold, entity ↔ form
  translation, the intent executor, co-save integration (record v4),
  and the first in-game test — settlers with needs in Fallout 4.

## [0.3.0] — Simulation

- EntityRegistry, Needs, Memory, Relationships, Goals, Behaviour (the
  Decide loop), and SimulationTick — the seven suites that made the
  tick real. 0.3.1 was the polish pass: banner headers, doc truth, and
  the first honest commit discipline.

## [0.2.0] — Entity System

- The component architecture: the EntityRegistry and the components
  the simulation composes from (the T-templated adapter pattern).

## [0.1.0] — Core Runtime (Services)

- The service layer: Configuration, Logging (spdlog), EventBus, Clock,
  Scheduler, Task, ServiceRegistry, and compile-time Version — the
  substrate every later milestone stands on.

## [0.0.x] — Foundation

- The blueprint: vision, philosophy, architecture, the development
  charter, and the decision log (ADR-0001 onward).
