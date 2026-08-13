# The Public-Header Audit (0.8.4)

**Design Law 003: Teach through code.** A header that lies about its
behaviour is worse than no header — it teaches the wrong lesson to
every reader after you.

The audit is the last gate before the freeze is in force: every
public header read fresh, every doc claim checked against the
implementation, every wart fixed *while the surface can still
change*. After this milestone, `SurfaceTest` enforces the freeze —
breaking the surface fails the build.

---

## Method

Every file under `Include/LCE/` was read as a *reader*, not as the
author:

1. **Doc vs behaviour** — each documented claim (pass order, stamping
   rule, scaling, defaults, keys) was traced into the implementation:
   `Decide`/`MostUrgent`/`ChooseTarget` in Behaviour.cpp; all six
   passes of `Update`; `Remember`; `ReportOutcome`'s four steps;
   `InheritGroupAttitudes`; `Bequeath`/`InheritMemory`;
   `FixedStep::Advance`; every `FromConfiguration` key.
2. **Namespace vs folder** — every namespace mapped to its directory
   under `Include/LCE/`.
3. **Header-only claims** — e.g. ServiceRegistry claims "no separate
   source file"; verified against `Source/`.
4. **Format** — banners, comment style, the two documentation idioms
   (`///` vs the house `//-----`).

## What held (the good news)

The behaviour documentation is **true**. Verified against the code:

- `Update`'s pass order — needs decay → memory fade → relationship
  drift → goal urgency → decide — matches its doc exactly.
- `Remember` stamps the day only when the caller left it 0; world
  facts (invalid Other) shape nothing; bond crossings publish
  edge-triggered; the group echo spreads disposition, never trust.
- `ReportOutcome`'s four steps — record, scale by result, serve or
  frustrate the goal, consume the intent — match, including the
  "a wrong is a wrong, full loss either way" rule.
- `FromConfiguration`'s keys all resolve: `sim.memory.fade`,
  `sim.memory.forget`, `sim.drift.rate`, `sim.goal.urgency`,
  `sim.trust.gain`, `sim.disposition.gain`, `sim.disposition.loss`,
  `sim.jitter`, `sim.group.inheritance`, `sim.hunger.desperate`,
  `sim.legacy.*`, `sim.memory.cap`, `sim.bond.threshold.*`.
- `Bequeath`/`InheritMemory` — floor, scale, age limit, deterministic
  heir order, bounded append — all as documented.
- **Namespace ↔ folder: zero mismatches.** All 17 namespaces map to
  their directories. The reorg (0.8.1) held.
- **Header-only claims true.** ServiceRegistry really has no `.cpp`;
  nothing to keep in sync.
- **The `WeatherMisty`/`Fog` label-duplication** was already resolved
  by design, not by deletion: the Fact kind (0.8.4) absorbs all new
  fact-types as labels, and the weather kinds stay in the enum because
  the adapter's co-save writes raw ordinals — append-only, never
  removed. Documented in Memory.h; nothing to change.

## What was fixed

### 1. The Goals doc lie (the one real find)

Goals.h claimed urgency "feeds the decision function" — but `Decide`
never reads Goals. A reader would have concluded the opposite of the
truth.

**The truth, now documented from both sides** (Goals.h and
Behaviour.h): Decide reads *needs only*. Goals shape behaviour through
the world's planning layer, which reads `Goals::Active` and biases
needs before the tick — the same channel Weather and Disease use.
`ReportOutcome` serves and frustrates goals; the ambition itself is
the world's to own. **Zero code change** — the behaviour was already
the contract; only the doc lied.

### 2. Logger.h's copy-paste Purpose

The header described itself as providing "compile-time version
information" — Version.h's text, pasted in. Corrected to describe the
logging interface; the doubled banner separator removed.

### 3. Seven malformed banners

Event, Logger, LogLevel, Version, Clock, Task, Configuration — quote
lines that overflowed the box (the 99-bugs lyric, the 100-char
Michelangelo line, the Covey attribution), doubled quotes in Event.h,
and trailing whitespace throughout. All regenerated from the uniform
template; each now carries a one-line quote that fits.

### 4. `/// <summary>` stragglers

Event.h and LogLevel.h used the XML-comment idiom; every other header
uses the house `//-----` block. Converted.

## What was deliberately NOT changed

- **The weather kinds and `Death` stay.** Removing them would corrupt
  the adapter's co-saves (raw ordinals). The enum's growth stops
  here — new fact-types become labels.
- **Goals stays adapter-owned.** Wiring Goals into `Decide` would put
  an interpretation into the core that the world owns. Documented,
  not wired.
- **Field additions to string-bearing structs** (`MemoryEvent`,
  `LegacyFact`, ...) are not size-pinned by `SurfaceTest` (their size
  differs across toolchains). They are governed by the append-only
  compat policy instead — additions are legal, changes to existing
  fields are not.

## The freeze

The audit found warts in *docs*, not in *contracts* — exactly what a
pre-freeze audit should find. `SurfaceTest` (pinned at 0.8.4) now
enforces the result: every public enum ordinal, struct field type,
and function signature is compile-time-pinned, and the freeze is in
force. From here, surface changes are deliberate, changelog-worthy
events — nothing more.
