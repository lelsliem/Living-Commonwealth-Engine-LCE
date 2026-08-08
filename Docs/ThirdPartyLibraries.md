=============================================================================

Library

=============================================================================



Name



spdlog



\-----------------------------------------------------------------------------



Purpose



High-performance C++ logging library used as the backend implementation for

the LCE logging system.



\-----------------------------------------------------------------------------



Used By



LCE::Logging



\-----------------------------------------------------------------------------



Why It Is Wrapped



LCE exposes its own logging API and does not expose spdlog directly.



This allows the logging backend to be replaced in the future without changing

the public SDK.



\-----------------------------------------------------------------------------



License



MIT



\-----------------------------------------------------------------------------



Official Repository



https://github.com/gabime/spdlog

