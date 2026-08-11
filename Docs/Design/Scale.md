# 0.8.0 — Scale · "The Settlement Survives" — Design

Locked 2026-08-11. The engine must hold a living Commonwealth, not a
village. Five stones, mapping 1:1 to the roadmap's five items; the
proof sentence: *a settlement of hundreds ticks inside a frame budget;
a year of sim time passes without drift.*

## The state today (what Scale stands on)

- The tick (`Update`) is five full passes: needs decay, memory fade,
  relationship drift, goal urgency, Decide. Each sweeps every entity
  that has the component, every call.
- The per-entity cost is dominated by **memory and relationships, not
  the population**: Decide's `ChooseTarget`/`IsUnavailable`/
  `FindThreat` scan all of a mind's events each tick; fade walks them;
  drift walks all pairs. A mind's cost grows with experience.
- Determinism is "same seed + same frame deltas". Variable-step
  `Update(delta)` means two machines at different frame rates drift
  apart.
- The co-save round-trips (v2 schema); proven at 673 minds in-game,
  unmeasured at thousands.

## The cost model (documented, stone 13)

**cost per tick ≈ Σ over minds of (O(events) + O(relationships)) +
O(population)** — independent of the number of stores, bounded only by
the memory cap. Measured shape at 5000 seeded minds (the Scale suite,
stone 16): **207 bytes per mind** in the co-save (needs + three memory
events + one relationship), well under a kilobyte. The game-thread
decision (0.4.0) stays as-is: the core is single-threaded and
deterministic by construction; threading the tick off the game thread
is the adapter's execution choice, made safe by the fixed-step
accumulator (stone 14b).

## The five stones

### Stone 13 — Tick budgets & profiling

`TickReport` — an opt-in measurement of one Update call: per-pass
counts (minds swept, events faded, pairs drifted) and per-pass wall
time. A non-null `TickReport*` fills it; **nullptr (the default)
measures nothing** — every existing caller is untouched. The adapter
passes one when it wants the numbers (e.g. to log the cost of a
settlement per frame); the core never benchmarks against a machine it
can't see — the *shape* is provable here, the in-game wall clock is
the adapter's.

Proven by the Scale suite: exact counts on a known registry; the
default path stays unmeasured.

### Stone 14a — The memory cap (`sim.memory.cap`, default 0)

A mind can only hold so much. When the cap is set, the lowest-weight
event is evicted on insert (ties → oldest, deterministic; the store is
append-ordered). **Cap 0 = unbounded = the default = unchanged
behavior.** This is what bounds the hot path: with the cap set,
`ChooseTarget`, `IsUnavailable`, `FindThreat`, and fade are all O(cap),
and the adapter's per-second market facts can no longer grow a mind's
memory without bound.

Decided: default 0. The core doesn't invent "how much a mind
remembers" — that's world vocabulary; the adapter tunes it once it
measures in-game.

### Stone 14b — FixedStep: the timing-independent tick

`FixedStep` — the adapter feeds real frame deltas (which vary); the
sim advances in whole fixed steps (which don't). Same seed + same
steps = same world, whatever the frame rate — variable frame deltas
were the #1 drift source, and fixed steps remove them by construction.
`Update(delta)` remains the raw primitive (ADR'd, backward compatible);
`FixedStep::Advance` is an opt-in composition helper; the host chooses
the cadence (`Step`, default 0.1s) and reads the step count to know how
many intents this frame produced.

Deliberately not built: staggered/phase-shifted passes — FP-divergent
from the current math, and the cap buys more with less machinery.

Proven by the Scale suite: the carry mechanics in exact-binary
seconds; and two identical worlds at different frame rates end
bit-identical (the timing-independence proof).

### Stone 15 — Soak tests (the finding is the point)

Two cadences, one suite:
- **A decade at day-steps** — 300 settlers, 3650 day-steps: calendar
  edges, bounded memory, no entity creep, no NaN.
- **An hour at the real tick cadence** — 30 settlers, 36,000 × 0.1s
  steps via FixedStep: FP accumulation on the actual hot path; the
  world provably moved (hunger decayed).

Whatever the decade exposes — a slow leak, a cap interaction, an FP
edge — is the milestone's real output, written back here.

### Stone 16 — Save/load at population scale

The Scale suite's integration block: 5000 minds round-trip
capture → restore → capture **exactly** (flattened byte-identical),
and the per-mind co-save cost is documented (207 bytes, above).

### Stone 17 — Determinism verified at scale

Two worlds, same seed, same steps, 1000 minds — flattened snapshots
**byte-for-byte identical** (the flattened form sorts entities by ID
and components by type, so the stores' unordered iteration can never
leak in). This is the strongest determinism statement in the engine's
history, and the property 1.0.0's save-compat promise stands on.

## Decisions (locked 2026-08-11)

1. Memory cap default **0** (world tunes; unchanged behavior).
2. `Update` keeps its raw signature; `FixedStep` is the opt-in helper.
3. The core stays single-threaded and deterministic; threading is the
   adapter's execution choice, documented here.
4. The core proves the cost *shape*; the adapter owns the in-game
   wall clock.
5. Soak runs two cadences: a decade at day-steps + an hour at 0.1s.

## The hand-off

Docs/AdapterProject.md carries the 0.8.0 hand-over. The adapter's
moves: set `sim.memory.cap` in the INI once it measures; adopt
`FixedStep` for its tick loop (feeding real frame deltas, reading the
step count); pass a `TickReport` when it wants per-frame costs; keep
the co-save as-is (schema unchanged — the cap lives in the tuning and
the tick, not the record).
