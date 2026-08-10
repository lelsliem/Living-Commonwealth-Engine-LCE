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
// │            “Show me the pain before you show me the cure.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · LCE Doctor
//
// The CLI. Point it at a project; it checks the SDK contract and
// prints a plain pass/fail log with the reason for every ✗. The checks
// themselves live in Checks.h — pure, and harness-tested.
//
//   LCEDoctor [path]          check a project (default: the working dir)
//
// Exit code: 0 when every essential check passes, 1 otherwise — so a
// CI step can fail on a project that does not meet the contract.
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCEDoctor/Checks.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace
{
    // The core path the doctor verifies: the LCE_CORE_PATH override
    // when set, else the adapter's conventional checkout.
    std::filesystem::path ResolveCorePath(const std::filesystem::path& target)
    {
        const char* override = LCE::Doctor::SafeGetEnv("LCE_CORE_PATH");

        if (override != nullptr && *override != '\0')
        {
            return override;
        }

        // A CMake project that fetches LCE has no local checkout — the
        // build pins it; nothing to verify here.
        const auto cmake = LCE::Doctor::ReadText(target / "CMakeLists.txt");

        if (cmake.find("LivingCommonwealthEngine") != std::string::npos
            || cmake.find("find_package(LCE") != std::string::npos)
        {
            return {};
        }

        // The xmake adapter convention (the lce.core rule's default).
        return "C:/LivingCommonwealthEngine";
    }
}

int main(int argc, char** argv)
{
    using namespace LCE::Doctor;

    std::printf("LCE Doctor — the SDK contract, checked.\n\n");

    // The target: the argument, or the working directory.
    const fs::path target =
        argc > 1 ? fs::path{ argv[1] } : fs::current_path();

    std::printf("target: %s\n\n", target.string().c_str());

    const auto print = [](const char* label, const Report& report)
    {
        std::printf("  [%s] %s\n", report.Passed ? "OK " : "FAIL", label);
        std::printf("        %s\n", report.Detail.c_str());
    };

    const Report targetReport = CheckTarget(target);
    print("target", targetReport);

    int passed = targetReport.Passed ? 1 : 0;
    int total = 1;

    if (targetReport.Passed)
    {
        const Report buildSystem = CheckBuildSystem(target);
        print("build system", buildSystem);
        ++total;
        passed += buildSystem.Passed ? 1 : 0;

        const Report wiring = CheckLWiring(target);
        print("LCE wiring", wiring);
        ++total;
        passed += wiring.Passed ? 1 : 0;

        const Report core = CheckCoreCheckout(ResolveCorePath(target));
        print("core checkout", core);
        ++total;
        passed += core.Passed ? 1 : 0;
    }

    const Report toolchain = CheckToolchain();
    print("toolchain", toolchain);
    ++total;
    passed += toolchain.Passed ? 1 : 0;

    std::printf("\nLCE Doctor: %d/%d checks passed.\n", passed, total);

    if (passed == total)
    {
        std::printf("The project meets the SDK contract. Build on.\n");
        return 0;
    }

    std::printf("Fix the FAILs above, then run LCE Doctor again.\n");

    return 1;
}
