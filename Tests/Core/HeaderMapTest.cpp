//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      HeaderMapTest.cpp
//
// Purpose:
//
//      The public-header surface guard (0.8.1). Two halves, one suite:
//
//        1. The canonical map — every public header, exactly as a
//           downstream project may include it. The harness fails when
//           the Include tree disagrees: a header moved, deleted, or
//           added without updating this map (and the changelog) is a
//           contract break, caught here before it reaches the adapter
//           or a modder's build.
//
//        2. The resolution sweep — every `LCE/...` include referenced
//           anywhere in the engine (headers, sources, tests, samples,
//           tools) must resolve to a real header. A stale include path
//           fails the harness, not a downstream build.
//
//      The map is the freeze list the 0.8.4 API-freeze discipline
//      stands on: appending a header is a changelog-worthy event;
//      moving or removing one requires this map to change first.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "HeaderMapTest.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace LCE::Tests
{
    namespace
    {
        namespace fs = std::filesystem;

        //-------------------------------------------------------------------------
        // The canonical public-header map. Paths are relative to
        // Include/ — exactly how a consumer writes the include. When a
        // header moves, this map must move with it (and the changelog
        // must say so); when one is added, it must be registered here.
        //-------------------------------------------------------------------------
        const char* const kPublicHeaders[] = {
            "LCE/Config/Configuration.h",
            "LCE/Events/Event.h",
            "LCE/Events/EventBus.h",
            "LCE/Logging/LogLevel.h",
            "LCE/Logging/Logger.h",
            "LCE/Runtime/ServiceRegistry.h",
            "LCE/Scheduling/Scheduler.h",
            "LCE/Simulation/Decision/Behaviour.h",
            "LCE/Simulation/Decision/Legacy.h",
            "LCE/Simulation/Decision/Outcome.h",
            "LCE/Simulation/Entity/EntityId.h",
            "LCE/Simulation/Entity/EntityRegistry.h",
            "LCE/Simulation/Entity/RegistrySnapshot.h",
            "LCE/Simulation/Mind/Goals.h",
            "LCE/Simulation/Mind/Memory.h",
            "LCE/Simulation/Mind/Needs.h",
            "LCE/Simulation/Mind/Relationships.h",
            "LCE/Simulation/Simulation.h",
            "LCE/Simulation/SimulationEvents.h",
            "LCE/Simulation/Society/Groups.h",
            "LCE/Simulation/Society/Traits.h",
            "LCE/Simulation/Substrate/Rng.h",
            "LCE/Simulation/Substrate/WorldTime.h",
            "LCE/Tasks/Task.h",
            "LCE/Time/Clock.h",
            "LCE/Version/Version.h",
        };

        constexpr std::size_t kPublicHeaderCount =
            sizeof(kPublicHeaders) / sizeof(kPublicHeaders[0]);

        //-------------------------------------------------------------------------
        // The engine root — the include tree lives at <root>/Include.
        // The build system compiles the real source root in
        // (LCE_SOURCE_ROOT, relative to THIS Tests dir — never
        // CMAKE_SOURCE_DIR, which is the consumer's root when the
        // engine is embedded via FetchContent), so the suite finds its
        // tree no matter what the working directory is: ctest runs the
        // harness from the BUILD tree, where a cwd walk cannot reach
        // the sources. The walk-up from the working directory remains
        // as a fallback for ad-hoc runs from inside the source tree.
        //-------------------------------------------------------------------------
        fs::path EngineRoot()
        {
#ifdef LCE_SOURCE_ROOT
            const auto compiled = fs::path(LCE_SOURCE_ROOT);

            if (fs::exists(
                    compiled / "Include" / "LCE" / "Simulation" / "Simulation.h"))
            {
                return compiled;
            }
#endif

            auto dir = fs::current_path();

            for (;;)
            {
                if (fs::exists(dir / "Include" / "LCE" / "Simulation" / "Simulation.h"))
                {
                    return dir;
                }

                const auto parent = dir.parent_path();

                if (parent == dir)
                {
                    return {};   // nowhere — the sweep fails cleanly below
                }

                dir = parent;
            }
        }

        std::vector<fs::path> WalkHeaders(const fs::path& root)
        {
            std::vector<fs::path> headers;

            if (!fs::exists(root))
            {
                return headers;
            }

            for (const auto& entry : fs::recursive_directory_iterator(root))
            {
                if (entry.is_regular_file()
                    && entry.path().extension() == ".h")
                {
                    headers.push_back(entry.path());
                }
            }

            return headers;
        }

        //-------------------------------------------------------------------------
        // Reads a file's text (empty when missing).
        //-------------------------------------------------------------------------
        std::string ReadText(const fs::path& file)
        {
            std::ifstream stream{ file };

            if (!stream)
            {
                return {};
            }

            std::ostringstream buffer;
            buffer << stream.rdbuf();
            return buffer.str();
        }
    }

    bool HeaderMapTest()
    {
        const auto root = EngineRoot();
        const auto includeRoot = root / "Include";

        if (includeRoot.empty())
        {
            return false;   // root discovery failed
        }

        //-------------------------------------------------------------------------
        // 1. The canonical map matches the tree — every registered header
        //    exists, and every header on disk is registered. A move, a
        //    deletion, or an unregistered addition fails here.
        //-------------------------------------------------------------------------
        {
            for (std::size_t i = 0; i < kPublicHeaderCount; ++i)
            {
                if (!fs::exists(includeRoot / kPublicHeaders[i]))
                {
                    std::printf("HeaderMap: missing %s\n", kPublicHeaders[i]);
                    return false;   // registered but gone — moved or deleted
                }
            }

            const auto onDisk = WalkHeaders(includeRoot);

            for (const auto& header : onDisk)
            {
                const auto relative = fs::relative(header, includeRoot);
                bool registered = false;

                for (std::size_t i = 0; i < kPublicHeaderCount; ++i)
                {
                    if (relative == kPublicHeaders[i])
                    {
                        registered = true;
                        break;
                    }
                }

                if (!registered)
                {
                    std::printf("HeaderMap: unregistered %s\n", relative.string().c_str());
                    return false;   // on disk but not in the map — unregistered
                }
            }
        }

        //-------------------------------------------------------------------------
        // 2. The resolution sweep: every LCE/... include referenced
        //    anywhere in the engine must resolve to a real header. The
        //    adapter and the samples are consumers too — a stale path
        //    here is a build break the harness catches first.
        //-------------------------------------------------------------------------
        {
            const char* scanDirs[] = { "Include", "Source", "Tests", "Samples", "Tools" };

            for (const auto* dir : scanDirs)
            {
                const auto scanRoot = root / dir;

                if (!fs::exists(scanRoot))
                {
                    continue;
                }

                for (const auto& entry : fs::recursive_directory_iterator(scanRoot))
                {
                    if (!entry.is_regular_file())
                    {
                        continue;
                    }

                    const auto ext = entry.path().extension().string();

                    if (ext != ".h" && ext != ".cpp")
                    {
                        continue;
                    }

                    std::istringstream stream{ ReadText(entry.path()) };
                    std::string line;

                    while (std::getline(stream, line))
                    {
                        // Only a real #include line counts — a directive
                        // is the first token on the line. Include-like
                        // text inside string literals (test data) is not
                        // a reference.
                        const auto first = line.find_first_not_of(" \t");

                        if (first == std::string::npos
                            || line.compare(first, 8, "#include") != 0)
                        {
                            continue;
                        }

                        const auto inc = first;
                        const auto marker = line.find("LCE/", inc);

                        if (marker == std::string::npos)
                        {
                            continue;
                        }

                        // The path runs from LCE/ to the closing quote or
                        // bracket — the opening one is behind the marker.
                        const auto quote = line.find('"', marker);
                        const auto angle = line.find('>', marker);

                        std::size_t end = std::string::npos;

                        if (quote != std::string::npos && angle != std::string::npos)
                        {
                            end = (quote < angle) ? quote : angle;
                        }
                        else if (quote != std::string::npos)
                        {
                            end = quote;
                        }
                        else if (angle != std::string::npos)
                        {
                            end = angle;
                        }

                        if (end == std::string::npos)
                        {
                            continue;
                        }

                        const auto path = line.substr(marker, end - marker);

                        if (!fs::exists(includeRoot / path))
                        {
                            std::printf(
                                "HeaderMap: stale include %s in %s\n",
                                path.c_str(), entry.path().string().c_str());
                            return false;   // stale include — resolve it or fix the map
                        }
                    }
                }
            }
        }

        return true;
    }
}
