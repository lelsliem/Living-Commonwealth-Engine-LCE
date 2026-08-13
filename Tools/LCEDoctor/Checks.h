//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │
// │                       ██╗      ██████╗███████╗
// │                       ██║     ██╔════╝██╔════╝
// │                       ██║     ██║     █████╗
// │                       ██║     ██║     ██╔══╝
// │                       ███████╗╚██████╗███████╗
// │                       ╚══════╝ ╚═════╝╚══════╝
// │
// │            Building living worlds through simulation.
// │
// │        “A doctor that only reports health was never a doctor.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · LCE Doctor
//
// The checks, pure: each takes paths and returns a verdict. No CLI, no
// printing — so the harness can test them against crafted directories,
// and the CLI is just the same checks with a voice.
//
// The SDK contract a project must meet:
//   1. The target is a real directory.
//   2. It has a build system (CMakeLists.txt or xmake.lua).
//   3. It wires LCE (FetchContent / find_package / the lce.core rule).
//   4. The core checkout it points at is present (when local).
//   5. Every LCE/... include the project references resolves to a real
//      header in that core — a moved header is a build break the
//      doctor names before the compiler does.
//   6. The toolchain is visible (CMake / xmake / MSVC).
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace LCE::Doctor
{
    namespace fs = std::filesystem;

    // getenv is fine here — a developer tool, not a network surface.
    // MSVC's deprecation warning is scoped to this one helper.
    inline const char* SafeGetEnv(const char* name) noexcept
    {
#ifdef _MSC_VER
        __pragma(warning(push))
        __pragma(warning(disable : 4996))
#endif
        return std::getenv(name);
#ifdef _MSC_VER
        __pragma(warning(pop))
#endif
    }

    //-------------------------------------------------------------------------
    // Report — one check's verdict and the reason, in plain words.
    //-------------------------------------------------------------------------
    struct Report
    {
        bool Passed = false;
        std::string Detail;
    };

    //-------------------------------------------------------------------------
    // 1. The target exists and is a directory.
    //-------------------------------------------------------------------------
    inline Report CheckTarget(const fs::path& target)
    {
        if (target.empty())
        {
            return { false, "no target given — pass a directory (or run inside one)" };
        }

        if (!fs::exists(target))
        {
            return { false, "no such path: " + target.string() };
        }

        if (!fs::is_directory(target))
        {
            return { false, "not a directory: " + target.string() };
        }

        return { true, "target is a directory: " + target.string() };
    }

    //-------------------------------------------------------------------------
    // 2. A build system is present. Reports which one(s) it found.
    //-------------------------------------------------------------------------
    inline Report CheckBuildSystem(const fs::path& target)
    {
        const bool cmake = fs::exists(target / "CMakeLists.txt");
        const bool xmake = fs::exists(target / "xmake.lua");

        if (cmake && xmake)
        {
            return { true, "both CMake (CMakeLists.txt) and xmake (xmake.lua)" };
        }

        if (cmake)
        {
            return { true, "CMake (CMakeLists.txt)" };
        }

        if (xmake)
        {
            return { true, "xmake (xmake.lua)" };
        }

        return { false, "no CMakeLists.txt or xmake.lua — how does this project build?" };
    }

    //-------------------------------------------------------------------------
    // Reads a file's text (or an empty string when missing/unreadable).
    //-------------------------------------------------------------------------
    inline std::string ReadText(const fs::path& file)
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

    //-------------------------------------------------------------------------
    // 3. The project wires LCE — the pinned FetchContent recipe,
    //    find_package(LCE), or the adapter's lce.core rule.
    //-------------------------------------------------------------------------
    inline Report CheckLWiring(const fs::path& target)
    {
        const auto cmake = ReadText(target / "CMakeLists.txt");
        const auto xmake = ReadText(target / "xmake.lua");

        const auto find = [](std::string_view haystack, std::string_view needle)
        {
            return haystack.find(needle) != std::string_view::npos;
        };

        // LCE::Core — the imported target both FetchContent and
        // find_package(LCE) provide — is the wiring that matters; the
        // scaffold lce-doctor init generates links it.
        if (find(cmake, "LCE::Core")
            || find(cmake, "LivingCommonwealthEngine")
            || find(cmake, "find_package(LCE")
            || find(cmake, "LCE.Core"))
        {
            return { true, "CMake wires LCE (FetchContent / find_package / LCE.Core)" };
        }

        if (find(xmake, "lce.core") || find(xmake, "LCE_CORE_PATH"))
        {
            return { true, "xmake wires LCE via the lce.core rule" };
        }

        if (!cmake.empty() || !xmake.empty())
        {
            return { false,
                "no LCE wiring found — add the pinned FetchContent recipe, "
                "find_package(LCE), or the lce.core rule" };
        }

        return { false, "no build file to inspect — nothing wires LCE yet" };
    }

    //-------------------------------------------------------------------------
    // 4. The core checkout is present at the referenced path (when the
    //    project points at one). FetchContent projects have no local
    //    checkout to verify — the build fetches and pins it.
    //-------------------------------------------------------------------------
    inline Report CheckCoreCheckout(const fs::path& corePath)
    {
        if (corePath.empty())
        {
            return { true,
                "no local core path to verify — FetchContent fetches and pins it "
                "(the tag is the pin)" };
        }

        const auto versionHeader =
            corePath / "Include" / "LCE" / "Version" / "Version.h";

        if (!fs::exists(versionHeader))
        {
            return { false,
                "core checkout not found: " + corePath.string()
                    + " (missing Include/LCE/Version/Version.h)" };
        }

        const auto text = ReadText(versionHeader);

        // Version.h defines the constants as inline constexpr int.
        const auto constant = [&text](std::string_view name) -> std::uint32_t
        {
            const auto namePos = text.find(name);

            if (namePos == std::string::npos)
            {
                return 0;
            }

            const auto eq = text.find('=', namePos);

            if (eq == std::string::npos)
            {
                return 0;
            }

            try
            {
                return static_cast<std::uint32_t>(
                    std::stoul(text.substr(eq + 1)));
            }
            catch (...)
            {
                return 0;
            }
        };

        const auto major = constant("MajorValue");
        const auto minor = constant("MinorValue");
        const auto patch = constant("PatchValue");

        std::string version =
            std::to_string(major) + "." + std::to_string(minor)
            + "." + std::to_string(patch);

        // The alpha suffix, when the string carries it.
        const auto stringPos = text.find("VersionString");

        if (stringPos != std::string::npos
            && text.find("alpha", stringPos) != std::string::npos)
        {
            version += "-alpha";
        }

        return { true, "core checkout: " + version + " at " + corePath.string() };
    }

    //-------------------------------------------------------------------------
    // Collect every LCE include referenced under a directory: all
    // #include "LCE/..." and #include <LCE/...> lines in .h/.cpp files.
    // Sorted, deduplicated — a set, so the doctor can name each stale
    // reference once.
    //-------------------------------------------------------------------------
    inline std::vector<std::string> CollectLceIncludes(const fs::path& root)
    {
        std::vector<std::string> found;

        if (root.empty() || !fs::exists(root))
        {
            return found;
        }

        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const auto ext = entry.path().extension().string();

            if (ext != ".h" && ext != ".hpp" && ext != ".cpp")
            {
                continue;
            }

            std::istringstream stream{ ReadText(entry.path()) };
            std::string line;

            while (std::getline(stream, line))
            {
                // Only a real #include line counts — a directive is the
                // first token on the line. Include-like text inside
                // string literals (test data) is not a reference.
                const auto first = line.find_first_not_of(" \t");

                if (first == std::string::npos
                    || line.compare(first, 8, "#include") != 0)
                {
                    continue;
                }

                // #include "LCE/Simulation/..." or #include <LCE/...>
                const auto open = line.find("LCE/", first);

                if (open == std::string::npos)
                {
                    continue;
                }

                const auto quote = line.find('"', open);
                const auto angle = line.find('>', open);

                std::size_t end = std::string::npos;

                if (quote != std::string::npos && angle != std::string::npos)
                {
                    end = (quote < angle) ? quote : angle;
                }
                else
                {
                    end = (quote != std::string::npos) ? quote : angle;
                }

                if (end == std::string::npos)
                {
                    continue;
                }

                // The include path starts at the LCE/ marker and ends at
                // the closing quote or bracket.
                found.push_back(line.substr(open, end - open));
            }
        }

        std::sort(found.begin(), found.end());
        found.erase(std::unique(found.begin(), found.end()), found.end());

        return found;
    }

    //-------------------------------------------------------------------------
    // 5b. Include layout: every LCE/... include a project references must
    //     resolve to a real header in the core's Include tree. A stale
    //     path (a moved header, a typo) is a build break waiting — the
    //     doctor names each one before the compiler gets the chance.
    //-------------------------------------------------------------------------
    inline Report CheckHeaderLayout(const fs::path& project, const fs::path& core)
    {
        if (core.empty())
        {
            return { true, "no local core to check against — FetchContent pins the headers" };
        }

        const auto includeRoot = core / "Include";

        if (!fs::is_directory(includeRoot))
        {
            return { false, "core has no Include/ tree: " + includeRoot.string() };
        }

        const auto references = CollectLceIncludes(project);

        if (references.empty())
        {
            return { false, "no LCE/... includes found under " + project.string() };
        }

        std::vector<std::string> stale;

        for (const auto& include : references)
        {
            if (!fs::exists(includeRoot / include))
            {
                stale.push_back(include);
            }
        }

        if (!stale.empty())
        {
            std::string detail = "stale include path"
                + (stale.size() == 1 ? std::string{ "" } : std::string{ "s" })
                + ": " + stale.front();

            for (std::size_t i = 1; i < stale.size() && i < 5; ++i)
            {
                detail += ", " + stale[i];
            }

            if (stale.size() > 5)
            {
                detail += " (+ " + std::to_string(stale.size() - 5) + " more)";
            }

            detail += " — does not exist under " + includeRoot.string();

            return { false, std::move(detail) };
        }

        return { true,
            std::to_string(references.size())
                + " LCE includes, all resolve under " + includeRoot.string() };
    }

    //-------------------------------------------------------------------------
    // 6. The toolchain is visible: CMake and xmake on PATH, MSVC via the
    //    developer-environment variables.
    //-------------------------------------------------------------------------
    inline bool OnPath(std::string_view name)
    {
        const char* path = SafeGetEnv("PATH");

        if (path == nullptr)
        {
            return false;
        }

        std::istringstream stream{ path };
        std::string entry;

        while (std::getline(stream, entry, ';'))
        {
            if (fs::exists(fs::path{ entry } / name))
            {
                return true;
            }
        }

        return false;
    }

    inline Report CheckToolchain()
    {
        const bool cmake = OnPath("cmake.exe");
        const bool xmake = OnPath("xmake.exe");

        const char* vcTools = SafeGetEnv("VCToolsInstallDir");
        const char* vsInstall = SafeGetEnv("VSINSTALLDIR");

        const bool msvc = vcTools != nullptr && vsInstall != nullptr;

        std::string detail = cmake ? "cmake ✓" : "cmake ✗ (add it to PATH)";
        detail += xmake ? ", xmake ✓" : ", xmake ✗ (add it to PATH)";
        detail += msvc
            ? ", MSVC ✓"
            : ", MSVC ✗ (run from a Developer prompt or set VCToolsInstallDir)";

        return { cmake && msvc, detail };
    }

    //-------------------------------------------------------------------------
    // ScaffoldEmbedder (0.8.8) — lce-doctor init.
    //
    // Writes a minimal embedder project straight from the Embedding.md
    // recipe, so the doc and the tool agree by construction: the
    // scaffold IS the recipe, generated by the tool. Three files:
    //   CMakeLists.txt — the pinned FetchContent path (Path 1)
    //   main.cpp       — the 0.8.0 runtime recipe (FixedStep + the bus)
    //   host.ini       — the modder's knob (sim.* keys)
    //
    // The packaging gate (Tools/scripts/consumer-test.sh) builds this
    // scaffold end to end — a generated project that compiles and runs
    // is the proof that the recipe is real.
    //-------------------------------------------------------------------------
    inline Report ScaffoldEmbedder(
        const fs::path& parent,
        const std::string& name)
    {
        // The name is a single path component — never a path of its
        // own (no separators, no traversal, no dots).
        const auto bad = [&name]()
        {
            return name.empty()
                || name.find('/') != std::string::npos
                || name.find('\\') != std::string::npos
                || name == "." || name == "..";
        };

        if (bad())
        {
            return { false,
                "invalid project name: \"" + name
                    + "\" — use a single plain name, e.g. myworld" };
        }

        const auto project = parent / name;

        if (fs::exists(project))
        {
            return { false,
                "already exists: " + project.string()
                    + " — choose a new name or delete it first" };
        }

        fs::create_directories(project);

        const auto write =
            [&project](const char* file, const std::string& contents)
        {
            std::ofstream{ project / file } << contents;
        };

        //-------------------------------------------------------------------------
        // The pinned FetchContent recipe (Docs/SDK/Embedding.md, Path 1).
        // GIT_TAG is the released version to build against.
        //-------------------------------------------------------------------------
        write("CMakeLists.txt",
            "# Generated by lce-doctor init (0.8.8) — the Embedding.md recipe\n"
            "# as a project. The doc and this file agree by construction.\n"
            "\n"
            "cmake_minimum_required(VERSION 3.28)\n"
            "project(" + name + " LANGUAGES CXX)\n"
            "\n"
            "set(CMAKE_CXX_STANDARD 23)\n"
            "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
            "\n"
            "include(FetchContent)\n"
            "FetchContent_Declare(\n"
            "    lce\n"
            "    GIT_REPOSITORY https://github.com/lelsliem/Living-Commonwealth-Engine-LCE-.git\n"
            "    GIT_TAG        v0.8.8   # pin to the released version you build against\n"
            ")\n"
            "FetchContent_MakeAvailable(lce)\n"
            "\n"
            "add_executable(" + name + " main.cpp)\n"
            "target_link_libraries(" + name + " PRIVATE LCE::Core)\n");

        //-------------------------------------------------------------------------
        // The runtime recipe (Embedding.md, 0.8.0): one loop, three
        // knobs, the bus, the co-save three-liner. A working program.
        //-------------------------------------------------------------------------
        write("main.cpp",
            "// Generated by lce-doctor init (0.8.8) — the Embedding.md\n"
            "// runtime recipe as a running program: one loop, three\n"
            "// knobs, the bus. This file IS the recipe, generated by the\n"
            "// tool so the doc and the code agree by construction.\n"
            "#include \"LCE/Config/Configuration.h\"\n"
            "#include \"LCE/Events/EventBus.h\"\n"
            "#include \"LCE/Simulation/Simulation.h\"\n"
            "#include \"LCE/Version/Version.h\"\n"
            "\n"
            "#include <chrono>\n"
            "#include <cstdio>\n"
            "#include <fstream>\n"
            "#include <string>\n"
            "#include <thread>\n"
            "\n"
            "using namespace LCE::Simulation;\n"
            "using namespace LCE::Events;\n"
            "\n"
            "int main()\n"
            "{\n"
            "    // Tuning from the modder's text file (host.ini beside this\n"
            "    // program): known keys override defaults, broken values\n"
            "    // keep the default, unknown keys are ignored.\n"
            "    LCE::Config::Configuration config;\n"
            "    {\n"
            "        std::ifstream in{ \"host.ini\" };\n"
            "        std::string line;\n"
            "\n"
            "        while (std::getline(in, line))\n"
            "        {\n"
            "            const auto eq = line.find('=');\n"
            "\n"
            "            if (eq != std::string::npos)\n"
            "            {\n"
            "                config.Set(line.substr(0, eq), line.substr(eq + 1));\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "\n"
            "    const SimulationTuning tuning =\n"
            "        SimulationTuning::FromConfiguration(config);\n"
            "\n"
            "    // The inputs — nothing is global (ADR-0014):\n"
            "    EntityRegistry registry;   // entities + components\n"
            "    Rng rng{ 2026 };           // the seeded stream\n"
            "    EventBus events;           // the push channel\n"
            "\n"
            "    // A minimal world to watch: one trader, one hungry mind.\n"
            "    const auto trader = registry.CreateEntity();\n"
            "    const auto mind = registry.CreateEntity();\n"
            "\n"
            "    registry.AddComponent<Needs>(\n"
            "        mind, Needs{ { Need{ NeedType::Hunger, 0.8f, 0.02f } } });\n"
            "\n"
            "    Remember(registry, mind, { trader, InteractionKind::Trade, 1.0f });\n"
            "\n"
            "    std::printf(\n"
            "        \"Consumer linked against %s — the loop runs.\\n\",\n"
            "        std::string(LCE::Version::String()).c_str());\n"
            "\n"
            "    // FixedStep: real frame deltas in, whole fixed steps out.\n"
            "    FixedStep step;\n"
            "\n"
            "    for (int i = 0; i < 60; ++i)\n"
            "    {\n"
            "        step.Advance(0.1, registry, tuning, &events, &rng);\n"
            "\n"
            "        // In a game you read the bus here — IntentProducedEvent\n"
            "        // tells you what each mind wants; you execute it.\n"
            "\n"
            "        std::this_thread::sleep_for(std::chrono::milliseconds(50));\n"
            "    }\n"
            "\n"
            "    // The co-save three-liner (register serializers at init):\n"
            "    //   const RegistrySnapshot snap = registry.Capture();\n"
            "    //   registry.Restore(snap);\n"
            "    //   registry.Clear();\n"
            "\n"
            "    std::printf(\n"
            "        \"The loop ran — save/load, tuning, and observation live in\"\n"
            "        \" Docs/SDK/Embedding.md.\\n\");\n"
            "    return 0;\n"
            "}\n");

        //-------------------------------------------------------------------------
        // The modder's knob — the same keys FromConfiguration reads.
        //-------------------------------------------------------------------------
        write("host.ini",
            "# Generated by lce-doctor init (0.8.8) — the modder's knob.\n"
            "# Known keys override defaults; broken values keep the default;\n"
            "# unknown keys are ignored (the adapter may carry its own here).\n"
            "sim.memory.fade = 0.2\n"
            "sim.drift.rate = 0.05\n"
            "sim.jitter = 0.15\n"
            "sim.memory.cap = 64\n");

        return { true,
            "scaffolded " + project.string()
                + " (CMakeLists.txt, main.cpp, host.ini) — build and run it" };
    }
}
