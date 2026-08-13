=============================================================================

Living Commonwealth Engine (LCE)

Decision Log

=============================================================================

We record why, not just what.

Every architectural decision is logged. Once accepted, a decision only changes if a better architectural solution exists. After all these decisions, we know exactly why LCE looks the way it does.

---

STATUS: closed record (0.8.9). The API is frozen (0.8.4) and the engine
is feature-complete; the decisions below are the project's reasoning,
not an active ledger. New ADRs are not expected while the freeze is in
force — the log is the "why" a future reader comes here for.

---

=============================================================================



0001

Open Source

Accepted

The Living Commonwealth Engine is fully open source under the MIT License. The simulation belongs to everyone.



ADR-0001 · 2026-01

\-----------------------------------------------------------------------------



0002

C++23 Standard

Accepted

LCE targets C++23. Modern features — concepts, ranges, modules-ready, constexpr improvements — are not just allowed but expected.



ADR-0002 · 2026-01

\-----------------------------------------------------------------------------



0003

Platform Independence

Accepted

The Core never includes Fallout 4, F4SE, or CommonLibF4 headers. Platform-specific code lives exclusively in Platform adapters.



ADR-0003 · 2026-01

\-----------------------------------------------------------------------------



0004

Service Registry

Accepted

Core services are provided through a registry, not direct instantiation. Loose coupling, replaceable implementations, testable by design.



ADR-0004 · 2026-01

\-----------------------------------------------------------------------------



0005

Module System

Accepted

Modules are dynamically loaded extensions. They consume the simulation through the SDK and cannot access Core internals.



ADR-0005 · 2026-01

\-----------------------------------------------------------------------------



0006

Single Coordinator

Accepted

The Engine layer contains one Coordinator. It orchestrates the simulation but never owns gameplay logic. Gameplay lives in modules.

\-----------------------------------------------------------------------------



0007

Event-Driven Architecture

Accepted

Communication between subsystems flows through events, not direct calls. The EventBus is the nervous system.



ADR-0007 · 2026-01

\-----------------------------------------------------------------------------



0008

Simulation Over Scripting

Accepted

Instead of scripting every event, LCE simulates systems from which events naturally emerge. Stories are not written — they occur.



ADR-0008 · 2026-01

\-----------------------------------------------------------------------------



0009

Documentation Before Implementation

Accepted

Every subsystem is designed and documented before a single line of implementation is written. The document is the spec.

\-----------------------------------------------------------------------------



0010

Test Harness

Accepted

LCE owns its own test harness. Tests live in the repository and run on every build. Untested code does not ship.



ADR-0010 · 2026-01

\-----------------------------------------------------------------------------



0011

Semantic Versioning

Accepted

LCE follows semantic versioning: MAJOR.MINOR.PATCH. Breaking API changes require a major version bump. The public contract is sacred.



ADR-0011 · 2026-01

\-----------------------------------------------------------------------------



0012

Public API Stability

Accepted

Once a public API is released, it does not change without deprecation. The SDK is the contract with module authors.



ADR-0012 · 2026-01

\-----------------------------------------------------------------------------



0013

Namespaces Mirror Folders

Accepted

Namespace hierarchy matches folder structure. LCE::Core::Logging lives in Core/Logging. If you can find the folder, you know the namespace.



ADR-0013 · 2026-01

\-----------------------------------------------------------------------------



0014

No Global State

Accepted

No global variables, no static singletons, no hidden mutable state. Everything flows through explicit ownership and the service registry.



ADR-0014 · 2026-01

\-----------------------------------------------------------------------------



0015

Dependency Injection

Accepted

Services receive their dependencies through constructors or the registry. No service reaches out to find another — it is given what it needs.



ADR-0015 · 2026-01

\-----------------------------------------------------------------------------



0016

RAII Resource Management

Accepted

Resources are acquired in constructors and released in destructors. No manual init/shutdown pairs. Ownership is expressed through object lifetime.



ADR-0016 · 2026-01

\-----------------------------------------------------------------------------



0017

Const Correctness

Accepted

Every method that does not modify state is marked const. The compiler enforces immutability. This is not optional.



ADR-0017 · 2026-01

\-----------------------------------------------------------------------------



0018

No Exceptions in Core

Proposed

Core uses std::expected and error codes instead of exceptions. Deterministic, no hidden control flow, no allocation surprises.



ADR-0018 · 2026-02

\-----------------------------------------------------------------------------



0019

Layered Dependency Rule

Accepted

Dependencies point downward only. A layer may depend on the layer below it but never sideways or upward. This single rule prevents circular dependencies.



ADR-0019 · 2026-02

\-----------------------------------------------------------------------------



0020

No Circular Dependencies

Accepted

Circular dependencies are a design failure, not a build problem. If two modules need to know each other, extract the shared concept downward.



ADR-0020 · 2026-02

\-----------------------------------------------------------------------------



0021

The Cathedral Principle

Accepted

Foundations are strengthened before new layers are added. We do not build the second floor until the first is load-bearing. Speed comes from never having to rebuild.



ADR-0021 · 2026-02

\-----------------------------------------------------------------------------



0022

Build for Fallout 4, Architect for Every Bethesda Game

Accepted

The first platform is Fallout 4, but the architecture serves every Bethesda title. If Fallout 5 ships tomorrow, only the adapter changes.



ADR-0022 · 2026-02

\-----------------------------------------------------------------------------



0023

Core Never Includes Game Headers

Accepted

No Fallout 4, F4SE, or CommonLibF4 header appears in Core. The simulation is game-agnostic by compile-time guarantee, not convention.



ADR-0023 · 2026-02

\-----------------------------------------------------------------------------



0024

Adapters Translate, Don't Simulate

Accepted

Platform adapters translate simulation calls to game APIs. They do not contain simulation logic. If logic leaks into an adapter, extract it downward.



ADR-0024 · 2026-02

\-----------------------------------------------------------------------------



0025

Header-Only Foundation Constants

Accepted

Compile-time immutable data stays header-only — no .cpp file, no linker cost, no runtime initialization. Version is the exemplar.



ADR-0025 · 2026-02

\-----------------------------------------------------------------------------



0026

Free Functions Over Static Classes

Accepted

When a class has no state, use free functions in a namespace. Logging's six functions are not methods on a Logger class — they are free functions in LCE::Logging.



ADR-0026 · 2026-02

\-----------------------------------------------------------------------------



0027

Hide Third-Party Libraries

Accepted

Public headers expose only LCE types and standard C++. spdlog, json, and all external libraries are hidden behind the implementation boundary.



ADR-0027 · 2026-02

\-----------------------------------------------------------------------------



0028

Forward Declarations First

Accepted

Prefer forward declarations over includes in headers. Only include what you construct or destroy. Compile times are a feature.



ADR-0028 · 2026-02

\-----------------------------------------------------------------------------



0029

Minimal Dependencies

Accepted

Every external library must justify its maintenance cost. If a feature can be implemented in fifty lines of standard C++, do not add a dependency.



ADR-0029 · 2026-03

\-----------------------------------------------------------------------------



0030

Own the Interface, Not the Implementation

Accepted

LCE owns its public interfaces. Implementations may wrap third-party libraries, but the public API is pure LCE. If spdlog disappears, only Logger.cpp changes.



ADR-0030 · 2026-03

\-----------------------------------------------------------------------------



0031

Design Before Code

Accepted

Each subsystem is designed — interface, responsibilities, dependencies, tests — before implementation begins. The design document is the first deliverable.



ADR-0031 · 2026-03

\-----------------------------------------------------------------------------



0032

One Subsystem at a Time

Accepted

Only one subsystem is in progress at any time. It is designed, documented, implemented, built, and tested before the next begins. No parallel chaos.



ADR-0032 · 2026-03

\-----------------------------------------------------------------------------



0033

The Four Questions

Accepted

Before introducing any new class, subsystem, feature, or dependency, the project leads review: Can it be simpler? Does it belong? Do we need this at all? Will this help build living worlds through simulation?

\-----------------------------------------------------------------------------


0034

Service Registry Shape and Placement

Accepted

The Service Registry lives in LCE::Runtime as a header-only container keyed by std::type_index. Type erasure through std::shared_ptr<void> keeps it dependency-free, replaceable, and testable; the templates live in the header because the erased storage requires their definitions at the call site. If a Services layer is ever introduced, the registry moves with one namespace rename.

ADR-0034 · 2026-08

\-----------------------------------------------------------------------------



\-----------------------------------------------------------------------------

