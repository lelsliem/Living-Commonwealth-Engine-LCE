=============================================================================

Living Commonwealth Engine (LCE)



Questions To Ask



Building living worlds through simulation.

=============================================================================



Before writing code, ask yourself:



1\. Can it be simpler?



2\. Does it belong?



3\. Do we need this at all?



4\. Will this help build living worlds through simulation?



\-----------------------------------------------------------------------------



Design Law 001



Simple things should be simple.

Complex things should be composed from simple things

next to copy paste and ask is



PS C:\\LivingCommonwealthEngine> cmake -S . -B Build -G "Visual Studio 17 2022" -A x64



\-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.



\-- Build spdlog: 1.17.0



\-- Build type:



\-- Configuring done (0.2s)



\-- Generating done (0.2s)



\-- Build files have been written to: C:/LivingCommonwealthEngine/Build



PS C:\\LivingCommonwealthEngine> cmake --build Build --config Debug



MSBuild version 17.14.51+25f168cee for .NET Framework







&#x20; Checking File Globs



&#x20; spdlog.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\spdlogd.lib



&#x20; Scanning sources for module dependencies...



&#x20; Clock.cpp



&#x20; Compiling...



&#x20; Clock.cpp



&#x20; LCE.Core.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\LCE.Core.lib



&#x20; Scanning sources for module dependencies...



&#x20; LCE.Core.Tests.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Bin\\Debug\\LCE.Core.Tests.exe



PS C:\\LivingCommonwealthEngine> builds now we need a test 

