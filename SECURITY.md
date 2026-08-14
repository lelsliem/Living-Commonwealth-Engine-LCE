# Security Policy

## Reporting a vulnerability

Report security issues privately, not in public issues. Use GitHub's
private vulnerability reporting on this repository:

<https://github.com/lelsliem/Living-Commonwealth-Engine-LCE/security/advisories/new>

## What happens next

- We acknowledge the report within five business days.
- We work on a fix and a release, and coordinate disclosure with you.
- We ask that you do not publicly disclose the issue until a fix ships.

## Supported versions

| Version | Supported |
|---|---|
| 0.9.x | ✅ |
| < 0.9.0 | ❌ |

## The security boundary

LCE is a static library embedders link into a game adapter: a flaw in
the core is a flaw in every embedding. Treat the core's memory-safety
claims — no raw ownership games, deterministic seeded RNG, the
adapter-vocabulary-free surface — as the boundary. The adapter's world
(save files, tuning, labels) is data the core never interprets.
