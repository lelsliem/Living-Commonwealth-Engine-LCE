# The Trust Story (0.8.9)

The beta's hardest question is not "does it work?" — it is "what
happens when it breaks?" A player installs The Living Commonwealth,
lets the world live for forty hours, then one day decides the mod
isn't for them. The trust story is the promise that uninstalling is
safe, saves stay playable, and the world the mod made can be walked
away from without a trace that hurts.

This is the player-facing contract behind the beta's "remove-the-DLL"
test, and the rehearsal for the 0.9.0 gate.

## The three promises

### 1. Remove the DLL, keep the saves

The mod is one file: `TheLivingCommonwealth.dll` in `Data/F4SE/Plugins/`.
Deleting it removes the mod entirely — nothing else is installed, no
loose files, no scripts, no records. The engine keeps no state outside
the co-save (below).

The test: uninstall the mod, load a save that lived with it, play
normally. The game must run clean, and the save must not be damaged —
the world simply stops living. Nothing the mod wrote ever becomes a
requirement for the save to load.

### 2. Saves are safe — the co-save is a shadow

The world's state lives in the **co-save**, the `*.f4se` file F4SE
writes beside every save. It is a shadow: the `.fos` save itself
carries the game's normal state, and the mod's record is separate
data the game never reads.

Consequences, stated plainly:

- A save made **with** the mod loads fine **without** it — the
  `.f4se` record is simply ignored. (The saved world is dormant, not
  corrupt.)
- A save made **without** the mod loads fine **with** it — the mod
  starts a fresh world from the game's current state. No backfill
  hacks, no migrations: the world begins where the game is.
- Deleting the `.f4se` files (or the mod) mid-game costs the sim's
  history — memories, relationships, the ledger — but never the
  game's own progress. The sim is a shadow; the save is the thing.

### 3. Uninstall leaves nothing behind

- Delete the DLL. Delete the co-saves (they are inert without the
  plugin, but tidy players remove them). Delete the mod's log file
  (`My Games/Fallout4/F4SE/TheLivingCommonwealth.log`). That is the
  entire uninstall — no registry, no INI edits outside the mod's own
  optional tuning file, no global F4SE changes.

The one honest caveat: the mod's world lives inside the game's own
data. A settler that learned to trust is a sim-side fact — it fades
with the co-save. The game's world is untouched, because the engine
never writes to it.

## Why this is structural, not patched

The engine was built for this from the start (the adapter contract,
`Docs/AdapterProject.md`): the simulation is a **pure data layer**
over the game. Entities, needs, memory, relationships, goals, the
ledger — all of it lives in the co-save through the snapshot API
(`Capture`/`Restore`/`Clear`). The game itself only ever receives
intents to execute; it never receives the sim's internals. So there
is nothing to clean up, because there is nothing the game ever had
to own.

The same property is why the engine itself is embedder-safe: a game
that removes LCE (or an embedder that abandons it) loses the sim, not
the game. The core never writes outside the state its host hands it.

## The 0.9.0 gate rehearsal

The trust story is a checklist, run before the doors open:

1. **Remove-the-DLL test** — play with the mod, save, remove the DLL,
   load the save, play. Clean? Documented as the mod's uninstall
   test.
2. **Save-round-trip test** — save with the mod, remove the DLL,
   reinstall, load. The co-save restores; the world resumes (the
   adapter's 0.4.0 proof, scaled to a public release).
3. **The uninstall page** — the Nexus description's "Uninstall" line
   is the three bullets above, verbatim. No mystery, no asterisks.

When all three pass on the release build, the beta can truthfully say
what every player asks first: "can I remove it?" — yes, and nothing
breaks.

## The engine's side of the promise

This document is the player-facing contract; the engine-side facts it
stands on live elsewhere:

- The co-save contract: `Docs/AdapterProject.md` (snapshot API,
  serializers, the durable record owned by the adapter).
- The stateless-tick discipline that makes the sim a pure data layer:
  `Docs/Design/CompatPolicy.md` and ADR-0014.
- The freeze that keeps the surface stable through the beta:
  `Docs/Design/Endgame.md`.
