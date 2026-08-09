//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      LoggingTest.cpp
//
// Purpose:
//
//      Verifies the LCE logging system can initialize, write, flush,
//      and shut down correctly.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Logging/Logger.h"

namespace LCE::Tests
{
    bool LoggingTest()
    {
        LCE::Logging::Initialize();

        LCE::Logging::Trace("Logging test: Trace");
        LCE::Logging::Debug("Logging test: Debug");
        LCE::Logging::Info("Logging test: Info");
        LCE::Logging::Warning("Logging test: Warning");
        LCE::Logging::Error("Logging test: Error");
        LCE::Logging::Critical("Logging test: Critical");

        LCE::Logging::Flush();
        LCE::Logging::Shutdown();

        return true;
    }
}