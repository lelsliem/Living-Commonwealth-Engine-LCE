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



PS C:\\LivingCommonwealthEngine> cmake --build Build --config Debug



CMake is re-running because C:/LivingCommonwealthEngine/Build/CMakeFiles/generate.stamp is out-of-date.



&#x20; the file 'C:/LivingCommonwealthEngine/CMakeLists.txt'



&#x20; is newer than 'C:/LivingCommonwealthEngine/Build/CMakeFiles/generate.stamp.depend'



&#x20; result='-1'



\-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.



\-- Build spdlog: 1.17.0



\-- Build type:



\-- Configuring done (0.0s)



\-- Generating done (0.2s)



\-- Build files have been written to: C:/LivingCommonwealthEngine/Build



MSBuild version 17.14.51+25f168cee for .NET Framework







&#x20; Checking File Globs



&#x20; 1>Checking Build System



&#x20; spdlog.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\spdlogd.lib



&#x20; Scanning sources for module dependencies...



&#x20; Logger.cpp



&#x20; Compiling...



&#x20; Logger.cpp



&#x20; LCE.Core.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\LCE.Core.lib



&#x20; Building Custom Rule C:/LivingCommonwealthEngine/CMakeLists.txt



PS C:\\LivingCommonwealthEngine>test successful and double tap deleleted build folder test again an ocd a good one lol PS C:\\LivingCommonwealthEngine> cmake --build Build --config Debug



CMake is re-running because C:/LivingCommonwealthEngine/Build/CMakeFiles/generate.stamp is out-of-date.



&#x20; the file 'C:/LivingCommonwealthEngine/CMakeLists.txt'



&#x20; is newer than 'C:/LivingCommonwealthEngine/Build/CMakeFiles/generate.stamp.depend'



&#x20; result='-1'



\-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.



\-- Build spdlog: 1.17.0



\-- Build type:



\-- Configuring done (0.0s)



\-- Generating done (0.2s)



\-- Build files have been written to: C:/LivingCommonwealthEngine/Build



MSBuild version 17.14.51+25f168cee for .NET Framework







&#x20; Checking File Globs



&#x20; 1>Checking Build System



&#x20; spdlog.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\spdlogd.lib



&#x20; Scanning sources for module dependencies...



&#x20; Logger.cpp



&#x20; Compiling...



&#x20; Logger.cpp



&#x20; LCE.Core.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\LCE.Core.lib



&#x20; Building Custom Rule C:/LivingCommonwealthEngine/CMakeLists.txt



PS C:\\LivingCommonwealthEngine> cmake -S . -B Build -G "Visual Studio 17 2022" -A x64



\-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.



\-- The CXX compiler identification is MSVC 19.42.34444.0



\-- Detecting CXX compiler ABI info



\-- Detecting CXX compiler ABI info - done



\-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe - skipped



\-- Detecting CXX compile features



\-- Detecting CXX compile features - done



\-- Build spdlog: 1.17.0



\-- Performing Test CMAKE\_HAVE\_LIBC\_PTHREAD



\-- Performing Test CMAKE\_HAVE\_LIBC\_PTHREAD - Failed



\-- Looking for pthread\_create in pthreads



\-- Looking for pthread\_create in pthreads - not found



\-- Looking for pthread\_create in pthread



\-- Looking for pthread\_create in pthread - not found



\-- Found Threads: TRUE



\-- Build type:



\-- Looking for \_fwrite\_nolock



\-- Looking for \_fwrite\_nolock - found



\-- Configuring done (4.2s)



\-- Generating done (0.1s)



\-- Build files have been written to: C:/LivingCommonwealthEngine/Build



PS C:\\LivingCommonwealthEngine> cmake --build Build --config Debug



MSBuild version 17.14.51+25f168cee for .NET Framework







&#x20; Checking File Globs



&#x20; 1>Checking Build System



&#x20; Building Custom Rule C:/LivingCommonwealthEngine/Depends/spdlog/CMakeLists.txt



&#x20; spdlog.cpp



&#x20; stdout\_sinks.cpp



&#x20; color\_sinks.cpp



&#x20; file\_sinks.cpp



&#x20; async.cpp



&#x20; cfg.cpp



&#x20; bundled\_fmtlib\_format.cpp



&#x20; Generating Code...



&#x20; spdlog.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\spdlogd.lib



&#x20; Building Custom Rule C:/LivingCommonwealthEngine/Source/CMakeLists.txt



&#x20; Scanning sources for module dependencies...



&#x20; Logger.cpp



&#x20; LoggerBackend.cpp



&#x20; Version.cpp



&#x20; Compiling...



&#x20; LoggerBackend.cpp



&#x20; Logger.cpp



&#x20; Version.cpp



&#x20; LCE.Core.vcxproj -> C:\\LivingCommonwealthEngine\\Build\\Lib\\Debug\\LCE.Core.lib



&#x20; Building Custom Rule C:/LivingCommonwealthEngine/CMakeLists.txt



PS C:\\LivingCommonwealthEngine>

