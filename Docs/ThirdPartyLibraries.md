# Third-Party Libraries

**Purpose:** every third-party library the LCE **core** links against — and
nothing else. The core is game-agnostic, so its dependency list is
deliberately small.

**Platform adapters are a different matter.** Each game brings its own
dependencies; no two games need the same set. Adapter dependencies live
with their adapter's **project** and are never linked into the core
(ADR-0023 — the core never includes game headers). They are listed below
only as a record — the Fallout 4 set was studied, then removed when the
adapter became a separate project.

---

## Core dependencies — linked into LCE.Core

### spdlog

- **Purpose:** high-performance C++ logging library; the backend
  implementation of the LCE logging system.
- **Used by:** `LCE::Logging`
- **Why it is wrapped:** LCE exposes its own logging API — spdlog is never
  exposed publicly, so the backend can be replaced without changing the SDK.
- **License:** MIT
- **Repository:** https://github.com/gabime/spdlog
- **Acquired via:** FetchContent, pinned to `v1.17.0` — the core has no
  vendored code and no submodules. A fresh clone configures and builds
  with no manual steps.
- **Proven in the field (0.4.x):** The Living Commonwealth adapter builds
  the core against the F4SE ecosystem's spdlog (1.16 +
  `SPDLOG_USE_STD_FORMAT`) so one logging implementation serves the
  plugin DLL — with zero changes to LCE's public API. The wrap works.

---

## Platform adapter dependencies — never core

Each game brings its own dependency set; no two games need the same
one. Adapter dependencies live with their adapter's project and are
never linked into the core (ADR-0023 — the core never includes game
headers). The Fallout 4 set (F4SE, CommonLibF4, common, json,
DirectXTK) was studied in `Depends/`, then removed when the adapter
became a separate project (0.4.0) — its provenance is recorded in the
adapter's own `Depends/README.md`, and the adapter boundary is
documented in `Docs/Architecture/PlatformIntegration.md`. This
document lists only what the core links: **spdlog, and nothing else.**
