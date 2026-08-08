=============================================================================

Living Commonwealth Engine (LCE)

Decision Log

=============================================================================



Purpose



Every architectural decision is recorded.



The goal is not simply to remember WHAT was decided,

but WHY it was decided.



Once accepted, a decision should only change if a better

architectural solution exists.



=============================================================================



Decision 0001



Title



Open Source



Status



Accepted



Decision



LCE will be fully open source under the MIT License.



Reason



Teach through code.

Encourage collaboration.

Allow the community to extend the engine.



\-----------------------------------------------------------------------------



Decision 0002



Title



Architecture Before Code



Status



Accepted



Decision



No subsystem will be implemented before it has been designed.



Reason



Reduce technical debt.

Improve documentation.

Keep architecture consistent.



\-----------------------------------------------------------------------------



Decision 0003



Title



Platform Independence



Status



Accepted



Decision



Core engine must never directly include

Fallout 4, F4SE or CommonLibF4 headers.



Reason



Maintain platform independence.



\-----------------------------------------------------------------------------



Decision 0004



Title



Service Registry



Status



Accepted



Decision



Core services are provided through

ServiceRegistry.



Reason



Loose coupling.

Easy testing.

Replaceable implementations.



\-----------------------------------------------------------------------------



Decision 0005



Title



Module Loader



Status



Accepted



Decision



Engine functionality is extended through

loadable modules.



Reason



Stable engine.

Extensible architecture.

Minimal core modifications.



\-----------------------------------------------------------------------------



Decision 0006



Title



Documentation First



Status



Accepted



Decision



Every public class requires



Header

Source

Documentation



Reason



Teach Through Code.



\-----------------------------------------------------------------------------



Decision 0007



Title



Build Philosophy



Status



Accepted



Decision



Every commit must compile successfully.



Reason



Prevent broken branches.



\-----------------------------------------------------------------------------



Decision 0008



Title



Version Strategy



Status



Accepted



Decision



0.x.x Development



1.0 Stable



Semantic Versioning thereafter.



\-----------------------------------------------------------------------------



Decision 0009



Title



Coding Standard



Status



Accepted



Decision



Modern C++23



Visual Studio 2022



Platform Toolset v143



/MT Runtime



Zero warnings



SPDX License headers



\-----------------------------------------------------------------------------



Decision 0010



Title



Project Motto



Status



Accepted



Decision



Building living worlds through simulation.



Reason



Every feature should support this philosophy.



Decision 0015 - Source File Discovery



Status: Accepted ✅



Automatic source discovery using file(GLOB\_RECURSE ... CONFIGURE\_DEPENDS ...)

Each target discovers only its own files.

No repository-wide globbing.

Zero maintenance when adding new source files.

Decision 0016 - Repository Ownership



Status: Accepted ✅



Every directory owns exactly one logical target.



Root

│

├── Source/          -> LCE\_Core

├── Platform/        -> Platform Adapters

├── SDK/             -> LCE\_SDK

├── Tests/           -> LCE\_Tests

├── Samples/         -> Example Applications

├── Modules/         -> Example Modules

├── Docs/            -> Documentation

└── Depends/         -> Third-party libraries



This scales extremely well.



Decision 0017 - Self Contained CMake



Status: Accepted ✅



Every major directory contains its own CMakeLists.txt.



LivingCommonwealthEngine/

│

├── CMakeLists.txt

├── Source/

│   └── CMakeLists.txt

├── Platform/

│   └── CMakeLists.txt

├── SDK/

│   └── CMakeLists.txt

├── Tests/

│   └── CMakeLists.txt

├── Samples/

│   └── CMakeLists.txt

└── Modules/

&#x20;   └── CMakeLists.txt



The root project orchestrates.

Each directory builds itself.



Decision 0018 - Consistent Project Identity



Status: Accepted ✅



Every source file should begin with the official LCE banner.



Including



.cpp

.h

.hpp

CMakeLists.txt

.cmake

Scripts where practical



Consistency matters.



Decision 0019 - Layered Dependency Rule



This one just became obvious while we were talking.



Status: Accepted ✅



Dependencies may only point downward.



Applications

&#x20;     │

&#x20;     ▼

Platform Adapters

&#x20;     │

&#x20;     ▼

Engine

&#x20;     │

&#x20;     ▼

Runtime

&#x20;     │

&#x20;     ▼

Services

&#x20;     │

&#x20;     ▼

Foundation

