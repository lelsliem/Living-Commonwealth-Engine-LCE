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

        if (find(cmake, "LivingCommonwealthEngine")
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
}
