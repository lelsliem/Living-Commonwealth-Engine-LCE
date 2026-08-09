//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EntityRegistryTest.h
//
// Purpose:
//
//      Verifies the Entity Registry: creation, destruction, generational
//      reuse, and component attachment — and that stale IDs can never
//      alias a live entity.
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
#pragma once

namespace LCE::Tests
{
    bool EntityRegistryTest();
}
