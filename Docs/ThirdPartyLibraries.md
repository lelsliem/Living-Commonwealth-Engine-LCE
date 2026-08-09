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

---

## Platform adapter dependencies — not core

The Fallout 4 set was vendored in `Depends/` for study, then removed when
the adapter became a separate project (0.4.0). The core never links them;
the adapter project brings its own. Future adapters bring their own sets
and document them with their adapter.

| Dependency  | What it is                              | License                             |
|-------------|-----------------------------------------|-------------------------------------|
| F4SE        | runtime + plugin contract               | BSD-3-Clause                        |
| CommonLibF4 | typed game API (C++23, static library)  | GPL-3.0 + modding/linking exceptions|
| common      | shared foundation (REL/address library) | zlib                                |
| json        | serialization (nlohmann)                | MIT                                 |
| DirectXTK   | DirectX Tool Kit                        | MIT                                 |

Details, the licensing landmine, and the adapter boundary:
`Docs/Architecture/PlatformIntegration.md`.
