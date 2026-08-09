//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      ClockTest.cpp
//
// Purpose:
//
//      Verifies that the LCE simulation clock measures elapsed time and can
//      be reset.
//
// Project:
//
//      Living Commonwealth Engine (LCE)
//
// License:
//
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//
//      (c) 2026-present LCE Contributors
//=============================================================================//

#include "LCE/Time/Clock.h"

#include <chrono>
#include <thread>

namespace LCE::Tests
{
    bool ClockTest()
    {
        Time::Clock clock;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto elapsed = clock.Elapsed();

        return elapsed.count() > 0.0;;
        
    }
}