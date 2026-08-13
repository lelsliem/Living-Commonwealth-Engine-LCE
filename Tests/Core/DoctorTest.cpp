//=============================================================================//
//                                                                             //
// Living Commonwealth Engine (LCE)                                           //
// Building living worlds through simulation.                                 //
//                                                                             //
// File:                                                                       //
//      DoctorTest.cpp                                                         //
//                                                                             //
// Purpose:                                                                    //
//      Verifies the LCE Doctor checks (0.5.0 SDK): each one is pure —        //
//      a path in, a verdict out — so the harness crafts real directories      //
//      and proves the pass/fail boundary. The CLI is just these checks        //
//      with a voice; the checks are the contract.                             //
//                                                                             //
// SPDX-License-Identifier: MIT                                                //
//                                                                             //
//=============================================================================//

#include "DoctorTest.h"

#include "LCEDoctor/Checks.h"

#include <filesystem>
#include <fstream>

namespace LCE::Tests
{
    namespace
    {
        using namespace LCE::Doctor;

        // One scratch workspace for the whole suite: a fresh directory
        // under the system temp, removed when the suite ends.
        struct Scratch
        {
            fs::path Root;

            Scratch()
            {
                Root = fs::temp_directory_path() / "lce-doctor-test";

                // A fresh start each run — a stale run's leftovers are
                // not the suite's business to keep.
                fs::remove_all(Root);
                fs::create_directories(Root);
            }

            ~Scratch()
            {
                fs::remove_all(Root);
            }

            fs::path Dir(const std::string& name) const
            {
                return Root / name;
            }

            void Write(const fs::path& file, const std::string& contents) const
            {
                fs::create_directories(file.parent_path());
                std::ofstream{ file } << contents;
            }
        };

        void MakeCMakeProject(const fs::path& dir, bool wiresLce)
        {
            fs::create_directories(dir);

            const std::string cmake = wiresLce
                ? "# pins the core\nFetchContent_Declare(\n"
                  "    LivingCommonwealthEngine\n    GIT_TAG v0.5.0)\n"
                : "# nothing about LCE\nproject(MyProject)\n";

            std::ofstream{ dir / "CMakeLists.txt" } << cmake;
        }

        void MakeXmakeProject(const fs::path& dir, bool wiresLce)
        {
            fs::create_directories(dir);

            const std::string xmake = wiresLce
                ? "-- drives the core\nadd_rules(\"lce.core\")\n"
                : "-- nothing about LCE\nset_project(\"MyProject\")\n";

            std::ofstream{ dir / "xmake.lua" } << xmake;
        }

        void MakeFakeCore(const fs::path& core)
        {
            fs::create_directories(core / "Include" / "LCE" / "Version");

            const auto header =
                core / "Include" / "LCE" / "Version" / "Version.h";

            std::ofstream{ header }
                << "namespace LCE::Version\n{\n"
                << "    inline constexpr int MajorValue = 0;\n"
                << "    inline constexpr int MinorValue = 5;\n"
                << "    inline constexpr std::string_view VersionString = \"0.5.0-alpha\";\n"
                << "}\n";
        }
    }

    bool DoctorTest()
    {
        Scratch scratch;

        //-------------------------------------------------------------------------
        // CheckTarget: what is and what isn't a project directory.
        //-------------------------------------------------------------------------
        {
            if (CheckTarget(fs::path{}).Passed
                || CheckTarget(scratch.Root / "nowhere").Passed)
            {
                return false;   // empty or missing → not a target
            }

            const auto fileTarget = scratch.Dir("afile");

            std::ofstream{ fileTarget } << "just a file";

            if (CheckTarget(fileTarget).Passed)
            {
                return false;   // a file is not a directory
            }

            if (!CheckTarget(scratch.Dir("")).Passed)
            {
                return false;   // the scratch root itself is a directory
            }
        }

        //-------------------------------------------------------------------------
        // CheckBuildSystem: CMake, xmake, both, neither.
        //-------------------------------------------------------------------------
        {
            if (CheckBuildSystem(scratch.Dir("empty")).Passed)
            {
                return false;   // an empty dir has no build system
            }

            MakeCMakeProject(scratch.Dir("cmake-project"), true);

            if (!CheckBuildSystem(scratch.Dir("cmake-project")).Passed)
            {
                return false;
            }

            MakeXmakeProject(scratch.Dir("xmake-project"), true);

            if (!CheckBuildSystem(scratch.Dir("xmake-project")).Passed)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // CheckLWiring: the project must reach LCE one supported way.
        //-------------------------------------------------------------------------
        {
            MakeCMakeProject(scratch.Dir("cmake-wired"), true);
            MakeCMakeProject(scratch.Dir("cmake-unwired"), false);

            if (!CheckLWiring(scratch.Dir("cmake-wired")).Passed
                || CheckLWiring(scratch.Dir("cmake-unwired")).Passed
                || CheckLWiring(scratch.Dir("empty")).Passed)
            {
                return false;
            }

            MakeXmakeProject(scratch.Dir("xmake-wired"), true);
            MakeXmakeProject(scratch.Dir("xmake-unwired"), false);

            if (!CheckLWiring(scratch.Dir("xmake-wired")).Passed
                || CheckLWiring(scratch.Dir("xmake-unwired")).Passed)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // CheckCoreCheckout: present and versioned, fetch-only, or missing.
        //-------------------------------------------------------------------------
        {
            const auto core = scratch.Dir("core");

            MakeFakeCore(core);

            const auto report = CheckCoreCheckout(core);

            if (!report.Passed
                || report.Detail.find("0.5.0") == std::string::npos)
            {
                return false;   // found, and the version was read
            }

            // A FetchContent project has no local checkout — the build
            // pins it; the doctor says so and passes.
            if (!CheckCoreCheckout(fs::path{}).Passed)
            {
                return false;
            }

            if (CheckCoreCheckout(scratch.Dir("not-a-core")).Passed)
            {
                return false;   // missing Version.h → fail
            }
        }

        //-------------------------------------------------------------------------
        // CheckHeaderLayout: every LCE include the project references
        // must resolve against the core's Include tree — a moved header
        // is a build break the doctor names before the compiler does.
        //-------------------------------------------------------------------------
        {
            // A core with one header in a known place.
            const auto core = scratch.Dir("layout-core");
            const auto header =
                core / "Include" / "LCE" / "Simulation" / "Simulation.h";

            fs::create_directories(header.parent_path());
            std::ofstream{ header } << "// the tick\n";

            // A project whose include resolves.
            const auto good = scratch.Dir("layout-good");
            fs::create_directories(good);
            std::ofstream{ good / "main.cpp" }
                << "#include \"LCE/Simulation/Simulation.h\"\n";

            const auto goodReport = CheckHeaderLayout(good, core);

            if (!goodReport.Passed
                || goodReport.Detail.find("all resolve") == std::string::npos)
            {
                return false;
            }

            // A project referencing a header the core no longer has —
            // the stale path must be named.
            const auto bad = scratch.Dir("layout-bad");
            fs::create_directories(bad);
            std::ofstream{ bad / "main.cpp" }
                << "#include \"LCE/Simulation/Behaviour.h\"\n"
                << "#include \"LCE/Simulation/Simulation.h\"\n";

            const auto badReport = CheckHeaderLayout(bad, core);

            if (badReport.Passed
                || badReport.Detail.find("Behaviour.h") == std::string::npos)
            {
                return false;
            }

            // A project with no LCE includes at all — the doctor says so.
            const auto none = scratch.Dir("layout-none");
            fs::create_directories(none);
            std::ofstream{ none / "main.cpp" } << "int main() {}\n";

            if (CheckHeaderLayout(none, core).Passed)
            {
                return false;
            }

            // No local core (FetchContent) — nothing to check against.
            if (!CheckHeaderLayout(good, fs::path{}).Passed)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // CheckToolchain: whatever this environment is, the check must
        // answer — and the PATH half must match what the environment
        // actually has (MSVC visibility depends on the shell; the
        // doctor reports it honestly either way).
        //-------------------------------------------------------------------------
        {
            const auto report = CheckToolchain();

            const bool cmakeSeen =
                report.Detail.find("cmake ✓") != std::string::npos;
            const bool xmakeSeen =
                report.Detail.find("xmake ✓") != std::string::npos;

            if (cmakeSeen != OnPath("cmake.exe")
                || xmakeSeen != OnPath("xmake.exe"))
            {
                return false;
            }

            // A failing check names the missing pieces — never silent.
            if (!report.Passed
                && report.Detail.find("✗") == std::string::npos)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // ScaffoldEmbedder (0.8.8) — lce-doctor init. The scaffold is the
        // Embedding.md recipe as files; a generated project must pass the
        // doctor's own checks (the doc and the tool agree by
        // construction), and a bad name must be refused.
        //-------------------------------------------------------------------------
        {
            const auto report =
                ScaffoldEmbedder(scratch.Root, "embedder");

            if (!report.Passed)
            {
                return false;   // the scaffold wrote its files
            }

            const auto project = scratch.Dir("embedder");

            if (!fs::exists(project / "CMakeLists.txt")
                || !fs::exists(project / "main.cpp")
                || !fs::exists(project / "host.ini"))
            {
                return false;   // all three files exist
            }

            // The recipe's shape: FetchContent wiring, LCE::Core, the
            // runtime loop, the knob file.
            const auto cmake = ReadText(project / "CMakeLists.txt");
            const auto main = ReadText(project / "main.cpp");
            const auto ini = ReadText(project / "host.ini");

            if (cmake.find("FetchContent_Declare") == std::string::npos
                || cmake.find("LCE::Core") == std::string::npos
                || main.find("FixedStep") == std::string::npos
                || main.find("EventBus") == std::string::npos
                || ini.find("sim.jitter") == std::string::npos)
            {
                return false;   // the recipe's parts are present
            }

            // The scaffold passes the doctor's own contract checks.
            if (!CheckTarget(project).Passed
                || !CheckBuildSystem(project).Passed
                || !CheckLWiring(project).Passed)
            {
                return false;
            }

            // The same doctor refuses to scaffold over something, or
            // with a name that walks a path.
            if (ScaffoldEmbedder(scratch.Root, "embedder").Passed
                || ScaffoldEmbedder(scratch.Root, "../evil").Passed
                || ScaffoldEmbedder(scratch.Root, "").Passed)
            {
                return false;
            }
        }

        return true;
    }
}
