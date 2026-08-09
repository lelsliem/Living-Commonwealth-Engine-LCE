# Platform Integration Notes (Fallout 4)

**Purpose:** what F4SE and CommonLibF4 can do, and how that shapes the 0.4.0
Fallout 4 adapter — without ever leaking game knowledge into the core.
The core never includes these headers (ADR-0003, ADR-0023); this document is
for the adapter's design.

---

## F4SE — the runtime

`Depends/f4se` (ianpatt/f4se). The Fallout 4 Script Extender.

**What it is:** a launcher (`f4se_loader`) that starts the game with a
patched, plugin-loading runtime. Plugins are DLLs that declare compatibility
and export an entry point.

**The plugin contract** (`f4se/f4se/PluginAPI.h`):

- `F4SEPlugin_Version` — a data struct declaring name, author, and version
  compatibility (address independence: signatures vs Address Library;
  structure independence: which runtime layout).
- `F4SEPlugin_Load(const F4SEInterface*)` — the entry point. From here a
  plugin queries typed interfaces by ID.

**Interfaces a plugin can query:**

| Interface | What it provides | Why LCE's adapter cares |
|-----------|------------------|--------------------------|
| **Messaging** | Game-lifecycle events: `NewGame`, `GameLoaded`, `GameDataReady`, `PreSaveGame`, `PostSaveGame`, `PreLoadGame`, `PostLoadGame`, `DeleteGame`, `InputLoaded`; plus plugin-to-plugin messages. | The simulation's heartbeat: start on `GameLoaded`, suspend/save on save events, reset on load. |
| **Serialization** | Co-save: write/read records inside the game's save file, with save/load/revert callbacks and form-ID resolution. | LCE's save/load stone (roadmap 0.4.0) hooks here — simulation state rides inside the game save. |
| **Papyrus** | Register native functions with the game's script VM; query event registrations. | Lets mods ask the simulation questions through Papyrus. |
| **Task** | `AddTask` / `AddUITask` — run work on the game thread / UI thread. | The thread-safety bridge: simulation may tick on its own thread, but game API calls must be marshalled to the game thread. |
| **Scaleform** | Register Flash/Scaleform UI callbacks. | Optional UI (HUD readouts, debug menus). |
| **Trampoline** | Allocate code from branch/local pools. | Low-level hooking — the adapter should rarely need this directly. |
| **Object** | Delay functors, object registry, persistent object storage. | F4SE's own persistence helpers. |

F4SE also ships `Hooks_*` (input, memory, Papyrus, save/load, Scaleform,
threads, gameplay) — machinery we inherit, not something we write.

## CommonLibF4 — the modern typed API

`Depends/commonlibf4` (libxse/commonlibf4). C++23, xmake build.
*"Intended to replace F4SE as a static dependency. The runtime component of
F4SE is still required."*

**What it is:** a static library that wraps the game's memory layout behind
typed, version-independent C++ classes, plus a modern F4SE API.

**`include/F4SE/`** — the plugin API in modern C++:

- `F4SE::Init(interface, InitInfo{...})` — one call, done.
- `F4SE::GetMessagingInterface()`, `GetSerializationInterface()`,
  `GetPapyrusInterface()`, `GetTaskInterface()`, ... — typed access to the
  same interfaces F4SE exposes raw.
- `F4SE_PLUGIN_LOAD(...)` / `F4SE_PLUGIN_VERSION` macros — the entry points.
- `PluginVersionData` — compatibility declaration, with helpful helpers
  (`UsesAddressLibrary`, `CompatibleVersions`).
- `REX::LOG` — logging with levels and rotation.

**`include/RE/`** — the reverse-engineered game, organized A–X:
`Actor`, `TESForm`, `AIProcess`, `ActorValueOwner`, `BSScript::IVirtualMachine`,
`Scaleform::GFx::Movie` — 1,400+ types in the `Fallout.h` umbrella. Typed
access to game objects and their data, ported against the Address Library so
it survives game updates.

**`REL`** — the version-independent addressing (address library). This is
what lets one plugin build work across runtime versions.

## Licensing — a landmine to plan for

LCE core is MIT. **CommonLibF4 is GPL-3.0-or-later with a modding exception
and a linking exception.** Per its README, linking a mod against CommonLibF4
and distributing it requires distributing source under the same license
(modded code and modding libraries are excepted).

Practical shape for 0.4.0: the **Fallout 4 adapter is a separate F4SE
plugin** that links LCE.Core + CommonLibF4. The adapter (and any code that
links the GPL library) may need GPL licensing, while the engine core stays
MIT. This is a legal decision to review before 0.4.0 — the common pattern in
this ecosystem is an MIT core + GPL adapter, clearly separated.

## How the adapter will fit (design implications)

1. **The adapter is a F4SE plugin** (`F4SEPlugin_Load`), built separately
   from `LCE.Core`, linking it as a static library.
2. **The core never changes for this.** LCE::Simulation entities are pure
   data; the adapter translates them to and from game forms. A settler's
   `Hunger` component becomes `ActorValue` writes through `RE::Actor` —
   the translation happens at the edge (ADR-0024: adapters translate, don't
   simulate).
3. **Lifecycle events** (Messaging) and **co-save** (Serialization) are the
   adapter's two biggest hooks — the simulation starts, ticks, and persists
   around the game's own life.
4. **Threading:** the adapter marshals game-touching work onto the game
   thread via `TaskInterface`. The simulation itself stays
   thread-agnostic.
5. The empty stubs in `Include/LCE/Interfaces/` (`IGameAdapter`, `IWorld`,
   `IEntity`) exist so the core can define the *shape* of the boundary
   without knowing the game. That interface is the 0.4.0 deliverable on the
   core side.

**Not needed for 0.2.0 — 0.3.0:** this knowledge is banked. When we reach
Platform Integration, the adapter project, its license, and the `IGameAdapter`
contract are the stones to lay.
