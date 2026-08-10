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
//   5. The toolchain is visible (CMake / xmake / MSVC).
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

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
    // 5. The toolchain is visible: CMake and xmake on PATH, MSVC via the
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
