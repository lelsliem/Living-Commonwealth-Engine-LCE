//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      HeaderMapTest.h
//
// Purpose:
//
//      The public-header surface guard (0.8.1): the SDK's headers are
//      the contract — every path a project can include. This suite
//      freezes that map and fails the harness the moment the tree
//      disagrees with it, so a header move (or a stale include) is
//      caught in the harness before it breaks a downstream build.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#pragma once

namespace LCE::Tests
{
    bool HeaderMapTest();
}
